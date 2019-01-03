#if defined(WIN32)
#include <windows.h>
#include <shellapi.h>
#include <Sddl.h>
#include <io.h>
#include <sources/recycler/recycler.h>
#include <string_util.h>
#include <semper_api.h>
#include <mem.h>
#include <watcher.h>
#include <semper.h>
#include <surface.h>
#include <sources/source.h>
static unsigned char *recycler_query_user_sid(size_t *len);

int recycler_notifier_setup_win32(recycler *r)
{
    source *s = r->ip;
    surface_data *sd = s->sd;
    control_data *cd = sd->cd;
    char buf[256] = {0};
    size_t sid_len = 0;
    char *str_sid = recycler_query_user_sid(&sid_len);
    recycler_common *rc = recycler_get_common();
    if(str_sid == NULL)
        return(-1);

    snprintf(buf + 1, 255, ":\\$Recycle.Bin\\%s", str_sid);

    for(char i = 'A'; i <= 'Z'; i++)
    {
        char root[] = {i, ':', '\\', 0};
        buf[0] = i;

        if(GetDriveTypeA(root) != 3)
        {
            continue;
        }

        void *tmh = watcher_init(buf,cd->eq,(event_handler)recycler_event_proc,r);

        if(tmh != INVALID_HANDLE_VALUE && tmh != NULL)
        {
            void *tmp = realloc(rc->mh, sizeof(void*) * (rc->mc + 1));

            if(tmp)
            {
                rc->mh = tmp;
                rc->mh[rc->mc++] = tmh;
            }
        }
    }

    LocalFree(str_sid);
    return(0);
}


static unsigned char *recycler_query_user_sid(size_t *len)
{
    void *hTok = NULL;
    char *str_sid = NULL;

    if(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hTok))
    {
        unsigned char *buf = NULL;
        unsigned long  token_sz = 0;
        GetTokenInformation(hTok, TokenUser, NULL, 0, &token_sz);

        if(token_sz)
            buf = (unsigned char*)LocalAlloc(LPTR, token_sz);


        if(GetTokenInformation(hTok, TokenUser, buf, token_sz, &token_sz))
        {
            ConvertSidToStringSid(((PTOKEN_USER)buf)->User.Sid, &str_sid);

            if(str_sid)
                *len = string_length(str_sid);
        }

        LocalFree(buf);
        CloseHandle(hTok);
    }

    return(str_sid);
}

/*In-house replacement for SHQueryRecycleBinW
 * That can be canceled at any time and can be customized*/
int recycler_query_user_win32(recycler *r)
{
    /*Get the user SID*/
    recycler_common *rc = recycler_get_common();
    size_t sid_len = 0;
    size_t file_count = 0;
    size_t size = 0;

    char *str_sid = recycler_query_user_sid(&sid_len);

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

            if(!semper_safe_flag_get(rc->kill))
            {
                fpsz = string_length(file);
                unsigned char* filtered = zmalloc(fpsz + 6);

                if(file == buf)
                    snprintf(filtered, fpsz + 6, "%s/$I*.*", file);
                else
                    snprintf(filtered, fpsz + 6, "%s/*.*", file);

                uniform_slashes(filtered);

                unsigned short* filtered_uni = semper_utf8_to_ucs(filtered);
                sfree((void**)&filtered);

                fh = FindFirstFileExW(filtered_uni, FindExInfoBasic, &wfd, FindExSearchNameMatch, NULL, 2);
                semper_free((void**)&filtered_uni);
            }

            do
            {
                if(semper_safe_flag_get(rc->kill) || fh == INVALID_HANDLE_VALUE)
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
                    uniform_slashes(s);
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
            while(!semper_safe_flag_get(rc->kill) && FindNextFileW(fh, &wfd));

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

    pthread_mutex_lock(&rc->mtx);

    rc->count=file_count;
    rc->size = size;

    pthread_mutex_unlock(&rc->mtx);
    return(1);
}
#endif
