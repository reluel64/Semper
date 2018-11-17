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
#ifdef WIN32
int recycler_notifier_check_win32(recycler *r);
int recycler_query_user_win32(recycler *r, double *val);
int recycler_notifier_setup_win32(recycler *r);
void recycler_notifier_destroy_win32(recycler *r);
unsigned char *recycler_query_user_sid(size_t *len);

#elif __linux__
#endif
