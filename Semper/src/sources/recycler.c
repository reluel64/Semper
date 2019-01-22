/*
 * Recycler monitor source
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */
#if defined(WIN32)
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
#include <watcher.h>
#include <linked_list.h>
#define string_length(s) (((s) == NULL ? 0 : strlen((s))))
#define RECYCLER_DIALOG_THREAD 0



static void *recycler_query_thread(void *p);
static void recycler_notifier_destroy(recycler *r);
#if 0
static size_t file_size(size_t low, size_t high)
{
    return (low | (high << 32));
}
#endif


recycler_common *recycler_get_common(void)
{
    static recycler_common rc ={0};
    return(&rc);
}

void recycler_init(void **spv, void *ip)
{

    recycler_common *rc = recycler_get_common();

    if(rc->inst_count == 0)
    {

        diag_info("Initializing Recycler Common");
        pthread_mutexattr_t mutex_attr;
        pthread_mutexattr_init(&mutex_attr);
        pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE_NP);
        pthread_mutex_init(&rc->mtx, &mutex_attr);
        pthread_mutexattr_destroy(&mutex_attr);
        rc->kill = safe_flag_init();
        rc->tha = safe_flag_init();
        list_entry_init(&rc->children);
    }

    rc->inst_count++;

    diag_info("Initializing Recycler with context 0x%p", ip);
    recycler *r = zmalloc(sizeof(recycler));

    list_entry_init(&r->current);
    pthread_mutex_lock(&rc->mtx);
    linked_list_add(&r->current, &rc->children);
    pthread_mutex_unlock(&rc->mtx);
    r->ip = ip;
    recycler_event_proc(r);
    *spv = r;
}

void recycler_reset(void *spv, void *ip)
{

    recycler *r = spv;
    recycler_common *rc = recycler_get_common();
    unsigned char *temp = param_string("Type", EXTENSION_XPAND_ALL, ip, "Items");
    r->rq = recycler_query_items;

    if(temp && strcasecmp(temp, "Size") == 0)
    {
        r->rq = recycler_query_size;
    }

    pthread_mutex_lock(&rc->mtx);
    sfree((void**)&r->cq_cmd);
    temp = param_string("CompleteQuery", EXTENSION_XPAND_ALL, ip, NULL);
    if(temp)
    {
        r->cq_cmd = strdup(temp);
    }
    pthread_mutex_unlock(&rc->mtx);
}

double recycler_update(void *spv)
{
    recycler *r = spv;
    recycler_common *rc = recycler_get_common();
    double lret = 0.0;

    pthread_mutex_lock(&rc->mtx);

    if(r->rq == recycler_query_size)
    {
        lret = (double)rc->size;
    }
    else
    {
        lret = (double)rc->count;
    }

    pthread_mutex_unlock(&rc->mtx);

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
    recycler_common *rc = recycler_get_common();


    if(cmd && spv)
    {


        pthread_mutex_lock(&rc->mtx);
        char can_empty = rc->size!=0;
        pthread_mutex_unlock(&rc->mtx);
        if(can_empty)
        {
            if(!strcasecmp("Empty", cmd))
            {

#if RECYCLER_DIALOG_THREAD /*This is disabled as the event queue was redesigned and it no longer requires alertable states*/
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
#if defined(WIN32)
                SHEmptyRecycleBin(NULL, NULL, 0);
#endif
#endif
            }

            else if(!strcasecmp("EmptySilent", cmd))
            {
#if defined(WIN32)
                SHEmptyRecycleBin(NULL, NULL, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
#endif
            }
        }
        else if(!strcasecmp("Open", cmd))
        {
#if defined(WIN32)
            ShellExecuteW(NULL, L"open", L"explorer.exe", L"/N,::{645FF040-5081-101B-9F08-00AA002F954E}", NULL, SW_SHOW);
#endif
        }
    }
}

void recycler_destroy(void **spv)
{
    recycler *r = *spv;
    recycler_common *rc = recycler_get_common();

    pthread_mutex_lock(&rc->mtx);
    sfree((void**)&r->cq_cmd);
    linked_list_remove(&r->current);
    pthread_mutex_unlock(&rc->mtx);

    if(rc->inst_count>0)
        rc->inst_count--;

    if(rc->inst_count==0)
    {
        safe_flag_set(rc->kill, 1);

        if(rc->qth)
            pthread_join(rc->qth, NULL);

        for(size_t i= 0; i< rc->mc;i++)
        {
            watcher_destroy(&rc->mh[i]);
        }

        sfree((void**)&rc->mh);
        rc->mc = 0;
        safe_flag_destroy(&rc->tha);
        safe_flag_destroy(&rc->kill);
        pthread_mutex_destroy(&rc->mtx);
        memset(rc,0,sizeof(recycler_common));
    }

    sfree(spv);
}

static void *recycler_query_thread(void *p)
{
    recycler *r = p;
    recycler_common *rc = recycler_get_common();

    safe_flag_set(rc->tha, 2);

#if defined(WIN32)
    recycler_query_user_win32(r);
#elif defined(__linux__)
    recycler_query_user_linux(r);

#endif


    pthread_mutex_lock(&rc->mtx);
    recycler *cr = NULL;
    list_enum_part(cr,&rc->children,current)
    {
        send_command_ex(cr->ip, cr->cq_cmd,0, 1);
    }

    pthread_mutex_unlock(&rc->mtx);



    safe_flag_set(rc->tha, 0);
    safe_flag_set(rc->kill, 0);
    return (NULL);
}

int recycler_event_proc(recycler *r)
{

    recycler_common *rc = recycler_get_common();

    if(rc->inst_count == 0)
    {
        return(-1);
    }

    recycler_notifier_destroy(r);
#if defined(WIN32)

    recycler_notifier_setup_win32(r);
#elif defined(__linux__)
    recycler_notifier_setup_linux(r);
#endif



    if(safe_flag_get(rc->tha))
    {
        safe_flag_set(rc->kill,1);
    }

    if(rc->qth)
    {
        pthread_join(rc->qth,NULL);
        rc->qth = 0;
    }

    safe_flag_set(rc->kill,0);
    safe_flag_set(rc->tha,1);


    if(pthread_create(&rc->qth, NULL, recycler_query_thread, r) != 0)
    {
        safe_flag_set(rc->tha, 0);
    }

    return(0);
}

static void recycler_notifier_destroy(recycler *r)
{
    recycler_common *rc = recycler_get_common();

    for(size_t i = 0; i < rc->mc; i++)
    {
        if(rc->mh[i])
        {
            watcher_destroy(&rc->mh[i]);
        }
    }

    rc->mc = 0;
    sfree((void**)&rc->mh);
}

