#undef WIN32
#include <sources/folderinfo.h>
#include <semper_api.h>
#include <string_util.h>
#include <linked_list.h>
#ifdef WIN32
#include <windows.h>
#elif __linux__
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#endif
#include <mem.h>
#include <pthread.h>

typedef struct _folderinfo
{
        struct _folderinfo* parent;
        unsigned char* path;
        void *working;
        void *stop;
        pthread_t th;
        size_t folder_count;
        size_t size;
        size_t file_count;

        size_t ofolder_count;
        size_t osize;
        size_t ofile_count;

        unsigned char type;
        unsigned char hiddenf;
        unsigned char systemf;
        unsigned char recurse;
} folderinfo;

typedef struct
{
        unsigned char *dir;
        list_entry current;
} folderinfo_dir_list;


#ifdef WIN32
static size_t file_size(size_t low, size_t high)
{
    return (low | (high << 32));
}

#endif
void folderinfo_init(void** spv, void* ip)
{
    folderinfo* fi = NULL;

    fi = zmalloc(sizeof(folderinfo));

    fi->working = safe_flag_init();
    fi->stop = safe_flag_init();
    unused_parameter(ip);
    *spv = fi;
}

void folderinfo_reset(void* spv, void* ip)
{
    folderinfo* fi = spv;
    sfree((void**)&fi->path);
    fi->parent = NULL;

    if(fi->th)
    {
        safe_flag_set(fi->stop, 1);
        pthread_join(fi->th, NULL);
        safe_flag_set(fi->stop, 0);
        fi->th = 0;
    }

    unsigned char* tmp = param_string("Folder", EXTENSION_XPAND_VARIABLES, ip, "C:/");

    void* parent = get_parent(tmp, ip);

    if(parent == NULL)
        fi->path = clone_string(tmp);
    else
    {
        fi->parent = get_private_data(parent);
    }

    fi->recurse = param_bool("SubFolders", ip, 0);
    fi->hiddenf = param_bool("Hidden", ip, 0);
    fi->systemf = param_bool("System", ip, 0);

    unsigned char* type = param_string("Type", EXTENSION_XPAND_SOURCES | EXTENSION_XPAND_VARIABLES, ip, "FileCount");

    if(!strcasecmp(type, "FileCount"))
    {
        fi->type = 0;
    }
    else if(!strcasecmp(type, "FolderCount"))
    {
        fi->type = 1;
    }
    else if(!strcasecmp(type, "FolderSize"))
    {
        fi->type = 2;
    }
}
#ifdef WIN32
static int folderinfo_collect_win32(unsigned char* root, folderinfo* fi)
{
    unsigned char *file = root;
    list_entry qbase = {0};
    list_entry_init(&qbase);

    while(file)
    {
        size_t fpsz = 0;
        WIN32_FIND_DATAW wfd = { 0 };
        void* fh = NULL;

        if(safe_flag_get(fi->stop) == 0)
        {
            fpsz = string_length(file);
            unsigned char* filtered = zmalloc(fpsz + 6);
            snprintf(filtered, fpsz + 6, "%s/*.*", file);

            unsigned short* filtered_uni = utf8_to_ucs(filtered);
            sfree((void**)&filtered);

            fh = FindFirstFileExW(filtered_uni, FindExInfoBasic, &wfd, FindExSearchNameMatch, NULL, 2);
            sfree((void**)&filtered_uni);
        }

        do
        {
            if(safe_flag_get(fi->stop) || fh == INVALID_HANDLE_VALUE)
            {
                break;
            }

            unsigned char* res = ucs_to_utf8(wfd.cFileName, NULL, 0);

            if(!strcasecmp(res, ".") || !strcasecmp(res, ".."))
            {
                sfree((void**)&res);
                continue;
            }

            if(fi->hiddenf && (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
            {
                fi->file_count++;

                if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    fi->folder_count++;
                }

                fi->size += file_size(wfd.nFileSizeLow, wfd.nFileSizeHigh);
            }

            if(fi->systemf && (wfd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM))
            {
                fi->file_count++;

                if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    fi->fochar *str_sid = recycler_query_user_sid(&sid_len);lder_count++;
                }

                fi->size += file_size(wfd.nFileSizeLow, wfd.nFileSizeHigh);
            }

            if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {

                if(!(wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) && !(wfd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM))
                {
                    fi->folder_count++;
                }

                if(fi->recurse)
                {
                    size_t res_sz = string_length(res);
                    unsigned char* ndir = zmalloc(res_sz + fpsz + 2);
                    snprintf(ndir, res_sz + fpsz + 2, "%s/%s", file, res);
                    folderinfo_dir_list *fdl = zmalloc(sizeof(folderinfo_dir_list));
                    list_entry_init(&fdl->current);
                    linked_list_add(&fdl->current, &qbase);
                    fdl->dir = ndir;
                    uniform_slashes(ndir);
                }
            }

            if(!(wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) && !(wfd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) &&
                    !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                fi->file_count++;
                fi->size += file_size(wfd.nFileSizeLow, wfd.nFileSizeHigh);
            }

            sfree((void**)&res);

        }
        while(safe_flag_get(fi->stop) == 0 && FindNextFileW(fh, &wfd));

        if(fh != NULL && fh != INVALID_HANDLE_VALUE)
        {
            FindClose(fh);
            fh = NULL;
        }

        if(root != file)
        {
            sfree((void**)&file);
        }

        if(linked_list_empty(&qbase) == 0)
        {
            folderinfo_dir_list *fdl = element_of(qbase.prev, fdl, current);
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

    return(0);
}
#elif __linux__
static int folderinfo_collect_linux(unsigned char* root, folderinfo* fi)
{
    unsigned char *file = root;
    list_entry qbase = {0};
    list_entry_init(&qbase);

    while(file)
    {
        struct dirent *dir_entry = NULL;
        DIR *dir = NULL;
        size_t fpsz = 0;
        fpsz = string_length(file);

        dir = opendir(file);

        if(dir == NULL)
            continue;

        dir_entry = readdir(dir);

        do
        {

            unsigned char can_free = 1;
            if(safe_flag_get(fi->stop) || dir_entry == NULL)
            {
                break;
            }

            if(!strcasecmp(dir_entry->d_name, ".") || !strcasecmp(dir_entry->d_name, "..") ||
                    (dir_entry->d_type!= DT_REG && dir_entry->d_type != DT_DIR))
            {
                continue;
            }




            size_t res_sz = string_length(dir_entry->d_name);
            unsigned char* ndir = zmalloc(res_sz + fpsz + 2);
            snprintf(ndir, res_sz + fpsz + 2, "%s/%s", file, dir_entry->d_name);


            if(dir_entry->d_type == DT_REG)
            {
                if((fi->hiddenf && dir_entry->d_name[0] == '.') ||( dir_entry->d_name[0] != '.'))
                {
                    struct stat st;
                    if(!stat(ndir,&st))
                    {
                        fi->file_count++;
                        fi->size+=st.st_size;
                    }
                }
            }
            else if(dir_entry->d_type == DT_DIR)
            {
                if((fi->hiddenf && dir_entry->d_name[0] == '.') ||( dir_entry->d_name[0] != '.'))
                {
                    fi->folder_count++;

                    if(fi->recurse)
                    {
                        folderinfo_dir_list *fdl = zmalloc(sizeof(folderinfo_dir_list));
                        list_entry_init(&fdl->current);
                        linked_list_add(&fdl->current, &qbase);
                        fdl->dir = ndir;
                        uniform_slashes(ndir);
                        can_free = 0;
                    }
                }
            }

            if(can_free)
            {
                sfree((void**)&ndir);
            }

        }
        while(safe_flag_get(fi->stop) == 0 && (dir_entry = readdir(dir)));

        if(dir)
        {

            closedir(dir);
            dir = NULL;
        }

        if(root != file)
        {
            sfree((void**)&file);
        }

        if(linked_list_empty(&qbase) == 0)
        {
            folderinfo_dir_list *fdl = element_of(qbase.prev, fdl, current);
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

    return(0);
}
#endif
static void* folderinfo_collect_thread(void* vfi)
{
    folderinfo* fi = vfi;

    fi->file_count = 0;
    fi->folder_count = 0;
    fi->size = 0;
#ifdef WIN32
    folderinfo_collect_win32(fi->path, fi);
#elif __linux__
    folderinfo_collect_linux(fi->path,fi);
#endif
    fi->ofile_count = fi->file_count;
    fi->ofolder_count = fi->folder_count;
    fi->osize = fi->size;
    safe_flag_set(fi->working,0);
    return(NULL);
}

double folderinfo_update(void* spv)
{

    folderinfo* fi = spv;

    if(safe_flag_get(fi->working) == 0 && fi->parent == NULL && fi->th == 0)
    {
        int status = 0;
        safe_flag_set(fi->working, 1);

        status = pthread_create(&fi->th, NULL, folderinfo_collect_thread, fi);


        if(status)
        {
            safe_flag_set(fi->working, 0);
            diag_crit("%s %d Failed to start folderinfo_collect_thread. Status %x", __FUNCTION__, __LINE__, status);
        }
    }

    if(safe_flag_get(fi->working) == 0 && fi->th)
    {
        pthread_join(fi->th, NULL);
        fi->th = 0;
    }

    if(fi->parent)
    {
        fi->ofile_count = fi->parent->ofile_count;
        fi->ofolder_count = fi->parent->ofolder_count;
        fi->osize = fi->parent->osize;
    }

    switch(fi->type)
    {
        case 0:
            return ((double)fi->ofile_count);

        case 1:
            return ((double)fi->ofolder_count);

        case 2:
            return ((double)fi->osize);
    }

    return (0.0);
}

void folderinfo_destroy(void** spv)
{
    folderinfo* fi = *spv;
    safe_flag_set(fi->stop, 1);


    if(fi->th != 0)
    {
        pthread_join(fi->th, NULL);
    }

    safe_flag_destroy(&fi->stop);
    safe_flag_destroy(&fi->working);
    sfree((void**)&fi->path);
    sfree(spv);
}
