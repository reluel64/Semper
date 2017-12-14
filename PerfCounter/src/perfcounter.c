/* Performance Counter collector source
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */
#include <pdh.h>
#include <SDK/extension.h>
#include <stdio.h>
#include <wchar.h>
#include <pdhmsg.h>
#include <time.h>

typedef struct
{
    PDH_HQUERY phq;                 //query
    PDH_HCOUNTER *counter;           //counter handle
    size_t cnt_cnt;                 //counter count (shitty naming)
    unsigned char delta;            //need 2 queries? (by default we DO)
    time_t t;                       //hold the old time to make sure that there's at least 1 second between 2 queries
    double v;                       //holder for the value (we make sure that we do not loose it)
} perf_counter;

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

static unsigned short* utf8_to_ucs(unsigned char* str)
{
    size_t len = string_length(str);

    if(len == 0)
    {
        return (NULL);
    }
    size_t di = 0;
    unsigned short* dest = zmalloc((len + 1) * 2);

    for(size_t si = 0; si < len; si++)
    {
        if(str[si] <= 0x7F)
        {
            dest[di] = str[si];
        }
        else if(str[si] >= 0xE0 && str[si] <= 0xEF)
        {
            dest[di] = (((str[si++]) & 0xF) << 12);
            dest[di] = dest[di] | (((unsigned short)(str[si++]) & 0x103F) << 6);
            dest[di] = dest[di] | (((unsigned short)(str[si]) & 0x103F));
        }
        else if(str[si] >= 0xc0 && str[si] <= 0xDF)
        {
            dest[di] = ((((unsigned short)str[si++]) & 0x1F) << 6);
            dest[di] = dest[di] | (((unsigned short)str[si]) & 0x103F);
        }
        di++;
    }
    return (dest);
}


void init(void **spv,void *ip)
{
    *spv=malloc(sizeof(perf_counter));

    memset(*spv,0,sizeof(perf_counter));
}


void reset(void *spv,void *ip)
{
    perf_counter *pc=spv;
    static unsigned char instance_name[PDH_MAX_INSTANCE_NAME]= {0};
    static unsigned char counter_name[PDH_MAX_COUNTER_NAME]= {0};
    static unsigned char object_name[PDH_MAX_COUNTER_NAME]= {0};
    static unsigned char counter_path[PDH_MAX_COUNTER_PATH]= {0};
    unsigned char *ws=NULL;

    if(pc->counter)
    {
        for(size_t i=0; i<pc->cnt_cnt; i++)
        {
            PdhRemoveCounter(pc->counter[i]);
        }
        free(pc->counter);
        pc->counter=NULL;
    }
    if(pc->phq)
    {
        PdhCloseQuery(pc->phq);
        pc->phq=NULL;
    }


    ws=extension_string("PerfInstance",0x3,ip,NULL);

    if(ws)
    {
        snprintf(instance_name,PDH_MAX_INSTANCE_NAME,"(%s)",ws);
    }
    else
    {
        instance_name[0]=0;
    }

    ws=extension_string("PerfObject",0x3,ip,NULL);

    if(ws)
    {
        snprintf(object_name,PDH_MAX_INSTANCE_NAME,"%s",ws);
    }
    else
    {
        object_name[0]=0;
    }

    ws=extension_string("PerfCounter",0x3,ip,NULL);

    if(ws)
    {
        snprintf(counter_name,PDH_MAX_INSTANCE_NAME,"%s",ws);
    }
    else
    {
        counter_name[0]=0;
    }

    pc->delta=extension_bool("PerfDelta",ip,1);


    PdhOpenQueryW(NULL,0,&pc->phq);

    if(pc->phq)
    {
        snprintf(counter_path,PDH_MAX_COUNTER_PATH,"\\%s%s\\%s",object_name,instance_name,counter_name);
        size_t sz=0;
        size_t index=0;
        size_t i=0;
        pc->cnt_cnt=0;
        unsigned short *pth=utf8_to_ucs(counter_path);
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

        for(unsigned short *cb=pth2,index=0; index<sz; index++)
        {
            PdhAddCounterW(pc->phq,cb+index,0,&pc->counter[i++]);
            index+=(wcslen(cb+index));

            if(cb[index]==0&&cb[index+1]==0)
            {
                break;
            }
        }

        pc->t=time(NULL);
        PdhCollectQueryData(pc->phq);
        free(pth);
        free(pth2);
    }
}


double update(void *spv)
{
    perf_counter *pc=spv;
    time_t ct=time(NULL);

    if(pc->counter&&pc->phq&&(pc->delta==0||ct-pc->t>=1 )&&!PdhCollectQueryData(pc->phq))
    {
        PDH_FMT_COUNTERVALUE pfc= {0};
        pc->t=ct;
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
    return(pc->v);
}

void destroy(void **spv)
{
    perf_counter *pc=*spv;
    if(pc->counter)
    {
        for(size_t i=0; i<pc->cnt_cnt; i++)
        {
            PdhRemoveCounter(pc->counter[i]);
        }
        free(pc->counter);
        pc->counter=NULL;
    }
    if(pc->phq)
    {
        PdhCloseQuery(pc->phq);
        pc->phq=NULL;
    }

    free(*spv);
    *spv=NULL; /*no dangling pointer please*/
}
