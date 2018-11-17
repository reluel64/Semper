/*
* Recycler monitor source
* Part of Project 'Semper'
* Written by Alexandru-Daniel Mărgărit
*/
#ifdef WIN32
#include <windows.h>
#include <shellapi.h>
#include <Sddl.h>
#include <io.h>
#endif
#include <sources/recycler/recycler.h>
#include <semper_api.h>
#include <mem.h>
#include <string_util.h>
#include <time.h>

#define string_length(s) (((s) == NULL ? 0 : strlen((s))))
#define RECYCLER_DIALOG_THREAD 0


static size_t recycler_get_time(void);
static void *recycler_query_thread(void *p);


int uniform_slashes(unsigned char *str)
{
    if(str == NULL)
    {
        return (-1);
    }

    for(size_t i = 0; str[i]; i++)
    {
#ifdef WIN32

        if(str[i] == '/')
        {
            str[i] = '\\';
        }

#elif __linux__

        if(str[i] == '\\')
        {
            str[i] = '/';
        }

#endif

        if((str[i] == '/' || str[i] == '\\') && (str[i + 1] == '\\' || str[i + 1] == '/'))
        {
            for(size_t cpos = i; str[cpos]; cpos++)
            {
                str[cpos] = str[cpos + 1];
            }

            if(i != 0)
            {
                i--;
            }

            continue;
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
void recycler_init(void **spv, void *ip)
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

void recycler_reset(void *spv, void *ip)
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
#ifdef WIN32
            SetEvent(r->me); /*wake the thread if it is in monitoring mode*/
#endif
        }

        semper_safe_flag_set(r->kill, 1);
        pthread_join(r->qth, NULL);
        r->qth = 0;
#ifdef WIN32
        CloseHandle(r->me);
#endif
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

double recycler_update(void *spv)
{
    recycler *r = spv;
    double lret = 0.0;


    if(r->mon_mode == 0)
    {
        if(r->mh == NULL || recycler_notifier_check_win32(r))
        {
#ifdef WIN32
            recycler_notifier_destroy_win32(r);
            recycler_notifier_setup_win32(r);
#elif __linux__
#warning "NO LINUX"
#endif
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

void recycler_command(void *spv, unsigned char *cmd)
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
#ifdef WIN32
                SHEmptyRecycleBin(NULL, NULL, 0);
#endif
#endif
            }

            else if(!strcasecmp("EmptySilent", cmd))
            {
#ifdef WIN32
                SHEmptyRecycleBin(NULL, NULL, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
#endif
            }
        }
        else if(!strcasecmp("Open", cmd))
        {
#ifdef WIN32
            ShellExecuteW(NULL, L"open", L"explorer.exe", L"/N,::{645FF040-5081-101B-9F08-00AA002F954E}", NULL, SW_SHOW);
#endif
        }
    }
}

void recycler_destroy(void **spv)
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
#ifdef WIN32
    recycler_notifier_destroy_win32(r);
#elif __linux__
#warning "NO LINUX"
#endif
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
#ifdef WIN32
                recycler_notifier_check_win32(r);
#elif __linux__
#warning "NO LINUX"
#endif
            }
#ifdef WIN32
            recycler_notifier_destroy_win32(r);
            recycler_notifier_setup_win32(r);
#elif __linux__
#warning "NO LINUX"
#endif


        }
#ifdef WIN32
        recycler_query_user_win32(r, &val);
#elif __linux__
#warning "NO LINUX"
#endif
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


