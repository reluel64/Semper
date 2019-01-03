#include <sources/recycler/recycler.h>
#include <dirent.h>
#include <sys/stat.h>
#include <mem.h>
#include <string_util.h>
#include <sources/source.h>
#include <surface.h>
#include <watcher.h>
#if defined(__linux__)


int recycler_notifier_setup_linux(recycler *r)
{
    source *s = r->ip;
    surface_data *sd = s->sd;
    control_data *cd = sd->cd;
    recycler_common *rc = recycler_get_common();

    char rroot[] = "$HOME/.local/share/Trash/files";
    unsigned char *buf = expand_env_var(rroot);

    rc->mh = zmalloc(sizeof(void*));
    rc->mc++;
    rc->mh[0] = watcher_init(buf,cd->eq,(event_handler)recycler_event_proc,r);
    sfree((void**)&buf);
    return(0);
}


int recycler_query_user_linux(recycler *r)
{
    recycler_common *rc = recycler_get_common();
    size_t file_count = 0;
    size_t size = 0;

    char rroot[] = "$HOME/.local/share/Trash/files";

    unsigned char *buf = expand_env_var(rroot);


    /*for(char ch = 'A'; ch <= 'Z'; ch++)*/
    {

        unsigned char *file = buf;
        list_entry qbase = {0};
        list_entry_init(&qbase);

        while(file)
        {
            size_t fpsz = string_length(file);
            struct dirent *dir = NULL;
            DIR *dh = opendir(file);
            char can_free = 1;
            if(dh!=NULL)
            {
                dir = readdir(dh);
            }

            do
            {
                if(safe_flag_get(rc->kill) || dir == NULL)
                    break;


                if(!strcasecmp(dir->d_name, ".") || !strcasecmp(dir->d_name, ".."))
                {

                    continue;
                }

                size_t res_sz = string_length(dir->d_name);
                unsigned char* ndir = zmalloc(res_sz + fpsz + 2);
                snprintf(ndir, res_sz + fpsz + 2, "%s/%s", file, dir->d_name);
                uniform_slashes(ndir);

                if(dir->d_type == DT_DIR)
                {
                    if(r->rq == recycler_query_size)
                    {
                        recycler_dir_list *fdl = zmalloc(sizeof(recycler_dir_list));
                        list_entry_init(&fdl->current);
                        linked_list_add(&fdl->current, &qbase);
                        fdl->dir = ndir;
                        can_free = 0;
                    }
                }
                else
                {
                    struct stat st;
                    if(!stat(ndir,&st))
                    {
                        size += st.st_size;
                    }
                }

                if(file == buf) //we only count what is in root
                {
                    file_count++;
                }

                if(can_free)
                {
                    sfree((void**)&ndir);
                }

            }
            while(!safe_flag_get(rc->kill) && (dir =  readdir(dh)));

            if(dh != NULL )
            {
                closedir(dh);
                dh = NULL;
            }

            if(buf != file)
            {
                sfree((void**)&file);
            }

            if(linked_list_empty(&qbase) == 0)
            {
                recycler_dir_list *fdl = element_of(qbase.prev, fdl, current);
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
    }

    sfree((void**)&buf);

    pthread_mutex_lock(&rc->mtx);

    rc->size = size;
    rc->count = file_count;

    pthread_mutex_unlock(&rc->mtx);
    return(1);
}
#endif
