/*
* Recycler monitor source
* Part of Project 'Semper'
* Written by Alexandru-Daniel Mărgărit
*/
#include <mem.h>
#include <SDK/extension.h>
#include <pthread.h>
#include <windows.h>
#include <stdio.h>
#include <shellapi.h>
#include <pthread.h>
#define RECYCLER_DESTROY 0x2

typedef struct
{
    void *ip;
    void **monitor_handles;
    void *mon_event;
    size_t monitor_count;
    size_t item_count;
    size_t recycler_size;
    size_t last_check;
    size_t current_check;
    pthread_t qth; //query thread
    pthread_t mth; //monitor thread
    pthread_mutex_t mtx;
    unsigned char check;
    unsigned char mon_mode;
    unsigned char query_inf;
    unsigned char query_active;
    unsigned char *complete_query;
} recycler;

static int recycler_notif_setup(recycler *r,unsigned char destroy);
static int recycler_changed(recycler *r,unsigned char block);
static void *recycler_query_thread(void *p);
static void *recycler_monitor_thread(void *p);

static void *zmalloc(size_t m)
{
    void *p=malloc(m);
    if(p)
    {
        memset(p,0,m);
        return(p);
    }
    return(NULL);
}

static void sfree(void **p)
{
    if(p)
    {
        free(*p);
        *p=NULL;
    }
}

void init(void **spv,void *ip)
{
    recycler *r=zmalloc(sizeof(recycler));
    r->ip=ip;
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&r->mtx,&mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);
    *spv=r;

}

void reset(void *spv, void *ip)
{
    recycler *r=spv;

    unsigned char *temp=param_string("Type",EXTENSION_XPAND_ALL,ip,"Items");
    r->query_inf=0;

    if(temp && strcasecmp(temp,"Size")==0)
    {
        r->query_inf=1;
    }

    pthread_mutex_lock(&r->mtx);
    sfree((void**)&r->complete_query);
    r->complete_query=strdup(param_string("CompleteQuery",EXTENSION_XPAND_ALL,ip,NULL));
    pthread_mutex_unlock(&r->mtx);

    r->mon_mode=param_size_t("MonitorMode",ip,0)!=0;

    if(r->mon_mode==0&&r->mth)
    {
        if(r->monitor_handles)
        {
            SetEvent(r->mon_event);
        }
        pthread_join(r->mth,NULL);
        CloseHandle(r->mon_event);
        r->mth=0;
    }
    else if(r->mon_mode&&r->mth==0)
    {
        r->mon_event=CreateEvent(NULL,0,0,NULL);
        pthread_create(&r->mth,NULL,recycler_monitor_thread,r);
    }
}

double update(void *spv)
{
    recycler *r=spv;

    if(r->mon_mode)
    {
        return((double)(r->query_inf==0?r->item_count:r->recycler_size));
    }

    if(r->monitor_handles==NULL||recycler_changed(r,0))
    {
        recycler_notif_setup(r,0x1);
        r->current_check++;
    }

    if( r->last_check!=r->current_check&&r->qth==0)
    {
         r->last_check=r->current_check;
        r->query_active=1;
        pthread_create(&r->qth,NULL,recycler_query_thread,r);
    }

    if(r->query_active==0&&r->qth)
    {
        pthread_join(r->qth,NULL);
        r->qth=0;
    }

    return((double)(r->query_inf==0?r->item_count:r->recycler_size));

}

void destroy(void **spv)
{
    recycler *r=*spv;
    if(r->qth)
    {
        pthread_join(r->qth,NULL);
    }
    pthread_mutex_destroy(&r->mtx);
    sfree((void**)&r->complete_query);
    recycler_notif_setup(r,RECYCLER_DESTROY);
    sfree(spv);
}
//---------------------------------------------------------------
static int recycler_notif_setup(recycler *r,unsigned char flag)
{
    if(flag)
    {
        for(size_t i = 0; i < r->monitor_count; i++)
        {
            if(r->monitor_handles[i])
                FindCloseChangeNotification(r->monitor_handles[i]);
        }

        sfree((void**)&r->monitor_handles);
        r->monitor_count=0;

        if(RECYCLER_DESTROY&flag)
        {
            return(0);
        }
    }

    for(unsigned char i = 0; i < 'Z' - 'A'; i++)
    {
        unsigned short rec_folder[] = { 'A'+i, ':', '\\', '$', 'R', 'e', 'c', 'y', 'c', 'l', 'e', '.', 'B', 'i', 'n', 0 };

        void *th = FindFirstChangeNotificationW(rec_folder, 1, 0x1 | 0x2 | 0x4 | 0x8 | 0x10);

        if(th != INVALID_HANDLE_VALUE)
        {
           void *tmh=realloc(r->monitor_handles,sizeof(void*)*(r->monitor_count+1));
           if(tmh)
           {
            r->monitor_handles=tmh;
            r->monitor_handles[r->monitor_count]=th;
            r->monitor_count++;
          }
          else
          {
            sfree((void**)&r->monitor_handles);
            r->monitor_count=0;
          }
        }
    }
    return(r->monitor_count==0);
}

static int recycler_changed(recycler *r,unsigned char block)
{
    size_t status=WaitForMultipleObjects(r->monitor_count,r->monitor_handles,0,block?-1:0);
    if(status>=WAIT_OBJECT_0&&status<=WAIT_OBJECT_0+r->monitor_count-1)
    {
        return(1);
    }
    return(0);
}

static void *recycler_query_thread(void *p)
{
    recycler *r=p;
    SHQUERYRBINFO tsb = { 0 };

    tsb.cbSize = sizeof(SHQUERYRBINFO);

    if(SHQueryRecycleBinW(NULL, &tsb) == S_OK)
    {
        r->item_count = tsb.i64NumItems;
        r->recycler_size = tsb.i64Size;
    }

    pthread_mutex_lock(&r->mtx);
    send_command(r->ip,r->complete_query);
    pthread_mutex_unlock(&r->mtx);
    r->query_active=0;
    return (NULL);
}


static void *recycler_monitor_thread(void *p)
{
    recycler *r=p;
    while(r->mon_mode)
    {
        if(r->monitor_handles==NULL||recycler_changed(r,1))
        {

            r->monitor_handles?r->monitor_handles[r->monitor_count-1]=NULL:0;
            recycler_notif_setup(r,0x1);
            if(r->monitor_handles)
            {
              void *tmh=realloc(r->monitor_handles,sizeof(void*)*(r->monitor_count+1));
              if(tmh)
              {
               r->monitor_handles=tmh;
               r->monitor_handles[r->monitor_count]=r->mon_event;
               r->monitor_count++;
             }
             else
             {
               sfree((void**)&r->monitor_handles);
               r->monitor_count=0;
             }
            }
            r->current_check++;
        }
        if(r->mon_mode==0)
        {
            break;
        }
        if(r->current_check!=r->last_check&&r->qth==0)
        {

            r->query_active=1;
            pthread_create(&r->qth,NULL,recycler_query_thread,r);
        }

        if(r->query_active==0&&r->qth)
        {
            r->last_check=r->current_check;
            pthread_join(r->qth,NULL);
            r->qth=0;
        }
    }
    if(r->monitor_handles)
    {
        r->monitor_handles[r->monitor_count-1]=NULL;
    }
    recycler_notif_setup(r,RECYCLER_DESTROY);
    return(NULL);
}
