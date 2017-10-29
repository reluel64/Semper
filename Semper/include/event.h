#pragma once
#include <pthread.h>
#define EVENT_PUSH_TAIL 0x1
#define EVENT_REMOVE_BY_DATA 0x2
#define EVENT_REMOVE_BY_HANDLER 0x4
#define EVENT_REMOVE_BY_DATA_HANDLER 0x6
#define EVENT_PUSH_TIMER 0x8
typedef int (*event_handler)(void*);
typedef struct _event
{
    event_handler handler;
    void* pv;
    void* timer;
    size_t time;
    list_entry current;

} event;

typedef struct _event_queue
{
    void* loop_event;
    pthread_mutex_t mutex;
    list_entry events;
    event *ce; //current event
} event_queue;

void event_wait(event_queue* eq);
event_queue* event_queue_init(void);

event* event_push(event_queue* eq, event_handler handler, void* pv, size_t timeout, unsigned char flags);
void event_remove(event_queue* eq, event_handler eh, void* pv, unsigned char flags);
void event_queue_clear(event_queue* eq);
void event_process(event_queue* eq);
#ifdef __linux__
void event_queue_set_window_event(event_queue *eq,int fd);
void event_queue_set_inotify_event(event_queue *eq,int fd);
#endif
