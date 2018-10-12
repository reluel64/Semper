/*
 * Surface lister
 * Derrived from file_lister.c
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */

#include <sources/internal/_surface_lister.h>
#include <sources/source.h>
#include <surface.h>
#include <string_util.h>
#include <mem.h>
#include <semper_api.h>
#include <semper.h>
#include <linked_list.h>
#ifdef __linux__
#include <dirent.h>
#endif

typedef struct
{
    unsigned char* display_name;
    unsigned char dir;
    list_entry current;
} surface_lister_file_list;


typedef struct _surface_lister
{
    void* cd;
    struct _surface_lister* parent;
    size_t index;
    /*Parent related stuff*/
    unsigned char update;
    size_t items;
    size_t current_item;
    size_t child_count;
    size_t base_len;
    unsigned char* path;
    unsigned char* base_path; // this shoud not be freed
    void* ip;
    list_entry file_list;
    surface_lister_file_list *start;
} surface_lister;


static int surface_lister_collect(surface_lister* sl)
{
    sl->start = NULL;
    sl->items = 0;
    sl->current_item = 0;
    surface_lister_file_list *slfl = NULL;
    surface_lister_file_list *tslfl = NULL;
    list_enum_part_safe(slfl, tslfl, &sl->file_list, current)
    {
        linked_list_remove(&slfl->current);
        sfree((void**)&slfl->display_name);
        sfree((void**)&slfl);
    }
#ifdef WIN32
    size_t fpsz = string_length(sl->path);
    unsigned char* filtered = zmalloc(fpsz + 6);
    strcpy(filtered, sl->path);
    strcpy(filtered + fpsz, "/*.*");

    unsigned short* filtered_uni = utf8_to_ucs(filtered);
    sfree((void**)&filtered);

    WIN32_FIND_DATAW wfd = { 0 };

    void* fh = FindFirstFileW(filtered_uni, &wfd);
    sfree((void**)&filtered_uni);

    if(fh == INVALID_HANDLE_VALUE)
        return (-1);

#elif __linux__

    DIR *dh = opendir(sl->path);

    if(dh == NULL)
    {
        return(-1);
    }

    struct dirent *dat = readdir(dh);

    if(dat == NULL)
    {
        closedir(dh);
        return(-1);
    }

#endif

    do
    {

        unsigned char* temp = NULL;

#ifdef WIN32
        temp = ucs_to_utf8(wfd.cFileName, NULL, 0);
#elif __linux__

        temp = clone_string(dat->d_name);
#endif

        if(temp == NULL)
            break;


        if(!strcasecmp(temp, "..") || !strcasecmp(temp, ".") || !strcasecmp(temp, "Data"))
        {
            sfree((void**)&temp);
            continue;
        }

        sl->items++;
        surface_lister_file_list *slfl = zmalloc(sizeof(surface_lister_file_list));

        if(sl->start == NULL)
        {
            sl->start = slfl;
        }

        list_entry_init(&slfl->current);
        linked_list_add(&slfl->current, &sl->file_list);
        slfl->display_name = temp;
#ifdef WIN32
        slfl->dir = (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#elif __linux__
        slfl->dir = (dat->d_type == DT_DIR);
#endif

    }

#ifdef WIN32

    while(FindNextFileW(fh, &wfd));

#elif __linux__

    while((dat = readdir(dh)) != NULL);

#endif

#ifdef WIN32
    FindClose(fh);
#elif __linux__
    closedir(dh);
#endif
    return (0);
}


void surface_lister_init(void** spv, void* ip)
{
    surface_lister* sl = zmalloc(sizeof(surface_lister));
    *spv = sl;
    source* s = ip;
    surface_data* sd = s->sd;
    list_entry_init(&sl->file_list);
    sl->cd = sd->cd;
    sl->ip = ip;
}

void surface_lister_reset(void* spv, void* ip)
{
    surface_lister* sl = spv;
    control_data* cd = sl->cd;
    unsigned char* temp = param_string("Path", EXTENSION_XPAND_VARIABLES, ip, NULL);

    void* parent = get_parent(temp, ip);

    if(parent)
    {
        sl->index = param_size_t("ChildIndex", ip, 0);
        sl->parent = get_private_data(parent);
    }
    else
    {
        sl->child_count = param_size_t("ChildCount", ip, 0);
        sl->base_path = cd->surface_dir;
        sl->path = sl->base_path;
        sl->base_len = cd->surface_dir_length;
        sl->update = 1;
    }
}

double surface_lister_update(void* spv)
{
    surface_lister *sl = spv;

    if(sl->parent && sl->parent->start && sl->index < sl->parent->child_count)
    {
        surface_lister_file_list *slfl = sl->parent->start;
        size_t i = sl->index;

        for(; i; i--)
        {
            if(slfl->current.prev == &sl->parent->file_list)
            {
                return(0.0);
            }

            slfl = element_of(slfl->current.prev, slfl, current);
        }

        if(i == 0)
        {
            return(1.0);
        }
    }

#if 1
    else if(sl->parent == NULL && sl->update)
#else
    else if(sl->parent == NULL)
#endif
    {
        surface_lister_collect(spv);
        sl->update = 0;
    }

    return(0.0);
}

unsigned char *surface_lister_string(void* spv)
{
    surface_lister *sl = spv;

    if(sl->parent && sl->parent->start && sl->index < sl->parent->child_count)
    {
        surface_lister_file_list *slfl = sl->parent->start;
        size_t i = sl->index;

        for(; i; i--)
        {
            if(slfl->current.prev == &sl->parent->file_list)
            {
                return(NULL);
            }

            slfl = element_of(slfl->current.prev, slfl, current);
        }

        if(i == 0)
        {
            return(slfl->display_name);
        }
    }

    return(NULL);
}

void surface_lister_command(void* spv, unsigned char* command)
{

    surface_lister *sl = spv;

    if(sl->parent && sl->parent->start && command && sl->index < sl->parent->child_count)
    {
        surface_lister_file_list *slfl = sl->parent->start;
        surface_lister *parent = sl->parent;
        size_t i = sl->index;

        for(; i; i--)
        {
            if(slfl->current.prev == &sl->parent->file_list)
            {
                break;
            }

            slfl = element_of(slfl->current.prev, slfl, current);
        }

        if(i == 0)
        {
            if(strcasecmp("Open", command) == 0)
            {
                if(slfl->dir)
                {
                    size_t path_len = string_length(parent->path);
                    size_t dir_len = string_length(slfl->display_name);
                    unsigned char *temp = zmalloc(path_len + dir_len + 2); //space for null and for slash

                    snprintf(temp, path_len + dir_len + 2, "%s/%s", parent->path, slfl->display_name);

                    if(parent->path != parent->base_path)
                    {
                        sfree((void**)&parent->path);
                    }

                    parent->path = temp;
                    parent->update = 1;
                }
                else
                {
                    size_t path_len = string_length(parent->path + parent->base_len + 1);
                    size_t dir_len = string_length(slfl->display_name);
                    unsigned char *temp = zmalloc(15 + dir_len + path_len); //space for null and for slash
                    snprintf(temp, 15 + dir_len + path_len, "LoadSurface(%s,%s)", parent->path + parent->base_len + 1, slfl->display_name);
                    send_command(parent->ip, temp);
                    sfree((void**)&temp);
                }
            }

            else if(strcasecmp("Unload", command) == 0)
            {
                size_t path_len = string_length(parent->path + parent->base_len + 1);

                unsigned char *temp = zmalloc(18 + path_len); //space for null and for slash
                snprintf(temp, 18 + path_len, "unLoadSurface(%s)", parent->path + parent->base_len + 1);
                send_command(parent->ip, temp);
                sfree((void**)&temp);
            }
        }
    }
    else if(sl->parent == NULL && command)
    {
        if(strcasecmp("Back", command) == 0 && strcasecmp(sl->path, sl->base_path))
        {
            unsigned char *lo = strrchr(sl->path, '/');

            if(sl->path != sl->base_path && strncasecmp(sl->path, sl->base_path, lo - sl->path))
            {

                sfree((void**)&sl->path);
                sl->path = sl->base_path;
            }
            else
            {
                lo[0] = 0;
            }

            sl->update = 1;
        }
        else if(sl->start)
        {

            if(strcasecmp("Up", command) == 0)
            {
                if(sl->start->current.next != &sl->file_list)
                {
                    if(sl->current_item)
                        sl->current_item--;

                    sl->start = element_of(sl->start->current.next, sl->start, current);
                }
            }
            else if(strcasecmp("Down", command) == 0 && sl->items - sl->current_item > sl->child_count)
            {
                if(sl->start->current.prev != &sl->file_list)
                {
                    sl->current_item++;
                    sl->start = element_of(sl->start->current.prev, sl->start, current);
                }
            }
        }
    }
}
void surface_lister_destroy(void** spv)
{
    surface_lister *sl = *spv;

    if(sl->parent == NULL)
    {
        surface_lister_file_list *slfl = NULL;
        surface_lister_file_list *tslfl = NULL;
        list_enum_part_safe(slfl, tslfl, &sl->file_list, current)
        {
            linked_list_remove(&slfl->current);
            sfree((void**)&slfl->display_name);
            sfree((void**)&slfl);
        }

        if(sl->base_path != sl->path)
        {
            sfree((void**)&sl->path);
        }
    }

    sfree(spv);
}
