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


void event_wait(event_queue* eq)
{
    if(eq)
    {
        eq->slept = 0;

        if(eq->wfcn)
        {
            eq->slept =  eq->wfcn(eq->pvwfcn, eq->to_sleep);
        }
    }
}

void event_wake(event_queue *eq)
{
    if(eq)
    {
        pthread_mutex_lock(&eq->mutex);

        if(eq->wkfcn)
        {
            eq->wkfcn(eq->pvwfcn);
        }

        pthread_mutex_unlock(&eq->mutex);
    }
}

event_queue* event_queue_init(event_wait_func wfcn, event_wake_func wkfcn, void *pvwfcn)
{
    event_queue* eq = zmalloc(sizeof(event_queue));

    if(eq)
    {
        pthread_mutexattr_t mutex_attr;
        pthread_mutexattr_init(&mutex_attr);
        pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&eq->mutex, &mutex_attr);
        pthread_mutexattr_destroy(&mutex_attr);
        list_entry_init(&eq->events);
        eq->wfcn = wfcn;
        eq->pvwfcn = pvwfcn;
        eq->wkfcn = wkfcn;
    }

    return (eq);
}

void event_remove(event_queue* eq, event_handler eh, void* pv, unsigned char flags)
{
    if(eq)
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
            }
        }

        pthread_mutex_unlock(&eq->mutex);
    }
}

int event_push(event_queue* eq, event_handler handler, void* pv, size_t timeout, unsigned char flags)
{
    if(eq == NULL)
    {
        return(-1);
    }

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

    event* e = zmalloc(sizeof(event));
    list_entry_init(&e->current);

    e->handler = handler;
    e->pv = pv;



    pthread_mutex_lock(&eq->mutex);

    if(flags & EVENT_PUSH_HIGH_PRIO)
    {
        flags &= ~EVENT_PUSH_TIMER;

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

    if(flags & EVENT_PUSH_TIMER)
    {
        e->time = (timeout >= 16 ? timeout : 16);
        flags &= ~EVENT_TIMER_INIT;
    }

    e->flags = flags;

    if(!(flags & EVENT_NO_WAKE) || (flags & EVENT_PUSH_HIGH_PRIO))
    {
        if(eq->wkfcn)
        {
            eq->wkfcn(eq->pvwfcn);
        }
    }

    pthread_mutex_unlock(&eq->mutex);

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

        sfree((void**)&e);
    }
}

void event_queue_clear(event_queue* eq)
{
    event* e = NULL;
    event* te = NULL;

    if(eq)
    {
        pthread_mutex_lock(&eq->mutex);
        list_enum_part_safe(e, te, &eq->events, current)
        {
            event_remove_internal(e);
        }
        pthread_mutex_unlock(&eq->mutex);
    }
}

void event_queue_destroy(event_queue** eq)
{
    if(*eq)
    {
        pthread_mutex_lock(&(*eq)->mutex);
        event_queue_clear(*eq);
        pthread_mutex_unlock(&(*eq)->mutex);
        pthread_mutex_destroy(&(*eq)->mutex);
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
    if(eq)
    {
        event* e = NULL;
        event* te = NULL;
        eq->to_sleep = -1;
        list_enum_part_backward_safe(e, te, &eq->events, current)
        {
            pthread_mutex_lock(&eq->mutex);
            eq->ce = e;
            pthread_mutex_unlock(&eq->mutex);
            
            if(!(e->flags & EVENT_TIMER_INIT) && (e->flags & EVENT_PUSH_TIMER))
            {
                e->flags |= EVENT_TIMER_INIT;
                eq->to_sleep = min(e->time, eq->to_sleep);
                continue;
            }

            if(e->flags & EVENT_PUSH_TIMER)
            {
                e->pos += eq->slept;

                if(e->time >= e->pos)
                {
                    eq->to_sleep = min(e->time - e->pos, eq->to_sleep);
                }
            }

            if((e->time <= e->pos) || !(e->flags & EVENT_PUSH_TIMER))
            {
               

                if(e->handler)
                {
                    e->handler(e->pv);
                }

                pthread_mutex_lock(&eq->mutex);
                eq->ce = NULL;
                event_remove_internal(e);
                pthread_mutex_unlock(&eq->mutex);
            }


        }
        pthread_mutex_lock(&eq->mutex);
        eq->ce = NULL;
        pthread_mutex_unlock(&eq->mutex);
    }
}
