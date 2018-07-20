/*
 * Recycler monitor
 * Part of Project 'Semper'
 */
#ifdef WIN32
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <SDK/semper_api.h>

#define string_length(s) (((s) == NULL ? 0 : strlen((s))))

typedef struct
{
    HKEY rh;
    HKEY rt;
    size_t buf_sz;
    unsigned char *buf;
    unsigned char *str_val;
    unsigned int type;
    unsigned short *key_name;
    unsigned short *key_value;
    double num_val;
    unsigned char mon;
    void *evt; /*we'll use an event because why not*/
} registry;



static void* zmalloc(size_t bytes)
{
    if(bytes == 0)
        return (NULL);
    void* p = malloc(bytes);
    if(p)
    {
        memset(p, 0, bytes);
        return (p);
    }
    return (NULL);
}

static void sfree(void** p)
{
    if(p)
    {
        free(*p);
        *p = NULL;
    }
}

void init(void **spv,void *ip)
{
    *spv=zmalloc(sizeof(registry));
}

void reset(void *spv,void *ip)
{
    registry *r=spv;

    r->rt=NULL;
    unsigned char *rn=NULL;
    sfree((void**)&r->buf);
    r->buf_sz=0;
    r->buf=zmalloc(r->buf_sz+2);
    if(r->rh)
    {
        RegCloseKey(r->rh);
        r->rh=NULL;
    }

    if(r->evt)
    {
        CloseHandle(r->evt);
        r->evt=NULL;
    }

    rn=param_string("HKEY",0x3,ip,"HKEY_CURRENT_USER");

    if(rn)
    {
        if(!strcasecmp(rn,"HKEY_CURRENT_USER"))
            r->rt=HKEY_CURRENT_USER;

        else if(!strcasecmp(rn,"HKEY_LOCAL_MACHINE"))
            r->rt=HKEY_LOCAL_MACHINE;

        else if(!strcasecmp(rn,"HKEY_CLASSES_ROOT"))
            r->rt=HKEY_CLASSES_ROOT;

        else if(!strcasecmp(rn,"HKEY_CURRENT_CONFIG"))
            r->rt=HKEY_CURRENT_CONFIG;

        else if(!strcasecmp(rn,"HKEY_USERS"))
            r->rt=HKEY_USERS;
    }

    semper_free((void**)&r->key_name);
    semper_free((void**)&r->key_value);

    r->key_name=semper_utf8_to_ucs(param_string("KeyName",0x3,ip,NULL));
    r->key_value=semper_utf8_to_ucs(param_string("KeyValue",0x3,ip,NULL));
    r->mon=param_bool("QueryChange",ip,1);

    if(r->rt)
    {
        RegOpenKeyExW(r->rt,r->key_name,0,KEY_READ,&r->rh);
    }
}


double update(void *spv)
{
    registry *r=spv;
    unsigned int ret=0;
    semper_free((void**)&r->str_val);
    unsigned char attempt=128;
    if(!r->rh)
    {
        RegOpenKeyExW(r->rt,r->key_name,0,KEY_READ,&r->rh);

        if(r->rh==NULL)
            return(0.0);
    }
    else if(r->evt)
    {
        RegNotifyChangeKeyValue(r->rh,1,15,r->evt,1);
    }

    if(r->evt&&WaitForSingleObject(r->evt,0)!=0)
    {
        return(r->num_val);
    }
    while(attempt--&&(ret=RegQueryValueExW(r->rh,r->key_value,NULL,(DWORD*)&r->type,r->buf,(DWORD*)&r->buf_sz))==ERROR_MORE_DATA)
    {
        sfree((void**)&r->buf);
        r->buf=zmalloc(r->buf_sz+2);
    }

    if(attempt==0)
    {
        r->buf_sz=0;
        sfree((void**)&r->buf);
        r->buf=zmalloc(r->buf_sz+2);
        return(0.0);
    }

    if(ret==ERROR_SUCCESS)
    {
        switch(r->type)
        {
        case REG_DWORD:
            r->num_val=(double)((unsigned int*)r->buf)[0];
            break;
        case REG_QWORD:
            r->num_val=(double)(((LARGE_INTEGER*)r->buf)->QuadPart);
            break;
        case REG_SZ:
        case REG_EXPAND_SZ:
        case REG_MULTI_SZ:
        {
            r->str_val=semper_ucs_to_utf8((unsigned short*)r->buf,NULL,0);
            r->num_val=strtod(r->str_val,NULL);
            break;
        }
        }
    }
    else
    {
        RegCloseKey(r->rh);
        r->rh=NULL;
    }

    if(r->mon&&r->evt==NULL)
    {
        r->evt=CreateEvent(NULL,1,0,NULL);
    }

    return(r->num_val);
}

unsigned char *string(void *spv)
{
    registry *r=spv;
    if(r->type==REG_QWORD||r->type==REG_DWORD)
    {
        return(NULL);
    }
    return(r->str_val==NULL?(unsigned char*)"":r->str_val);
}

void destroy(void **spv)
{
    registry *r=*spv;

    semper_free((void**)&r->key_name);
    semper_free((void**)&r->key_value);
    sfree((void**)&r->buf);
    semper_free((void**)&r->str_val);

    if(r->evt)
    {
        CloseHandle(r->evt);
    }

    if(r->rh)
    {
        RegCloseKey(r->rh);
    }

    sfree(spv);
}
#endif
