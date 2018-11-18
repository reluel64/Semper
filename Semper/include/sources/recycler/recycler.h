#pragma once
#include <pthread.h>
#include <stdio.h>
#include <linked_list.h>
typedef enum
{
    recycler_query_items,
    recycler_query_size
} recycler_query_t;

typedef struct
{
        size_t mc;                  //count of monitors
        size_t inst_count;
        void **mh;                  //monitor handle
        double inf;                 //the value from the query
        pthread_t qth;              //query thread
        pthread_mutex_t mtx;
        void *kill;
        void *tha;
        size_t size;
        size_t count;
        list_entry children;
}recycler_common;

typedef struct
{
        void *ip;
        size_t lc;                  //last change
        size_t cc;                  //current change

        recycler_query_t rq;       //query type


      unsigned char mon_mode;    //monitoring mode
      unsigned char *cq_cmd;     //command when query is completed
      list_entry current;
} recycler;





typedef struct
{
        unsigned char *dir;
        list_entry current;
} recycler_dir_list;

int recycler_event_proc(recycler *r);
recycler_common *recycler_get_common(void);
#ifdef WIN32
int recycler_notifier_check_win32(recycler *r);
int recycler_query_user_win32(recycler *r);
int recycler_notifier_setup_win32(recycler *r);


#elif __linux__
int recycler_query_user_linux(recycler *r);
int recycler_notifier_setup_linux(recycler *r);
#endif
