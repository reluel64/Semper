/*
 * FolderView source
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */
#include <semper_api.h>
#include <string_util.h>
#include <linked_list.h>
#include <sources/source.h>
#include <surface.h>
#include <mem.h>
#include <pthread.h>

#ifdef WIN32
#include <initguid.h>
#include <cairo/cairo-win32.h>
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <CommonControls.h>
#elif __linux__
#include <dirent.h>
#endif

#define SYNC_CLEAR    0x1
#define SYNC_RSYNC    0x8
#define SYNC_CHILDREN 0x2
#define SYNC_ICONS    0x4
#define UPDATE_FILE_LIST 0x1
#define UPDATE_SYNC     0x2

typedef enum
{
    _none = 0,
    _parent = 1,
    _child = 2
} folderview_type;

typedef enum
{
    t_name = 0,
    t_icon = 1,
    t_size = 2,
    t_ext = 3,
    t_date_mod = 4,
    t_date_creat = 5,
    t_date_acces = 6
} folderview_child_type;

typedef enum
{
    pt_path = 0,
    pt_folder_sz = 1,
    pt_file_cnt = 2,
    pt_folder_cnt = 3
} folderview_parent_type;

typedef enum
{
    s_name = 0,
    s_size = 2,
    s_date_creat = 3,
    s_date_mod = 4,
    s_date_acces = 5
} folderview_sort_type;

typedef struct
{
    size_t size;
    size_t mod_date;
    size_t creat_date;
    size_t access_date;
    size_t file_count;
    list_entry current;
    unsigned char dir;
    unsigned char *ext;
    unsigned char *name;
    unsigned char *file_path;
} folderview_item;

typedef struct
{
    void *ip;
    size_t items;
    size_t child_count;
    size_t base_len;
    list_entry files;
    list_entry children;
    pthread_mutex_t mutex;
    pthread_t thread;
    folderview_item *start;
    folderview_sort_type stype;
    folderview_parent_type ptype;
    unsigned char s_desc;
    void* work;
    unsigned char collect_mode;
    void *update;
    void *stop; //trigger for child synchronization
    unsigned char sys;
    unsigned char hidden;
    unsigned char show_files;
    unsigned char show_folders;
    unsigned char hide_ext;
    unsigned char *qcomplete_act;
    unsigned char *path;
    unsigned char *base_path;
    unsigned char *icon_root; //must not be freed
} folderview_parent;

typedef struct
{
    folderview_parent *parent;
    folderview_child_type type;
    folderview_item *item;
    size_t index;
    unsigned char *icon_path;
    unsigned char *str_val;
    list_entry current;
} folderview_child;

typedef struct
{
    folderview_type type;
    void *data;
} folderview_holder;

typedef struct
{
    unsigned char *dir;
    list_entry current;
} folderview_dir_list;

static int folderview_collect(unsigned char *root, folderview_parent *f, folderview_item *fi, list_entry *head);
static void* folderview_collect_thread(void* vfi);
static void folderview_item_child_sync(folderview_parent *fp, unsigned char flag);

#ifdef WIN32
static size_t dword_qword(size_t low, size_t high)
{
    return (low | (high << 31));
}
#endif
void folderview_init(void **spv, void *ip)
{
    folderview_holder *fh = zmalloc(sizeof(folderview_holder));
    *spv = fh;
}

void folderview_reset(void *spv, void *ip)
{
    folderview_holder *fh = spv;

    folderview_child *fc = ((fh->type == _child) ? fh->data : NULL);
    folderview_parent *fp = ((fh->type == _parent) ? fh->data : (fc != NULL ? fc->parent : NULL));
    unsigned char* temp = param_string("Path", EXTENSION_XPAND_VARIABLES, ip, NULL);
    void* parent = get_parent(temp, ip);

    if(fc)
    {
        fc->parent = NULL; //assume  the child is oprhan
    }

    if((parent && fh->type == _none) || fh->type == _child)
    {
        void *pv = get_private_data(parent);

        if(fp == NULL)
        {
            if(((folderview_holder*)pv)->type == _parent)
                fp = ((folderview_holder*)pv)->data;
        }

        if(fc == NULL)
        {
            fc = zmalloc(sizeof(folderview_child));
            list_entry_init(&fc->current);
        }

        fc->index = param_size_t("ChildIndex", ip, 0);
        temp = param_string("ChildType", EXTENSION_XPAND_ALL, ip, "Name");

        if(temp)
        {
            if(strcasecmp(temp, "Name") == 0)
                fc->type = t_name;

            else if(strcasecmp(temp, "Icon") == 0)
                fc->type = t_icon;

            else if(strcasecmp(temp, "Size") == 0)
                fc->type = t_size;

            else if(strcasecmp(temp, "Extension") == 0)
                fc->type = t_ext;

            else if(!strcasecmp(temp, "DateAccessed"))
                fc->type = t_date_acces;

            else if(!strcasecmp(temp, "DateCreated"))
                fc->type = t_date_creat;

            else if(!strcasecmp(temp, "DateModified"))
                fc->type = t_date_mod;
        }

        if(fp)
        {
            safe_flag_set(fp->update, UPDATE_FILE_LIST);
            fc->parent = fp;
        }

        fh->data = fc;

        fh->type = _child;

        if((fc->current.next == NULL || linked_list_empty(&fc->current)) && fp)
        {
            pthread_mutex_lock(&fp->mutex);
            linked_list_add_last(&fc->current, &fp->children);
            pthread_mutex_unlock(&fp->mutex);
        }

    }
    else
    {
        source *s = ip;
        surface_data *sd = s->sd;
        pthread_mutex_t empty_mtx;
        memset(&empty_mtx, 0, sizeof(pthread_mutex_t));

        //this was a child so we will make it a parent by destroying its previous structure and re-creating it
        if(fc)
        {
            if(fc->parent)
                pthread_mutex_lock(&fc->parent->mutex);

            sfree((void**)&fc->icon_path);

            if(fc->current.next)
                linked_list_remove(&fc->current);

            sfree((void**)&fc->str_val);
            sfree((void**)&fh->data);

            if(fc->parent)
                pthread_mutex_unlock(&fc->parent->mutex);
        }

        if(fp == NULL)
        {
            fp = zmalloc(sizeof(folderview_parent));
            list_entry_init(&fp->children);
            list_entry_init(&fp->files);
        }
        else
        {
            safe_flag_set(fp->stop, 1);

            if(fp->thread)
                pthread_join(fp->thread, NULL);

            safe_flag_set(fp->stop, 0);
        }

        sfree((void**)&fp->base_path);
        sfree((void**)&fp->qcomplete_act);

        safe_flag_destroy(&fp->update);
        safe_flag_destroy(&fp->work);
        safe_flag_destroy(&fp->stop);

        fp->update = safe_flag_init();
        fp->work = safe_flag_init();
        fp->stop = safe_flag_init();

        fp->qcomplete_act = clone_string(param_string("CompletionAction", EXTENSION_XPAND_ALL, ip, NULL));
        fp->collect_mode = param_size_t("CollectType", ip, 0);
        fp->child_count = param_size_t("ChildCount", ip, 0);
        fp->base_path = clone_string(param_string("Path", EXTENSION_XPAND_ALL, ip, NULL));
        fp->hide_ext = param_bool("HideExtensions", ip, 0);
        fp->sys = param_bool("ShowSystem", ip, 0);
        fp->hidden = param_bool("ShowHidden", ip, 0);
        fp->show_files = param_bool("ShowFiles", ip, 1);
        fp->show_folders = param_bool("ShowFolders", ip, 1);
        fp->path = fp->base_path;
        fp->base_len = string_length(fp->base_path);
        fp->ip = ip;
        fh->data = fp;
        fh->type = _parent;
        safe_flag_set(fp->update, 1);
        temp = param_string("SortBy", EXTENSION_XPAND_ALL, ip, "Name");

        if(temp)
        {
            if(!strcasecmp(temp, "Name"))
                fp->stype = s_name;
            else if(!strcasecmp(temp, "DateAccessed"))
                fp->stype = s_date_acces;
            else if(!strcasecmp(temp, "DateCreated"))
                fp->stype = s_date_creat;
            else if(!strcasecmp(temp, "DateModified"))
                fp->stype = s_date_mod;

            if(!strcasecmp(temp, "Size"))
                fp->stype = s_size;
        }

        temp = param_string("ParentType", EXTENSION_XPAND_ALL, ip, "Path");

        if(temp)
        {
            if(!strcasecmp(temp, "Path"))
                fp->ptype = pt_path;
            else if(!strcasecmp(temp, "FolderSize"))
                fp->ptype = pt_folder_sz;
            else if(!strcasecmp(temp, "FolderCount"))
                fp->ptype = pt_folder_cnt;
            else if(!strcasecmp(temp, "FileCount"))
                fp->ptype = pt_file_cnt;
        }

        fp->s_desc = param_bool("SortDescending", ip, 0);

        if(memcmp(&fp->mutex, &empty_mtx, sizeof(pthread_mutex_t)) == 0)
        {
            pthread_mutexattr_t mutex_attr;
            pthread_mutexattr_init(&mutex_attr);
            pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
            pthread_mutex_init(&fp->mutex, &mutex_attr);
            pthread_mutexattr_destroy(&mutex_attr);
        }

        fp->icon_root = sd->sp.surface_dir;
    }
}

double folderview_update(void *spv)
{
    folderview_holder *fh = spv;
    folderview_child  *fc = fh->type == _child ? fh->data : NULL;
    folderview_parent *fp = fh->type == _parent ? fh->data : (fc != NULL ? fc->parent : NULL);
    double ret = 0.0;

    if(fp == NULL)
    {
        return(ret);
    }

    pthread_mutex_lock(&fp->mutex);

    if(fh->type == _parent)
    {
        if(safe_flag_get(fp->work) == 0 && fp->thread)
        {
            pthread_join(fp->thread, NULL);
            fp->thread = 0;
        }

        if(safe_flag_get(fp->work) == 0 && safe_flag_get(fp->update) && fp->thread == 0)
        {
            int status = 0;
            safe_flag_set(fp->work, 1);
            status = pthread_create(&fp->thread, NULL, folderview_collect_thread, fp);

            if(status)
            {
                safe_flag_set(fp->work, 0);
                diag_crit("%s %d Failed to start folderview_collect_thread. Status %x", __FUNCTION__, __LINE__, status);
            }
            else
            {
                /*Because the update process is fast and the
                * collector thread does not have the occasion
                * to pend for the mutex, we're giving the
                * chance for it to start by polling the
                * work variable until is no longer 1
                */
                while(safe_flag_get(fp->work) == 1);

            }
        }

        if(fp->ptype != pt_path)
            ret = (double)fp->items;
    }
    else if(fh->type == _child)
    {
        if(fc->item)
        {
            if(fc->type == t_size)
                ret = (double)fc->item->size;
        }
    }

    pthread_mutex_unlock(&fp->mutex);
    return(ret);
}

unsigned char *folderview_string(void* spv)
{
    folderview_holder *fh = spv;

    folderview_child  *fc = fh->type == _child ? fh->data : NULL;
    folderview_parent *fp = fh->type == _parent ? fh->data : (fc != NULL ? fc->parent : NULL);
    unsigned char *ret = "";

    if(fp == NULL)
    {
        return(ret);
    }

    pthread_mutex_lock(&fp->mutex);

    if(fh->type == _child)
    {
        sfree((void**)&fc->str_val);

        if(fc->type == t_name)
            fc->str_val = fc->item != NULL && fc->item->name != NULL ? clone_string(fc->item->name) : zmalloc(1);
        else if(fc->type == t_icon)
            fc->str_val = fc->icon_path != NULL ? clone_string(fc->icon_path) : zmalloc(1);
        else if(fc->type == t_ext)
            fc->str_val = fc->item != NULL && fc->item->ext != NULL ? clone_string(fc->item->ext) : zmalloc(1);

        ret = fc->str_val;

    }
    else if(fh->type == _parent)
    {
        if(fp)
        {
            if(fp->ptype == pt_path)
                ret = fp->path;
        }
    }

    pthread_mutex_unlock(&fp->mutex);

    return(ret);
}

void folderview_command(void* spv, unsigned char* command)
{

    folderview_holder *fh = spv;
    folderview_child  *fc = fh->type == _child ? fh->data : NULL;
    folderview_parent *fp = fh->type == _parent ? fh->data : (fc != NULL ? fc->parent : NULL);

    if(safe_flag_get(fp->work)== 0)
    {
        pthread_mutex_lock(&fp->mutex);

        if(fc && fp->start && command && fc->index < fp->child_count && fc->item)
        {
            if(strcasecmp("Open", command) == 0)
            {
                if(fc->item->dir)
                {
                    size_t path_len = string_length(fp->path);
                    size_t dir_len = string_length(fc->item->name);
                    unsigned char *temp = zmalloc(path_len + dir_len + 2); //space for null and for slash

                    snprintf(temp, path_len + dir_len + 2, "%s/%s", fp->path, fc->item->name);
                    uniform_slashes(temp);

                    if(fp->path != fp->base_path)
                    {
                        sfree((void**)&fp->path);
                    }

                    fp->path = temp;
                    safe_flag_set(fp->update, UPDATE_FILE_LIST);
                }
                else
                {

#ifdef WIN32
                    wchar_t *s = utf8_to_ucs(fc->item->file_path);
                    wchar_t *d = utf8_to_ucs(fp->path);

                    if(s && d)
                    {

                        CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
                        ShellExecuteW(NULL, NULL, s, NULL, d, SW_SHOW);
                        CoUninitialize();

                        sfree((void**)&s);
                        sfree((void**)&d);
                    }
                    else if(s)
                        sfree((void**)&s);
                    else
                        sfree((void**)&d);

#endif
                }
            }
        }
        else if(fc == NULL && command)
        {
            if(strcasecmp("Back", command) == 0 && strcasecmp(fp->path, fp->base_path))
            {
                unsigned char *lo = strrchr(fp->path, '/');

                if(lo == NULL)
                    lo = strrchr(fp->path, '\\');


                if(fp->path != fp->base_path && strncasecmp(fp->path, fp->base_path, lo - fp->path))
                {
                    sfree((void**)&fp->path);
                    fp->path = fp->base_path;
                }
                else if(lo)
                {
                    lo[0] = 0;
                }

                safe_flag_set(fp->update, UPDATE_FILE_LIST);

            }
            else if(fp->start)
            {
                unsigned char up = strcasecmp("Up", command) == 0;
                unsigned char down = strcasecmp("Down", command) == 0;

                if(up || down)
                {

                    if(((up && fp->start->current.prev != &fp->files) || (down && fp->start->current.next != &fp->files)))
                    {
                        if(up)
                        {
                            fp->start = element_of(fp->start->current.prev, fp->start, current);

                        }
                        else
                        {
                            fp->start = element_of(fp->start->current.next, fp->start, current);
                        }

                        safe_flag_set(fp->update, UPDATE_SYNC);
                    }
                }
            }
        }

        pthread_mutex_unlock(&fp->mutex);
    }

}

static void folderview_destroy_list(list_entry *head)
{
    folderview_item *fi = NULL;
    folderview_item *fit = NULL;
    list_enum_part_safe(fi, fit, head, current)
    {
        linked_list_remove(&fi->current);
        sfree((void**)&fi->name);
        sfree((void**)&fi->file_path);
        sfree((void**)&fi);
    }
}

void folderview_destroy(void **spv)
{
    folderview_holder *fh = *spv;
    folderview_parent *fp = NULL;

    if(fh->type == _parent) //clean the parent and any references to it
    {
        fp = fh->data;
        safe_flag_set(fp->stop, 1);

        if(fp->thread)
            pthread_join(fp->thread, NULL);

        fp->thread = 0;
        folderview_child *fc = NULL;
        folderview_child *tfc = NULL;
        folderview_destroy_list(&fp->files);
        folderview_item_child_sync(fp, SYNC_CLEAR | SYNC_ICONS | SYNC_CHILDREN);
        //invalidate children
        list_enum_part_safe(fc, tfc, &fp->children, current)
        {
            linked_list_remove(&fc->current);
            fc->parent = NULL;
        }

        if(fp->path != fp->base_path)
            sfree((void**)&fp->path);

        sfree((void**)&fp->base_path);
        sfree((void**)&fp->qcomplete_act);
        pthread_mutex_destroy(&fp->mutex);
        safe_flag_destroy(&fp->update);
        safe_flag_destroy(&fp->work);
        safe_flag_destroy(&fp->stop);
        sfree((void**)&fh->data);
        sfree(spv);
    }
    else if(fh->type == _child)
    {
        folderview_child *fc = fh->data;
        folderview_parent *fp = fc->parent;

        if(fp)
            pthread_mutex_lock(&fp->mutex);

        sfree((void**)&fc->icon_path);

        if(fc->current.next)
            linked_list_remove(&fc->current);

        sfree((void**)&fc->str_val);
        sfree((void**)&fh->data);
        sfree(spv);

        if(fp)
            pthread_mutex_unlock(&fp->mutex);
    }
    else
    {
        sfree(spv);
    }
}


static int folderview_sort_compare(list_entry *l1, list_entry *l2, void *pv)
{
    int res = 0;
    folderview_item *fi1 = element_of(l1, fi1, current);
    folderview_item *fi2 = element_of(l2, fi2, current);
    folderview_parent *fp = pv;

    if(fp->stype == s_name && fi1->name && fi2->name)
    {
        res = strcasecmp(fi1->name, fi2->name);

    }
    else if(fp->stype == s_size)
    {
        if(fi1->size < fi2->size)
            res = -1;
        else if(fi1->size > fi2->size)
            res = 1;
    }
    else if(fp->stype == s_date_acces)
    {
        if(fi1->access_date < fi2->access_date)
            res = -1;
        else if(fi1->access_date > fi2->access_date)
            res = 1;
    }
    else if(fp->stype == s_date_creat)
    {
        if(fi1->creat_date < fi2->creat_date)
            res = -1;
        else if(fi1->creat_date > fi2->creat_date)
            res = 1;
    }
    else if(fp->stype == s_date_mod)
    {
        if(fi1->mod_date < fi2->mod_date)
            res = -1;
        else if(fi1->mod_date > fi2->mod_date)
            res = 1;
    }

    return(fp->s_desc ? res * -1 : res);
}



static int folderview_collect(unsigned char *root, folderview_parent *f, folderview_item *fi, list_entry *head)
{
    unsigned char *file = root;
    list_entry qbase = {0};
    list_entry_init(&qbase);


    while(file)
    {
        if(safe_flag_get(f->stop)== 0)
        {

            size_t fpsz = string_length(file);
#ifdef WIN32
            unsigned char* filtered = zmalloc(fpsz + 6);
            snprintf(filtered, fpsz + 6, "%s/*.*", file);

            unsigned short* filtered_uni = utf8_to_ucs(filtered);
            sfree((void**)&filtered);

            WIN32_FIND_DATAW wfd = { 0 };

            void* fh = FindFirstFileW(filtered_uni, &wfd);
            sfree((void**)&filtered_uni);


#elif __linux__

            DIR *dh = opendir(file);



            struct dirent *dat = (dh ? readdir(dh) : NULL);
#endif

            do
            {

                size_t size = 0;

                unsigned char dir = 0;
                unsigned char hidden = 0;
                unsigned char sys = 0;
#ifdef WIN32
                size = dword_qword(wfd.nFileSizeLow, wfd.nFileSizeHigh);

                dir = (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                hidden = (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
                sys = (wfd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0;
#elif __linux__

                if(dat == NULL)
                    break;

                dir = (dat->d_type == DT_DIR);
#endif
#ifdef WIN32

                if(safe_flag_get(f->stop) || fh == INVALID_HANDLE_VALUE)
                {
                    break;
                }

#endif
                unsigned char* temp = NULL;
#ifdef WIN32

                if((dir == 1 && f->show_folders == 0) || (dir == 0 && f->show_files == 0) || (sys && f->sys == 0) || (hidden && f->hidden == 0))
                    continue;


                temp = ucs_to_utf8(wfd.cFileName, NULL, 0);
#elif __linux__

                temp = clone_string(dat->d_name);
#endif

                if(temp == NULL)
                    break;


                if(!strcasecmp(temp, "..") || !strcasecmp(temp, "."))
                {
                    sfree((void**)&temp);
                    continue;
                }

                pthread_mutex_lock(&f->mutex);

                if(f->ptype == pt_folder_sz && dir == 0)
                    f->items += size;
                else if(f->ptype == pt_file_cnt && dir == 0)
                    f->items++;
                else if(f->ptype == pt_folder_cnt && dir)
                    f->items++;

                pthread_mutex_unlock(&f->mutex);

                if((f->collect_mode == 2 && dir == 0) || (f->collect_mode == 1 && fi == NULL) || f->collect_mode == 0)
                {
                    size_t temp_len = string_length(temp);
                    folderview_item *fi = zmalloc(sizeof(folderview_item));

                    list_entry_init(&fi->current);
                    linked_list_add(&fi->current, head);
                    fi->name = temp;
                    fi->file_path = zmalloc(temp_len + fpsz + 2);
                    snprintf(fi->file_path, temp_len + fpsz + 2, "%s/%s", file, temp);

                    if(dir == 0 && f->hide_ext)
                    {
                        unsigned char *dot = strrchr(temp, '.');

                        if(dot)
                            dot[0] = 0;
                    }

                    uniform_slashes(fi->file_path);
                    fi->dir = dir;
#ifdef WIN32
                    fi->access_date = dword_qword(wfd.ftLastAccessTime.dwHighDateTime, wfd.ftLastAccessTime.dwHighDateTime);
                    fi->mod_date = dword_qword(wfd.ftLastWriteTime.dwHighDateTime, wfd.ftLastWriteTime.dwHighDateTime);
                    fi->creat_date = fi->dir ? dword_qword(wfd.ftLastWriteTime.dwHighDateTime, wfd.ftLastWriteTime.dwHighDateTime) :
                                     dword_qword(wfd.ftCreationTime.dwHighDateTime, wfd.ftCreationTime.dwHighDateTime);
#endif
                    fi->ext = strchr(fi->name, '.');

                    if(fi->dir == 0)
                    {
                        fi->size = size;
                    }
                }
                else if(f->collect_mode && dir)
                {
                    size_t res_sz = string_length(temp);
                    unsigned char* ndir = zmalloc(res_sz + fpsz + 2);
                    snprintf(ndir, res_sz + fpsz + 2, "%s/%s", file, temp);
                    folderview_dir_list *fdl = zmalloc(sizeof(folderview_dir_list));
                    list_entry_init(&fdl->current);
                    linked_list_add(&fdl->current, &qbase);
                    fdl->dir = ndir;
                    uniform_slashes(ndir);
                    sfree((void**)&temp);
                }
                else if(f->collect_mode == 1 && fi)
                {
                    if(fi->dir && dir == 0)
                    {
                        fi->size += size;
                    }

                    sfree((void**)&temp);
                }
                else
                {
                    sfree((void**)&temp);
                }
            }

#ifdef WIN32

            while(safe_flag_get(f->stop) == 0 && FindNextFileW(fh, &wfd));

#elif __linux__

            while(safe_flag_get(f->stop) == 0 && (dat = readdir(dh)) != NULL);

#endif

#ifdef WIN32
            (fh != NULL && fh != INVALID_HANDLE_VALUE) ? FindClose(fh) : 0;
            fh = NULL;
#elif __linux__
            dh ? closedir(dh) : 0;
#endif
        }

        //pop a directory from the queue
        if(root != file)
        {
            sfree((void**)&file);
        }

        if(linked_list_empty(&qbase) == 0)
        {
            folderview_dir_list *fdl = element_of(qbase.prev, fdl, current);
            file = fdl->dir;
            linked_list_remove(&fdl->current);
            sfree((void**)&fdl);
        }
        else
        {
            file = NULL;
            break;
        }
    }

    return (0);
}

typedef struct
{
    unsigned char *buf;
    size_t csz;
} png_writer;


static cairo_status_t folderview_writer(void *pv, const unsigned char *data, unsigned int length)
{
    png_writer *pw = pv;
    void *nbuf = realloc(pw->buf, pw->csz + length);

    if(nbuf)
    {
        pw->buf = nbuf;
        memcpy(pw->buf + pw->csz, data, length);
        pw->csz += length;
    }
    else
    {
        sfree((void**)&pw->buf);
        memset(pw, 0, sizeof(png_writer));
        return(CAIRO_STATUS_NO_MEMORY);
    }

    return(CAIRO_STATUS_SUCCESS);
}

static int folderview_create_icon(unsigned char *store_root, unsigned char *file_path, folderview_child *fc)
{
#ifdef WIN32
    unsigned short *s = utf8_to_ucs(file_path);
    IImageList* ico_list = NULL;
    SHFILEINFOW sh = {0};
    ICONINFO ico_inf = {0};
    HICON ico = NULL;
    BITMAP img = {0};

    if(SHGetFileInfoW(s, 0, &sh, sizeof(sh), SHGFI_SYSICONINDEX) == 0)
    {
        sfree((void**)&s);
        return(-1);
    }

    sfree((void**)&s); //garbage

    if(SHGetImageList(0x2, &IID_IImageList, (void**)&ico_list) != S_OK)
    {
        DestroyIcon(sh.hIcon);
        return(-1);
    }

    if(ico_list->lpVtbl->GetIcon(ico_list, sh.iIcon, ILD_IMAGE, &ico) != S_OK)
    {
        DestroyIcon(sh.hIcon);
        ico_list->lpVtbl->Release(ico_list);
        return(-1);
    }

    if(ico && GetIconInfo(ico, &ico_inf))
    {
        if(GetObject(ico_inf.hbmColor, sizeof(BITMAP), &img))
        {
            png_writer pw = {0};
            cairo_surface_t *srf = cairo_win32_surface_create_with_dib(CAIRO_FORMAT_ARGB32, img.bmWidth, img.bmHeight);
            HDC dc = cairo_win32_surface_get_dc(srf);
            DrawIconEx(dc, 0, 0, ico, 0, 0, 0, NULL, DI_NORMAL);

            sfree((void**)&fc->icon_path);
            size_t srlen = string_length(store_root);
            fc->icon_path = zmalloc(srlen + 80);
            snprintf(fc->icon_path, srlen + 66, "%s/%llu.png", store_root, fc->index);

            uniform_slashes(fc->icon_path);
            unsigned short *uni = utf8_to_ucs(fc->icon_path);


            cairo_surface_write_to_png_stream(srf, folderview_writer, &pw);
            FILE *f = _wfopen(uni, L"wb");
            sfree((void**)&uni);

            fwrite(pw.buf, 1, pw.csz, f);
            fclose(f);
            sfree((void**)&pw.buf);
            cairo_surface_destroy(srf);

        }

        DeleteObject(ico_inf.hbmColor);
        DeleteObject(ico_inf.hbmMask);
    }

    DestroyIcon(sh.hIcon);
    DestroyIcon(ico);
    ico_list->lpVtbl->Release(ico_list);
#endif
    return(0);
}

static void folderview_item_child_sync(folderview_parent *fp, unsigned char flag)
{
    folderview_child *fc = NULL;

    if(flag & SYNC_CLEAR)
    {
        list_enum_part(fc, &fp->children, current)
        {

            if(flag & SYNC_ICONS)
                sfree((void**)&fc->icon_path);

            if(flag & SYNC_CHILDREN)
                fc->item = NULL;
        }
    }

    if(flag & SYNC_RSYNC)
    {
        if(linked_list_empty(&fp->files) == 0 && fp->start && (flag & SYNC_CHILDREN))
        {
            folderview_item *fi = fp->start;

            for(size_t i = 0; i <= fp->child_count; i++)
            {
                if(safe_flag_get(fp->stop))
                {
                    break;
                }
                else
                {
                    list_enum_part(fc, &fp->children, current)
                    {
                        if(fc->index == i)
                        {
                            fc->item = fi;
                        }
                    }
                }

                if(fi->current.next == &fp->files)
                    break;

                fi = element_of(fi->current.next, fi, current);
            }
        }

        if(flag & SYNC_ICONS)
        {
#ifdef WIN32
            CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
#endif
            list_enum_part(fc, &fp->children, current)
            {
                if(fc->type == t_icon && fc->item)
                {
                    folderview_create_icon(fp->icon_root, fc->item->file_path, fc);
                }
            }
#ifdef WIN32
            CoUninitialize();
#endif
        }
    }
}

static void *folderview_collect_thread(void* vfi)
{
    folderview_parent *fp = vfi;

    safe_flag_set(fp->work, 2); //set this to a non-1 value to allow the main thread to continue

    unsigned char update_flag = safe_flag_get(fp->update);

    if(update_flag & UPDATE_SYNC)
    {
        pthread_mutex_lock(&fp->mutex);
        safe_flag_set(fp->update, update_flag & ~UPDATE_SYNC);
        folderview_item_child_sync(fp, SYNC_CHILDREN | SYNC_ICONS | SYNC_RSYNC | SYNC_CLEAR);
        pthread_mutex_unlock(&fp->mutex);
    }

    else if(update_flag & UPDATE_FILE_LIST)
    {
        safe_flag_set(fp->update, update_flag & ~UPDATE_FILE_LIST);
        list_entry new_head = {0};
        list_entry_init(&new_head);

        //clear the file list
        pthread_mutex_lock(&fp->mutex);
        fp->items = 0;
        fp->start = NULL;
        folderview_destroy_list(&fp->files);
        folderview_item_child_sync(fp, SYNC_CLEAR | SYNC_ICONS | SYNC_CHILDREN);
        pthread_mutex_unlock(&fp->mutex);

        folderview_collect(fp->path, fp, NULL, &new_head); //get the file list

        if(safe_flag_get(fp->stop) == 0 && linked_list_empty(&new_head) == 0)
        {
            if(fp->collect_mode == 1)
            {
                folderview_item *fi = NULL;
                list_enum_part(fi, &new_head, current)
                {
                    if(safe_flag_get(fp->stop))
                        break;

                    if(fi->dir)
                        folderview_collect(fi->file_path, fp, fi, NULL);
                }
            }

            if(safe_flag_get(fp->stop))
            {
                folderview_destroy_list(&new_head);
            }
            else
            {
                //publish the new file list
                pthread_mutex_lock(&fp->mutex);
                linked_list_replace(&new_head, &fp->files);
                merge_sort(&fp->files, folderview_sort_compare, fp);
                fp->start = element_of(fp->files.next, fp->start, current);
                folderview_item_child_sync(fp, SYNC_ICONS | SYNC_CHILDREN | SYNC_RSYNC);
                pthread_mutex_unlock(&fp->mutex);
            }
        }
        else
        {
            folderview_destroy_list(&new_head);
        }
    }

    safe_flag_set(fp->work,0);

    if(fp->qcomplete_act)
        send_command(fp->ip, fp->qcomplete_act);

    return(NULL);
}
