/*
* Recycler monitor source
* Part of Project 'Semper'
* Written by Alexandru-Daniel Mărgărit
*/
#ifdef WIN32
#include <pthread.h>
#include <windows.h>
#include <stdio.h>
#include <shellapi.h>
#include <pthread.h>
#include <linked_list.h>
#include <Sddl.h>
#include <io.h>
#include <time.h>

#define string_length(s) (((s) == NULL ? 0 : strlen((s))))
#define RECYCLER_DIALOG_THREAD 0


typedef enum
{
    recycler_query_items,
    recycler_query_size
} recycler_query_t;


typedef struct
{
    void *ip;
    void **mh;                  //monitor handle
    void *me;                   //monitor event
    double inf;                 //the value from the query
    size_t mc;                  //count of monitors
    size_t lc;                  //last change
    size_t cc;                  //current change
    pthread_t qth;              //query thread
    pthread_mutex_t mtx;
    recycler_query_t rq;       //query type
    void *kill;
    void *tha;
    unsigned char can_empty;
    unsigned char mon_mode;    //monitoring mode
    unsigned char *cq_cmd;     //command when query is completed
} recycler;

typedef struct
{
    unsigned char *dir;
    list_entry current;
} recycler_dir_list;


static int recycler_notifier_check(recycler *r);
static void *recycler_query_thread(void *p);
static int recycler_query_user(recycler *r, double *val);
static unsigned char *recycler_query_user_sid(size_t *len);
static void recycler_notifier_destroy(recycler *r);
static int recycler_notifier_setup(recycler *r);
static size_t recycler_get_time(void);


static void *zmalloc(size_t m)
{
    void *p = malloc(m);

    if(p)
    {
        memset(p, 0, m);
        return(p);
    }

    return(NULL);
}

static void sfree(void **p)
{
    if(p)
    {
        free(*p);
        *p = NULL;
    }
}

static int windows_slahses(unsigned char* s)
{
    if(s == NULL)
    {
        return (-1);
    }

    for(size_t i = 0; s[i]; i++)
    {
        if(s[i] == '/')
        {
            s[i] = '\\';
        }
    }

    return (0);
}

#if 0
static size_t file_size(size_t low, size_t high)
{
    return (low | (high << 32));
}
#endif
void init(void **spv, void *ip)
{

    diag_info("Initializing Recycler with context 0x%p", ip);
    recycler *r = zmalloc(sizeof(recycler));
    r->ip = ip;
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&r->mtx, &mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);
    r->kill = semper_safe_flag_init();
    r->tha = semper_safe_flag_init();
    *spv = r;
}

void reset(void *spv, void *ip)
{
    recycler *r = spv;
    pthread_mutex_lock(&r->mtx);
    unsigned char *temp = param_string("Type", EXTENSION_XPAND_ALL, ip, "Items");
    r->rq = recycler_query_items;

    if(temp && strcasecmp(temp, "Size") == 0)
    {
        r->rq = recycler_query_size;
    }

    sfree((void**)&r->cq_cmd);

    r->cq_cmd = strdup(param_string("CompleteQuery", EXTENSION_XPAND_ALL, ip, NULL));

    pthread_mutex_unlock(&r->mtx);

    /*Check if there is a monitor running and if it is, try to stop it*/
    char new_monitor = param_size_t("MonitorMode", ip, 0) != 0;

    if(r->mon_mode > new_monitor && r->qth)
    {
        r->mon_mode = new_monitor;

        if(r->me)
        {
            SetEvent(r->me); /*wake the thread if it is in monitoring mode*/
        }

        semper_safe_flag_set(r->kill, 1);
        pthread_join(r->qth, NULL);
        r->qth = 0;
        CloseHandle(r->me);
    }

    if(r->mon_mode < new_monitor && r->me == NULL)
    {
        r->me = CreateEvent(NULL, 0, 0, NULL);
        r->mon_mode = new_monitor;
        semper_safe_flag_set(r->tha, 1);

        if(pthread_create(&r->qth, NULL, recycler_query_thread, r) != 0)
        {
            semper_safe_flag_set(r->tha, 0);
        }
    }
}

double update(void *spv)
{
    recycler *r = spv;
    double lret = 0.0;


    if(r->mon_mode == 0)
    {
        if(r->mh == NULL || recycler_notifier_check(r))
        {
            recycler_notifier_destroy(r);
            recycler_notifier_setup(r);
            r->cc++;
        }

        if(r->lc != r->cc && r->qth == 0)
        {
            r->lc = r->cc;
            semper_safe_flag_set(r->tha, 1);

            if(pthread_create(&r->qth, NULL, recycler_query_thread, r) != 0)
            {
                semper_safe_flag_set(r->tha, 0);
            }
        }

        if(semper_safe_flag_get(r->tha) == 0 && r->qth)
        {
            pthread_join(r->qth, NULL);
            r->qth = 0;
        }
    }

    lret = r->inf;

    return(lret);
}

#if RECYCLER_DIALOG_THREAD

static void *dialog_thread(void *p)
{
    SHEmptyRecycleBin(NULL, NULL, 0);
    return(NULL);
}

#endif

void command(void *spv, unsigned char *cmd)
{
    recycler *r = spv;

    if(cmd && spv)
    {
        if(r->can_empty)
        {
            if(!strcasecmp("Empty", cmd))
            {

#if RECYCLER_DIALOG_THREAD
                /*Shitty workaround as the SHEmptyRecycleBin() will put the thread
                 * in a non-alertable state which will cause the event queue to stall while
                 * the dialog is up
                 * This issue could be handled in two ways:
                 * 1) Start a separate thread to handle the dialog - this is actually implemented
                 * 2) Redesign the event processing mechanism - hell no*/
                pthread_t dummy;
                pthread_attr_t th_att = {0};
                pthread_attr_init(&th_att);
                pthread_attr_setdetachstate(&th_att, PTHREAD_CREATE_DETACHED);
                pthread_create(&dummy, &th_att, dialog_thread, spv);
                pthread_attr_destroy(&th_att);
#else
                SHEmptyRecycleBin(NULL, NULL, 0);
#endif
            }

            else if(!strcasecmp("EmptySilent", cmd))
            {
                SHEmptyRecycleBin(NULL, NULL, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
            }
        }
        else if(!strcasecmp("Open", cmd))
        {
            ShellExecuteW(NULL, L"open", L"explorer.exe", L"/N,::{645FF040-5081-101B-9F08-00AA002F954E}", NULL, SW_SHOW);
        }
    }
}

void destroy(void **spv)
{
    recycler *r = *spv;
    semper_safe_flag_set(r->kill, 1);

    if(r->me)
    {
        SetEvent(r->me);
        CloseHandle(r->me);
    }

    if(r->qth)
        pthread_join(r->qth, NULL);

    semper_safe_flag_destroy(&r->tha);
    semper_safe_flag_destroy(&r->kill);
    pthread_mutex_destroy(&r->mtx);
    sfree((void**)&r->cq_cmd);
    recycler_notifier_destroy(r);
    sfree(spv);
}



static void *recycler_query_thread(void *p)
{

    char first = 0;
    recycler *r = p;
    time_t start = recycler_get_time();
    semper_safe_flag_set(r->tha, 2);

    do
    {
        time_t diff = start;
        double val = 0.0;

        if(r->mon_mode)
        {
            if(first != 0)
            {
                recycler_notifier_check(r);
            }

            recycler_notifier_destroy(r);
            recycler_notifier_setup(r);

        }

        recycler_query_user(r, &val);

        if(r->mon_mode)
            diff = recycler_get_time() - start;

        /*In monitoring mode, without this condition (val!=r->inf), the monitoring thread might flood the main event queue
         * which would lead to sluggish response of the application and/or undefined results from other
         * sources that are time sensitive (CPU, Network, etc)
         * This behaviour could occur when deleting huge amounts of small files or if
         * ghost events (1) are reported.
         *
         * (1) the notifier detects a change but it does not exist on the disk
         * */
        if(val != r->inf)
        {
            pthread_mutex_lock(&r->mtx);

            if(diff >= 1000 || !r->mon_mode)
                send_command(r->ip, r->cq_cmd);
            else if(r->mon_mode)
                send_command_ex(r->ip, r->cq_cmd, 1000 - diff, 1);


            r->inf = val;
            pthread_mutex_unlock(&r->mtx);

        }

        if(first == 0)
            first = 1;
    }
    while(r->mon_mode && !semper_safe_flag_get(r->kill));

    semper_safe_flag_set(r->tha, 0);
    semper_safe_flag_set(r->kill, 0);
    return (NULL);
}


static void recycler_notifier_destroy(recycler *r)
{
    for(size_t i = 0; i < r->mc; i++)
    {
        if(r->mh[i] && r->mh[i] != INVALID_HANDLE_VALUE)
        {
            FindCloseChangeNotification(r->mh[i]);
        }
    }

    r->mc = 0;
    sfree((void**)&r->mh);
}

static int recycler_notifier_setup(recycler *r)
{
    char buf[256] = {0};
    size_t sid_len = 0;
    char *str_sid = recycler_query_user_sid(&sid_len);

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

        void *tmh = FindFirstChangeNotificationA(buf, 0, 0x1 | 0x2 | 0x4 | 0x8 | 0x10);

        if(tmh != INVALID_HANDLE_VALUE && tmh != NULL)
        {
            void *tmp = realloc(r->mh, sizeof(void*) * (r->mc + 1));

            if(tmp)
            {
                r->mh = tmp;
                r->mh[r->mc++] = tmh;
            }
        }
    }

    if(r->mon_mode)
    {
        void *tmp = realloc(r->mh, sizeof(void*) * (r->mc + 1));

        if(tmp)
        {
            r->mh = tmp;
            r->mh[r->mc] = r->me;
        }
    }

    LocalFree(str_sid);
    return(0);
}

static int recycler_notifier_check(recycler *r)
{
    if(r->mon_mode)
    {
        WaitForMultipleObjects(r->mc + 1, r->mh, 0, -1);
    }
    else
    {
        for(size_t i = 0; i < r->mc; i++)
        {
            if(r->mh[i] && r->mh[i] != INVALID_HANDLE_VALUE)
            {
                if(WaitForSingleObject(r->mh[i], 0) == 0)
                    return(1);
            }
        }
    }

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
static int recycler_query_user(recycler *r, double *val)
{
    /*Get the user SID*/

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
                        windows_slahses(ndir);
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
                recycler_dir_list *fdl = element_of(qbase.prev, recycler_dir_list, current);
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

static size_t recycler_get_time(void)
{
#ifdef WIN32
    return(clock());
#elif __linux__
    struct timespec t = {0};
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    return(t.tv_sec * 1000 + t.tv_nsec / 1000000);
#endif
}

#endif
