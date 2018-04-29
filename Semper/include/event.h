#pragma once
#include <pthread.h>
#define EVENT_PUSH_TAIL             1<<0
#define EVENT_REMOVE_BY_DATA        1<<1
#define EVENT_REMOVE_BY_HANDLER     1<<2
#define EVENT_REMOVE_BY_DATA_HANDLER (EVENT_REMOVE_BY_DATA|EVENT_REMOVE_BY_HANDLER)
#define EVENT_PUSH_TIMER            1<<3
#define EVENT_NO_WAKE               1<<4
#define EVENT_PUSH_HIGH_PRIO        1<<5


typedef int (*event_handler)(void*);
typedef int (*event_wait_handler)(void *,void *);

typedef struct
{
    event_handler handler;
    void* pv;
    void* timer;
    size_t time;
    list_entry current;
} event;

typedef struct _event_queue
{
    void *loop_event;
    pthread_mutex_t mutex;
    list_entry events;
    list_entry waiters;
    event *ce; //current event
} event_queue;

typedef struct
{
    event_wait_handler ewh;
    void *pv;
    void *wait;
#ifdef WIN32
    void *mon_th;
#endif
    unsigned int flags;
    list_entry current;
} event_waiter;


unsigned char event_wait(event_queue* eq);
event_queue* event_queue_init(void);
int event_remove_wait(event_queue *eq,void *wait);
int event_add_wait(event_queue *eq,event_wait_handler ewh,void *pv,void *wait,unsigned int flags);
int event_push(event_queue* eq, event_handler handler, void* pv, size_t timeout, unsigned char flags);
void event_remove(event_queue* eq, event_handler eh, void* pv, unsigned char flags);
void event_queue_clear(event_queue* eq);
void event_process(event_queue* eq);
int event_queue_empty(event_queue *eq);
