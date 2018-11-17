#include <sources/recycler/recycler.h>

/*In-house replacement for SHQueryRecycleBinW
 * That can be canceled at any time and can be customized*/
#ifdef __linux__
int recycler_query_user_linux(recycler *r, double *val)
{
    /*Get the user SID*/

    size_t sid_len = 0;
    size_t file_count = 0;
    size_t size = 0;



    if(str_sid == NULL)
    {
        diag_error("%s %d Failed to get user SID", __FUNCTION__, __LINE__);
        return(-1);
    }

    char rroot[] = ":\\$Recycle.Bin\\";
    unsigned char *buf = zmalloc(sizeof(rroot) + sid_len + 3);
    snprintf(buf + 1, sizeof(rroot) + sid_len + 3, "%s%s", rroot, str_sid);

    for(char ch = 'A'; ch <= 'Z'; ch++)
    {
        buf[0] = ch;
        char root[] = {ch, ':', '\\', 0};

        if(GetDriveTypeA(root) != 3)
        {
            continue;
        }

        unsigned char *file = buf;
        list_entry qbase = {0};
        list_entry_init(&qbase);

        while(file)
        {
            size_t fpsz = 0;
            WIN32_FIND_DATAW wfd = { 0 };
            void* fh = NULL;

            if(!semper_safe_flag_get(r->kill))
            {
                fpsz = string_length(file);
                unsigned char* filtered = zmalloc(fpsz + 6);

                if(file == buf)
                    snprintf(filtered, fpsz + 6, "%s/$I*.*", file);
                else
                    snprintf(filtered, fpsz + 6, "%s/*.*", file);

                unsigned short* filtered_uni = semper_utf8_to_ucs(filtered);
                sfree((void**)&filtered);

                fh = FindFirstFileExW(filtered_uni, FindExInfoBasic, &wfd, FindExSearchNameMatch, NULL, 2);
                semper_free((void**)&filtered_uni);
            }

            do
            {
                if(semper_safe_flag_get(r->kill) || fh == INVALID_HANDLE_VALUE)
                    break;

                unsigned char* res = semper_ucs_to_utf8(wfd.cFileName, NULL, 0);

                if(!strcasecmp(res, ".") || !strcasecmp(res, ".."))
                {
                    sfree((void**)&res);
                    continue;
                }

                if(file == buf)
                {
                    char valid = 0;
                    size_t res_len = string_length(res);
                    unsigned char *s = zmalloc(res_len + fpsz + 6);
                    snprintf(s, res_len + fpsz + 6, "%s\\%s", file, res);
                    windows_slahses(s);
                    s[fpsz + 2] = 'R';

                    if(access(s, 0) == 0)
                    {
                        s[fpsz + 2] = 'I';
                        FILE *f = fopen(s, "rb");

                        if(f)
                        {
                            size_t sz = 0;
                            fseek(f, 8, SEEK_SET);
                            fread(&sz, sizeof(size_t), 1, f);
                            fclose(f);
                            size += sz;
                            valid = 1;
                        }
                    }

                    sfree((void**)&s);

                    if(valid == 0)
                    {
                        diag_warn("%s %d Entry %s is not valid", __FUNCTION__, __LINE__, res);
                        sfree((void**)&res);
                        continue;
                    }
                }

#if 0

                if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
#if 0

                    if(r->rq == recycler_query_size)
                    {
                        size_t res_sz = string_length(res);
                        unsigned char* ndir = zmalloc(res_sz + fpsz + 2);
                        snprintf(ndir, res_sz + fpsz + 2, "%s/%s", file, res);
                        recycler_dir_list *fdl = zmalloc(sizeof(recycler_dir_list));
                        list_entry_init(&fdl->current);
                        linked_list_add(&fdl->current, &qbase);
                        fdl->dir = ndir;
                        uniform_slashes(ndir);
                    }

#endif
                }
                else
                {

                    size += file_size(wfd.nFileSizeLow, wfd.nFileSizeHigh);
                }

#endif

                if(file == buf) //we only count what is in root
                {
                    file_count++;
                }

                sfree((void**)&res);

            }
            while(!semper_safe_flag_get(r->kill) && FindNextFileW(fh, &wfd));

            if(fh != NULL && fh != INVALID_HANDLE_VALUE)
            {
                FindClose(fh);
                fh = NULL;
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

    LocalFree(str_sid);
    sfree((void**)&buf);

    pthread_mutex_lock(&r->mtx);
    r->can_empty = (file_count != 0);

    switch(r->rq)
    {
        case recycler_query_items:
            *val = (double)file_count;
            break;

        case recycler_query_size:
            *val = (double)size;
            break;
    }

    pthread_mutex_unlock(&r->mtx);
    return(1);
}
#endif
