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
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <poll.h>
#include <limits.h>
#endif


static void event_start_processing(event_queue* eq)
{
#ifdef WIN32

    if(eq)
    {
        SetEvent(eq->loop_event);
    }

#elif __linux__

    if(eq)
    {
        eventfd_write((int)(size_t)eq->loop_event, 0x3010);
    }

#endif
}

#ifdef __linux__
static void event_start_processing_sig_wrap(union sigval sig)
{
    if(sig.sival_ptr != NULL)
    {
        event_queue *eq = sig.sival_ptr;
        event_start_processing(eq);
    }
}
#endif

int event_add_wait(event_queue *eq, event_wait_handler ewh, void *pv, void *wait, unsigned int flags)
{
    if(eq == NULL || ewh == NULL)
        return(-1);

    event_waiter *ew = zmalloc(sizeof(event_waiter));

    if(ew)
    {
        ew->ewh = ewh;
        ew->flags = flags;
        ew->pv = pv;
        ew->wait = wait;
        list_entry_init(&ew->current);
        pthread_mutex_lock(&eq->mutex);
        linked_list_add(&ew->current, &eq->waiters);
        pthread_mutex_unlock(&eq->mutex);
    }
    else
    {
        return(-1);
    }

    return(0);
}

int event_remove_wait(event_queue *eq, void *wait)
{
    if(eq == NULL || wait == NULL)
        return(-1);

    event_waiter *ew = NULL;
    event_waiter *tew = NULL;
    list_enum_part_safe(ew, tew, &eq->waiters, current)
    {
        if(ew->wait == wait || wait == (void*) - 1)
        {
#ifdef WIN32
            UnregisterWaitEx(ew->mon_th, INVALID_HANDLE_VALUE);
#endif
            pthread_mutex_lock(&eq->mutex);
            linked_list_remove(&ew->current);
            pthread_mutex_unlock(&eq->mutex);
            sfree((void**)&ew);
        }
    }
    return(0);
}

unsigned char event_wait(event_queue* eq)
{

    unsigned char event_p = 0;
    event_waiter *ew = NULL;
#ifdef WIN32


    list_enum_part(ew, &eq->waiters, current)
    {
        if(ew->mon_th == NULL)
            RegisterWaitForSingleObject(&ew->mon_th, ew->wait, (WAITORTIMERCALLBACK)event_start_processing, eq, -1, WT_EXECUTEONLYONCE);
    }

    if(MsgWaitForMultipleObjectsEx(1, &eq->loop_event, -1, QS_ALLEVENTS, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE) == WAIT_OBJECT_0)
    {
        event_p = 1;
    }

    list_enum_part(ew, &eq->waiters, current)
    {
        if(ew->mon_th != NULL || ew->mon_th != INVALID_HANDLE_VALUE)
        {
            UnregisterWaitEx(ew->mon_th, INVALID_HANDLE_VALUE);
        }
        ew->mon_th = NULL;

        if(ew->wait == NULL || WaitForSingleObject(ew->wait, 0) == 0)
            ew->ewh(ew->pv, ew->wait);
    }

#elif __linux__
    /*build wait structures*/

    eventfd_t dummy;
    size_t evts = 1;
    struct pollfd *events = zmalloc(sizeof(struct pollfd));
    events[0].fd = (int)(size_t)eq->loop_event;
    events[0].events = POLLIN;

    list_enum_part(ew, &eq->waiters, current)
    {
        if(ew->wait == NULL && ew->wait == (void*) - 1)
            continue;

        struct pollfd *temp = realloc(events, (evts + 1) * sizeof(struct pollfd));


        if(temp)
        {
            events = temp;
            events[evts].fd = (int)(size_t)ew->wait;
            events[evts].events = ew->flags;
            events[evts].revents = 0;
            evts++;
        }
    }


    if(events)
    {
        poll(events, evts, -1);
    }


    if(events[0].revents)
    {
        eventfd_read((int)(size_t)eq->loop_event, &dummy); //consume the event
        event_p = 1;
    }

    list_enum_part(ew, &eq->waiters, current)
    {
        if(ew->wait != 0 && ew->wait != (void*) - 1)
        {
            for(size_t i = 1; i < evts; i++)
            {
                if(events[i].fd == (int)(size_t)ew->wait && events[i].revents)
                {
                    ew->ewh(ew->pv, ew->wait);
                    break;
                }
            }
        }
        else
        {
            ew->ewh(ew->pv, ew->wait);
        }
    }

    sfree((void**)&events);

#endif
    return(event_p);
}

event_queue* event_queue_init(void)
{
    event_queue* eq = zmalloc(sizeof(event_queue));
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&eq->mutex, &mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);
    list_entry_init(&eq->events);
    list_entry_init(&eq->waiters);

#ifdef WIN32
    eq->loop_event = CreateEvent(NULL, 0, 1, NULL);
#elif __linux__
    eq->loop_event = (void*)(size_t)eventfd(0x2712, EFD_NONBLOCK);
#endif
    return (eq);
}

void event_remove(event_queue* eq, event_handler eh, void* pv, unsigned char flags)
{
    pthread_mutex_lock(&eq->mutex);
    event* e = NULL;

    list_enum_part(e, &eq->events, current)
    {
        unsigned char match = 0;

        switch(flags & EVENT_REMOVE_BY_DATA_HANDLER)
        {
            case EVENT_REMOVE_BY_DATA_HANDLER:
                if(e->handler == eh && e->pv == pv)
                    match = 1;

                break;

            case EVENT_REMOVE_BY_DATA:
                if(e->pv == pv)
                    match = 1;

                break;

            case EVENT_REMOVE_BY_HANDLER:
                if(e->handler == eh)
                    match = 1;

                break;
        }

        if(match)
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
    }

    pthread_mutex_unlock(&eq->mutex);
}

int event_push(event_queue* eq, event_handler handler, void* pv, size_t timeout, unsigned char flags)
{
    if(flags & EVENT_REMOVE_BY_DATA_HANDLER)
    {
        event_remove(eq, handler, pv, flags);
    }

    pthread_mutex_lock(&eq->mutex);

    if(eq->ce && flags == 0)
    {
        if(eq->ce->handler == handler && eq->ce->pv == pv) //defer the event until the next cycle to avoid a busyloop
        {
            flags |= EVENT_PUSH_TAIL;
        }
    }

    pthread_mutex_unlock(&eq->mutex);
    timeout = (timeout >= 16 ? timeout : 16);
    event* e = zmalloc(sizeof(event));
    list_entry_init(&e->current);

    e->handler = handler;
    e->pv = pv;

    if((flags & EVENT_PUSH_TIMER) && !(flags & EVENT_PUSH_HIGH_PRIO))
    {

#ifdef WIN32

        e->timer = CreateWaitableTimer(NULL, 1, NULL);
        LARGE_INTEGER li = { 0 };
        li.QuadPart = -10000 * (long)timeout;
        SetWaitableTimer(e->timer, &li, 0, (PTIMERAPCROUTINE)event_start_processing, eq, 0);

#elif __linux__

        struct itimerspec tval = {0};
        struct sigevent ev = {0};

        tval.it_value.tv_sec = timeout / 1000;
        tval.it_value.tv_nsec = 1000000 * (timeout - (tval.it_value.tv_sec * 1000));

        memset(&ev, 0, sizeof(struct sigevent));
        ev.sigev_notify = SIGEV_THREAD;
        ev.sigev_notify_function = event_start_processing_sig_wrap;
        ev.sigev_value.sival_ptr = eq;

        timer_create(CLOCK_MONOTONIC, &ev, (timer_t*)&e->timer);
        timer_settime((timer_t)e->timer, 0, &tval, NULL);
#endif
    }



    pthread_mutex_lock(&eq->mutex);

    if(flags & EVENT_PUSH_HIGH_PRIO)
    {
        if(eq->ce)
        {
            _linked_list_add(eq->ce->current.prev, &e->current, &eq->ce->current);
        }
        else
        {
            linked_list_add(&e->current, &eq->events);
        }

    }
    else if(flags & EVENT_PUSH_TAIL)
    {
        linked_list_add_last(&e->current, &eq->events);
    }
    else
    {
        linked_list_add(&e->current, &eq->events);
    }

    pthread_mutex_unlock(&eq->mutex);

    if(!(flags & EVENT_NO_WAKE) || (flags & EVENT_PUSH_HIGH_PRIO))
    {
        event_start_processing(eq);
    }



    return (0);
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
            e->timer = NULL;
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
        pthread_mutex_lock(&(*eq)->mutex);
        event_remove_wait(*eq, (void*) - 1);
        event_queue_clear(*eq);
        pthread_mutex_unlock(&(*eq)->mutex);
        pthread_mutex_destroy(&(*eq)->mutex);
#ifdef WIN32
        CloseHandle((*eq)->loop_event);
#elif __linux__
        close(((int*)(*eq)->loop_event)[0]);
        sfree((void**) & (*eq)->loop_event);
#endif
        sfree((void**)eq);
    }
}


int event_queue_empty(event_queue *eq)
{
    int empty = 0;

    if(eq == NULL)
        return(1);

    pthread_mutex_lock(&eq->mutex);
    empty = linked_list_empty(&eq->events);
    pthread_mutex_unlock(&eq->mutex);
    return(empty);
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

            struct itimerspec void_tim = {0};
            struct itimerspec ct = {0};
            timer_gettime(e->timer, &ct);
            pe = (memcmp(&void_tim, &ct, sizeof(struct itimerspec)) == 0);
#endif
        }

        if(pe)
        {
            pthread_mutex_lock(&eq->mutex);
            eq->ce = e;
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
    eq->ce = NULL;
    pthread_mutex_unlock(&eq->mutex);
}
