/*
 * Event queueing routines
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */

#include <semper.h>
#include <mem.h>
#include <event.h>
#include <pthread.h>
#ifdef __linux__
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <poll.h>
#include <limits.h>

typedef struct
{
    event_queue *eq;
    timer_t tid;
    event *evt;
} event_trigger_id;

#endif


#ifdef WIN32
static void event_trigger(event_queue* eq)
#elif __linux__
static void event_trigger(int sig,siginfo_t *inf,void* pv)
#endif
{
#ifdef WIN32
    if(eq)
    {
        SetEvent(eq->loop_event);
    }
#elif __linux__
    if(pv&&inf==NULL)
    {
        event_queue *eq=pv;
        eventfd_write(((int*)eq->loop_event)[0],10000);// such event, much magic
    }
    else if(inf!=NULL)
    {
        event_queue *eq=inf->si_value.sival_ptr;
        if(eq)
        {
            eventfd_write(((int*)eq->loop_event)[0],10000);// such event, much magic
        }
    }
#endif
}

void event_wait(event_queue* eq)
{
#ifdef WIN32

    if(eq->loop_event == NULL)
    {
        eq->loop_event = CreateEventW(NULL, 0, 1, NULL);
    }

    MsgWaitForMultipleObjectsEx(1, &eq->loop_event, -1, QS_ALLEVENTS, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);

#elif __linux__

    struct pollfd p[3]= {0};

    eventfd_t dummy;
    p[0].events=POLLIN|POLL_OUT;
    p[1].events=POLLIN|POLL_OUT|POLL_MSG;
    p[2].events=POLLIN;

    p[0].fd=((int*)eq->loop_event)[0];
    p[1].fd=((int*)eq->loop_event)[1];
    p[2].fd=((int*)eq->loop_event)[2]; //for the watcher we will consume the events later

    poll(p,3,-1);

    if(p[0].revents)
    {
        eventfd_read(((int*)eq->loop_event)[0],&dummy); //consume the event
    }
#endif
}


#ifdef __linux__
void event_queue_set_window_event(event_queue *eq,int fd)
{
    ((int*)eq->loop_event)[1]=fd;
}

void event_queue_set_inotify_event(event_queue *eq,int fd)
{
    ((int*)eq->loop_event)[2]=fd;
}
#endif


event_queue* event_queue_init(void)
{
    event_queue* eq = zmalloc(sizeof(event_queue));

    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&eq->mutex,&mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);
    list_entry_init(&eq->events);

#ifdef __linux__

    struct sigaction sa= {0};
    sigset_t sigs;
    sigemptyset(&sigs);
    sigaddset(&sigs,SIGALRM);
    //  sigprocmask(SIG_BLOCK,&sigs,NULL);
    sa.sa_handler=NULL;
    sa.sa_sigaction=event_trigger;
    sa.sa_flags= SA_SIGINFO;
    //sa.sa_mask=sigs;
    sigaction(SIGALRM,&sa,NULL);
    eq->loop_event=zmalloc(sizeof(int)*3);
    ((int*)eq->loop_event)[0]=eventfd(2401, EFD_NONBLOCK);
#endif
    return (eq);
}

void event_remove(event_queue* eq, event_handler eh, void* pv, unsigned char flags)
{
    pthread_mutex_lock(&eq->mutex);
    event* e = NULL;

    list_enum_part(e, &eq->events, current)
    {
        switch(flags&EVENT_REMOVE_BY_DATA_HANDLER)
        {
        case EVENT_REMOVE_BY_DATA_HANDLER:
            if(e->handler == eh && e->pv == pv)
            {
                e->handler = NULL;
                if(e->timer)
                {
#ifdef WIN32
                    CloseHandle(e->timer);
#elif __linux__
                    if(e->timer)
                    {
                        timer_delete(e->timer);
                    }
#endif
                    e->timer = NULL;
                }
            }
            break;
        case EVENT_REMOVE_BY_DATA:
            if(e->pv == pv)
            {
                e->handler = NULL;
                if(e->timer)
                {
#ifdef WIN32
                    CloseHandle(e->timer);
#elif __linux__
                    if(e->timer)
                    {
                        timer_delete(e->timer);
                    }
#endif
                    e->timer = NULL;
                }
            }
            break;
        case EVENT_REMOVE_BY_HANDLER:
            if(e->handler == eh)
            {
                e->handler = NULL;
                if(e->timer)
                {
#ifdef WIN32
                    CloseHandle(e->timer);
#elif __linux__
                    if(e->timer)
                    {
                        timer_delete(e->timer);
                    }
#endif
                    e->timer = NULL;
                }
            }
            break;
        }
    }

    pthread_mutex_unlock(&eq->mutex);
}


event* event_push(event_queue* eq, event_handler handler, void* pv, size_t timeout, unsigned char flags)
{

    if(flags & EVENT_REMOVE_BY_DATA_HANDLER)
    {
        event_remove(eq, handler, pv, flags);
    }

    pthread_mutex_lock(&eq->mutex);

    if(eq->ce&&flags==0)
    {
        if(eq->ce->handler==handler&&eq->ce->pv==pv) //defer the event until the next cycle
        {
            flags|=EVENT_PUSH_TAIL;
        }
    }
    pthread_mutex_unlock(&eq->mutex);
    timeout=(timeout<16?16:timeout); //16 ms seems the resolution liked by Windows so we will use that as a limitation for both Windows and Linux
    event* e = zmalloc(sizeof(event));
    list_entry_init(&e->current);


    e->handler = handler;
    e->pv = pv;

    if(flags & EVENT_PUSH_TIMER)
    {

#ifdef WIN32

        e->timer = CreateWaitableTimer(NULL, 1, NULL);
        LARGE_INTEGER li = { 0 };
        li.QuadPart = -10000 * (long)timeout;
        SetWaitableTimer(e->timer, &li, 0, (PTIMERAPCROUTINE)event_trigger, eq, 0);

#elif __linux__
        struct itimerspec tval= {0};
        struct sigevent ev= {0};

        tval.it_value.tv_sec=timeout/1000;
        tval.it_value.tv_nsec=1000000*(timeout-(tval.it_value.tv_sec*1000));

        memset(&ev,0,sizeof(struct sigevent));
        ev.sigev_notify=SIGEV_SIGNAL;
        ev.sigev_signo=SIGALRM;
        ev.sigev_value.sival_ptr=eq;//this little prick gave me about 2 hours of headaches
        timer_create(CLOCK_MONOTONIC,&ev,(timer_t*)&e->timer);
        timer_settime((timer_t)e->timer,0,&tval,NULL);
#endif

    }

    pthread_mutex_lock(&eq->mutex);

    if(flags & EVENT_PUSH_TAIL)
    {
        linked_list_add_last(&e->current, &eq->events);
    }
    else
    {
        linked_list_add(&e->current, &eq->events);
    }

    if(!(flags&EVENT_PUSH_TIMER))
    {
#ifdef WIN32
        event_trigger(eq);
#elif __linux__
        event_trigger(0,NULL,eq);
#endif
    }
    pthread_mutex_unlock(&eq->mutex);

    return (e);
}

static void event_remove_internal(event* e)
{
    if(e)
    {
        if(e->current.next && e->current.prev)
        {
            linked_list_remove(&e->current);
        }
        if(e->timer)
        {
#ifdef WIN32
            CloseHandle(e->timer);
#elif __linux__
            timer_delete(e->timer);
#endif
            e->timer=NULL;
        }
        sfree((void**)&e);
    }
}

void event_queue_clear(event_queue* eq)
{
    pthread_mutex_lock(&eq->mutex);
    event* e = NULL;
    event* te = NULL;
    list_enum_part_safe(e, te, &eq->events, current)
    {
        event_remove_internal(e);
    }
    pthread_mutex_unlock(&eq->mutex);
}

void event_queue_destroy(event_queue** eq)
{
    if(*eq)
    {
        event_queue_clear(*eq);
        pthread_mutex_destroy(&(*eq)->mutex);
#ifdef WIN32
        CloseHandle((*eq)->loop_event);
#elif __linux__
        close(((int*)(*eq)->loop_event)[0]);
        sfree((void**)&(*eq)->loop_event);
#endif
        sfree((void**)eq);

    }
}

void event_process(event_queue* eq)
{
    event* e = NULL;
    event* te = NULL;

    list_enum_part_backward_safe(e, te, &eq->events, current)
    {
        unsigned char pe = 1;

        if(e->timer)
        {
#ifdef WIN32
            pe = WaitForSingleObject(e->timer, 0) == 0; //just check the status of the timer
#elif __linux__

            struct itimerspec void_tim= {0};
            struct itimerspec ct= {0};
            timer_gettime(e->timer,&ct);
            pe=  (memcmp(&void_tim,&ct,sizeof(struct itimerspec))==0);
#endif
        }

        if(pe)
        {

            pthread_mutex_lock(&eq->mutex);
            eq->ce=e;
            pthread_mutex_unlock(&eq->mutex);
            if(e->handler)
            {
                e->handler(e->pv);
            }
            pthread_mutex_lock(&eq->mutex);
            event_remove_internal(e);
            pthread_mutex_unlock(&eq->mutex);
        }
    }
    pthread_mutex_lock(&eq->mutex);
    eq->ce=NULL;
    pthread_mutex_unlock(&eq->mutex);
}
