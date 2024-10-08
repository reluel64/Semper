#pragma once
#include <pthread.h>
#define EVENT_PUSH_TAIL             (1<<0)
#define EVENT_REMOVE_BY_DATA        (1<<1)
#define EVENT_REMOVE_BY_HANDLER     (1<<2)
#define EVENT_PUSH_TIMER            (1<<3)
#define EVENT_NO_WAKE               (1<<4)
#define EVENT_TIMER_INIT            (1<<5)
#define EVENT_REMOVE_BY_DATA_HANDLER (EVENT_REMOVE_BY_DATA|EVENT_REMOVE_BY_HANDLER)

typedef struct _event_queue event_queue;

typedef int (*event_handler)(void*);
typedef size_t (*event_wait_func)(void*pv,size_t );
typedef void (*event_wake_func)(void *pv);
typedef struct
{
    unsigned char flags;
    event_handler handler;
    void* pv;
    size_t time;
    size_t pos;
    list_entry current;
} event;

typedef struct _event_queue
{
    event_wait_func wfcn;
    event_wake_func wkfcn;
    pthread_mutex_t mutex;
    list_entry events;
    size_t to_sleep;
    size_t slept;
    void *pvwfcn;
} event_queue;

event_queue* event_queue_init(event_wait_func wfcn,event_wake_func wkfcn,void *pvwfcn);
void event_wait(event_queue* eq);
int event_push(event_queue* eq, event_handler handler, void* pv, size_t timeout, unsigned char flags);
int event_queue_empty(event_queue *eq);
void event_remove(event_queue* eq, event_handler eh, void* pv, unsigned char flags);
void event_queue_clear(event_queue* eq);
void event_process(event_queue* eq);
void event_wake(event_queue *eq);
void event_queue_destroy(event_queue** eq);

