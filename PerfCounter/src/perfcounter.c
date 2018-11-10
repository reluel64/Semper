/* Performance Counter collector source
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */
#ifdef WIN32
#include <pdh.h>
#include <stdio.h>
#include <wchar.h>
#include <pdhmsg.h>
#include <time.h>
#include <winperf.h>
#include <pthread.h>
#include <linked_list.h>
#include <sys/time.h>
typedef struct
{
        pthread_mutex_t mutex;
        pthread_t th;
        pthread_cond_t cond;
        list_entry counters;
        PDH_HQUERY query;
        size_t inst_cnt;
        void *kill;
}perf_counter_common;


typedef struct
{
        list_entry current;
        perf_counter_common *pcc;
        PDH_HCOUNTER *counter;          //counter handle
        size_t cnt_cnt;                 //counter count (shitty naming)
        unsigned char delta;            //need 2 queries? (by default we DO)
        double v;                       //holder for the value (we make sure that we do not loose it)
} perf_counter;


static void *perf_counter_thread(void *pv);
#define string_length(s) (((s) == NULL ? 0 : strlen((s))))

static void* zmalloc(size_t bytes)
{
    if(bytes == 0)
        return (NULL);

    void* p = malloc(bytes );

    if(p)
    {
        memset(p, 0, bytes);
        return (p);
    }
    return (NULL);
}


void init(void **spv,void *ip)
{
    static perf_counter_common pcc = {0};
    perf_counter *pc = NULL;
    if(pcc.inst_cnt == 0)
    {
        pcc.kill = semper_safe_flag_init();
        pthread_mutexattr_t mutex_attr;
        pthread_mutexattr_init(&mutex_attr);
        pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE_NP);
        pthread_mutex_init(&pcc.mutex, &mutex_attr);
        pthread_mutexattr_destroy(&mutex_attr);
        pthread_cond_init(&pcc.cond, NULL);
        PdhOpenQueryW(NULL,0,&pcc.query);
        list_entry_init(&pcc.counters);
        pthread_create(&pcc.th, NULL, perf_counter_thread, &pcc);
    }

    pc =zmalloc(sizeof(perf_counter));
    list_entry_init(&pc->current);
    pc->pcc = &pcc;
    pcc.inst_cnt++;
    *spv = pc;
}


void reset(void *spv,void *ip)
{
    perf_counter *pc=spv;
    static unsigned char instance_name[PDH_MAX_INSTANCE_NAME]= {0};
    static unsigned char counter_name[PDH_MAX_COUNTER_NAME]= {0};
    static unsigned char object_name[PDH_MAX_COUNTER_NAME]= {0};
    static unsigned char counter_path[PDH_MAX_COUNTER_PATH]= {0};
    unsigned char *ws=NULL;

    pthread_mutex_lock(&pc->pcc->mutex);

    if(pc->counter)
    {
        for(size_t i=0; i<pc->cnt_cnt; i++)
        {
            PdhRemoveCounter(pc->counter[i]);
        }

        free(pc->counter);
        pc->counter=NULL;
    }

    linked_list_remove(&pc->current);
    list_entry_init(&pc->current);
    pthread_mutex_unlock(&pc->pcc->mutex);


    ws=param_string("PerfInstance",0x3,ip,NULL);

    if(ws)
        snprintf(instance_name,PDH_MAX_INSTANCE_NAME,"(%s)",ws);

    else
        instance_name[0]=0;

    ws=param_string("PerfObject",0x3,ip,NULL);

    if(ws)
        snprintf(object_name,PDH_MAX_INSTANCE_NAME,"%s",ws);

    else
        object_name[0]=0;

    ws=param_string("PerfCounter",0x3,ip,NULL);

    if(ws)
        snprintf(counter_name,PDH_MAX_INSTANCE_NAME,"%s",ws);

    else
        counter_name[0]=0;

    pc->delta=param_bool("PerfDelta",ip,1);


    if(pc->pcc->query)
    {
        snprintf(counter_path,PDH_MAX_COUNTER_PATH,"\\%s%s\\%s",object_name,instance_name,counter_name);
        size_t sz=0;
        size_t index=0;
        size_t i=0;
        pc->cnt_cnt=0;
        unsigned short *pth=semper_utf8_to_ucs(counter_path);
        PdhExpandWildCardPathW(NULL,pth,NULL,(DWORD*)&sz,0);
        unsigned short *pth2=zmalloc((sz+1)*2);
        PdhExpandWildCardPathW(NULL,pth,pth2,(DWORD*)&sz,0);

        for(unsigned short *cb=pth2; index<sz; index++)
        {
            pc->cnt_cnt++;
            index+=(wcslen(cb+index));

            if(cb[index]==0&&cb[index+1]==0)
            {
                break;
            }
        }

        pc->counter=zmalloc(sizeof(PDH_HCOUNTER)*pc->cnt_cnt);
        pthread_mutex_lock(&pc->pcc->mutex);
        for(unsigned short *cb=pth2,index=0; index<sz; index++)
        {
            PdhAddCounterW(pc->pcc->query, cb+index, 0, &pc->counter[i++]);
            index+=(wcslen(cb+index));

            if(cb[index]==0&&cb[index+1]==0)
            {
                break;
            }
        }
        linked_list_add(&pc->current,&pc->pcc->counters);
        pthread_mutex_unlock(&pc->pcc->mutex);
        semper_free((void**)&pth);
        free(pth2);
    }

}


double update(void *spv)
{
    perf_counter *pc=spv;



    return(pc->v);
}

void destroy(void **spv)
{
    perf_counter *pc=*spv;

    pthread_mutex_lock(&pc->pcc->mutex);

    if(pc->counter)
    {
        for(size_t i=0; i<pc->cnt_cnt; i++)
        {
            PdhRemoveCounter(pc->counter[i]);
        }
        free(pc->counter);
        pc->counter=NULL;
    }
    linked_list_remove(&pc->current);
    pthread_mutex_unlock(&pc->pcc->mutex);


    if(pc->pcc->inst_cnt > 0)
        pc->pcc->inst_cnt --;


    if(pc->pcc->inst_cnt == 0)
    {
        semper_safe_flag_set(pc->pcc->kill,1);
        pthread_cond_signal(pc->pcc->cond); /*wake the thread*/
        pthread_join(pc->pcc->th,NULL);
        semper_safe_flag_destroy(&pc->pcc->kill);

        pthread_mutex_destroy(&pc->pcc->mutex);
        pthread_cond_destroy(&pc->pcc->cond);
        PdhCloseQuery(pc->pcc->query);
    }


    free(*spv);
    *spv=NULL; /*no dangling pointer please*/
}
#endif


void *perf_counter_thread(void *pv)
{
    perf_counter_common *pcc = pv;
    pthread_mutex_t mtx;
    pthread_mutex_init(&mtx,NULL);
    while(semper_safe_flag_get(pcc->kill) == 0)
    {
        struct timeval tv;
        struct timespec ts;
        perf_counter *pc = NULL;
        pthread_mutex_lock(&pcc->mutex);

        PdhCollectQueryData(pcc->query);
        list_enum_part(pc,&pcc->counters,current)
        {
            if(pc->counter)
            {
                PDH_FMT_COUNTERVALUE pfc= {0};

                pc->v=0.0;

                for(size_t i=0; i<pc->cnt_cnt; i++)
                {
                    if(!PdhGetFormattedCounterValue(pc->counter[i],PDH_FMT_DOUBLE|PDH_FMT_NOSCALE,NULL,&pfc))
                    {
                        if(pfc.CStatus==PDH_CSTATUS_NEW_DATA ||pfc.CStatus==PDH_CSTATUS_VALID_DATA)
                        {
                            pc->v+=pfc.doubleValue;
                        }
                    }
                }
            }
        }


        pthread_mutex_unlock(&pcc->mutex);
        gettimeofday(&tv, NULL);


        ts.tv_sec = time(NULL) + 1000 / 1000;
        ts.tv_nsec = tv.tv_usec * 1000 + 1000 * 1000 * (1000 % 1000);
        ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
        ts.tv_nsec %= (1000 * 1000 * 1000);

        pthread_mutex_lock(&mtx);
        pthread_cond_timedwait(&pcc->cond, &mtx, &ts);
        pthread_mutex_unlock(&mtx);
        pthread_mutex_destroy(&mtx);


    }
    pthread_mutex_destroy(&mtx);
    return(NULL);
}

