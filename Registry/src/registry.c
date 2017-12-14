#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <SDK/extension.h>

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

#define string_length(s) (((s) == NULL ? 0 : strlen((s))))

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

static wchar_t *utf8_to_ucs(unsigned char *str)
{
    size_t len = string_length(str);

    if (len == 0)
    {
        return(NULL);
    }
    size_t di = 0;
    wchar_t *dest = malloc((len + 1) * 2);
    memset(dest,0,(len + 1) * 2);
    for (size_t si = 0; si < len; si++)
    {
        if (str[si] <= 0x7F)
        {
            dest[di] = str[si];
        }
        else if (str[si] >= 0xE0 && str[si] <= 0xEF)
        {
            dest[di] = (((str[si++]) & 0xF) << 12);
            dest[di] = dest[di] | (((unsigned short)(str[si++]) & 0x103F) << 6);
            dest[di] = dest[di] | (((unsigned short)(str[si]) & 0x103F));
        }
        else if (str[si] >= 0xc0 && str[si] <= 0xDF)
        {
            dest[di] = ((((unsigned short)str[si++]) & 0x1F) << 6);
            dest[di] = dest[di] | (((unsigned short)str[si]) & 0x103F);
        }
        di++;
    }
    return(dest);
}

static unsigned char *ucs_to_utf8(wchar_t *s_in, size_t *bn, unsigned char be)
{
    /*Query the needed bytes to be allocated*/
    size_t nb = 0;
    unsigned char *out = NULL;
    for (size_t i = 0; s_in[i]; i++)
    {
        size_t ch = s_in[i];
        if (be)
        {
            unsigned char byte_hi = 0;
            unsigned char byte_lo = 0;
            byte_hi = (unsigned char)((ch & 0xFF00) >> 8);
            byte_lo = (unsigned char)((ch & 0xFF));
            ch = ((unsigned short)byte_lo) << 8 | (byte_hi);
        }
        if (ch < 0x80)
        {
            nb++;
        }
        else if (ch >= 0x80 &&ch < 0x800)
        {
            nb += 2;
        }
        else if (ch >= 0x800 && ch < 0xFFFF)
        {
            nb += 3;
        }
        else if (ch >= 0x10000 && ch < 0x10FFFF)
        {
            nb += 4;
        }
    }
    if (nb == 0)
        return(NULL);
    out = malloc(nb + 1);
    memset(out,0,nb + 1);
    /*Let's encode unicode to UTF-8*/
    size_t di = 0;
    for (size_t i = 0; s_in[i]; i++)
    {
        size_t ch = s_in[i];
        if (be)
        {
            unsigned char byte_hi = 0;
            unsigned char byte_lo = 0;
            byte_hi = (unsigned char)((ch & 0xFF00) >> 8);
            byte_lo = (unsigned char)((ch & 0xFF));
            ch = ((unsigned short)byte_lo) << 8 | (byte_hi);
        }
        if (ch < 0x80)
        {
            out[di++] = (unsigned char)s_in[i];
        }
        else if ((size_t)ch >= 0x80 && (size_t)ch < 0x800)
        {
            out[di++] = (s_in[i] >> 6) | 0xC0;
            out[di++] = (s_in[i] & 0x3F) | 0x80;
        }
        else if (ch >= 0x800 && ch < 0xFFFF)
        {
            out[di++] = (s_in[i] >> 12) | 0xE0;
            out[di++] = ((s_in[i] >> 6) & 0x3F) | 0x80;
            out[di++] = (s_in[i] & 0x3F) | 0x80;
        }
        else if (ch >= 0x10000 &&ch < 0x10FFFF)
        {
            out[di++] = ((unsigned long)s_in[i] >> 18) | 0xF0;
            out[di++] = ((s_in[i] >> 12) & 0x3F) | 0x80;
            out[di++] = ((s_in[i] >> 6) & 0x3F) | 0x80;
            out[di++] = (s_in[i] & 0x3F) | 0x80;
        }
    }
    if(bn)
        *bn = nb;
    return(out);
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

    rn=extension_string("HKEY",0x3,ip,"HKEY_CURRENT_USER");

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

    sfree((void**)&r->key_name);
    sfree((void**)&r->key_value);

    r->key_name=utf8_to_ucs(extension_string("KeyName",0x3,ip,NULL));
    r->key_value=utf8_to_ucs(extension_string("KeyValue",0x3,ip,NULL));
    r->mon=extension_bool("QueryChange",ip,1);

    if(r->rt)
    {
        RegOpenKeyExW(r->rt,r->key_name,0,KEY_READ,&r->rh);
    }
}


double update(void *spv)
{
    registry *r=spv;
    unsigned int ret=0;
    sfree((void**)&r->str_val);
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
            r->str_val=ucs_to_utf8((unsigned short*)r->buf,NULL,0);
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

    sfree((void**)&r->key_name);
    sfree((void**)&r->key_value);
    sfree((void**)&r->buf);
    sfree((void**)&r->str_val);

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
