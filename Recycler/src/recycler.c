/*
* Recycler monitor source
* Part of Project 'Semper'
* Written by Alexandru-Daniel Mărgărit
*/
#include <mem.h>
#include <SDK/semper_api.h>
#include <pthread.h>
#include <windows.h>
#include <stdio.h>
#include <shellapi.h>
#include <pthread.h>
#include <linked_list.h>
#include <Sddl.h>
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
    unsigned char kill;
    unsigned char check;
    unsigned char mon_mode;
    unsigned char query_inf;
    unsigned char query_active;
    unsigned char *complete_query;
} recycler;

typedef struct
{
    unsigned char *dir;
    list_entry current;
} recycler_dir_list;


static int recycler_notif_setup(recycler *r,unsigned char destroy);
static int recycler_changed(recycler *r,unsigned char block);
static void *recycler_query_thread(void *p);
static void *recycler_monitor_thread(void *p);
static void recycler_query_user(recycler *r);


#define string_length(s) (((s) == NULL ? 0 : strlen((s))))

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

static unsigned short* utf8_to_ucs(unsigned char* str)
{
    size_t len = string_length(str);
    size_t di = 0;

    if(len == 0)
    {
        return (NULL);
    }

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


static unsigned char* ucs_to_utf8(unsigned short* s_in, size_t* bn, unsigned char be)
{
    size_t nb = 0;
    size_t di = 0;
    unsigned char* out = NULL;

    if(s_in==NULL)
    {
        return(NULL);
    }

    /*Query the needed bytes to be allocated*/
    for(size_t i = 0; s_in[i]; i++)
    {
        size_t ch = s_in[i];

        if(be)
        {
            unsigned char byte_hi = 0;
            unsigned char byte_lo = 0;
            byte_hi = (unsigned char)((ch & 0xFF00) >> 8);
            byte_lo = (unsigned char)((ch & 0xFF));
            ch = ((unsigned short)byte_lo) << 8 | (byte_hi);
        }

        if(ch < 0x80)
        {
            nb++;
        }
        else if(ch >= 0x80 && ch < 0x800)
        {
            nb += 2;
        }
        else if(ch >= 0x800 && ch < 0xFFFF)
        {
            nb += 3;
        }
        else if(ch >= 0x10000 && ch < 0x10FFFF)
        {
            nb += 4;
        }
    }

    if(nb == 0)
    {
        return (NULL);
    }

    out = zmalloc(nb + 1);
    /*Let's encode Unicode to UTF-8*/

    for(size_t i = 0; s_in[i]; i++)
    {
        size_t ch = s_in[i];

        if(be)
        {
            unsigned char byte_hi = 0;
            unsigned char byte_lo = 0;
            byte_hi = (unsigned char)((ch & 0xFF00) >> 8);
            byte_lo = (unsigned char)((ch & 0xFF));
            ch = ((unsigned short)byte_lo) << 8 | (byte_hi);
        }

        if(ch < 0x80)
        {
            out[di++] = (unsigned char)s_in[i];
        }
        else if((size_t)ch >= 0x80 && (size_t)ch < 0x800)
        {
            out[di++] = (s_in[i] >> 6) | 0xC0;
            out[di++] = (s_in[i] & 0x3F) | 0x80;
        }
        else if(ch >= 0x800 && ch < 0xFFFF)
        {
            out[di++] = (s_in[i] >> 12) | 0xE0;
            out[di++] = ((s_in[i] >> 6) & 0x3F) | 0x80;
            out[di++] = (s_in[i] & 0x3F) | 0x80;
        }
        else if(ch >= 0x10000 && ch < 0x10FFFF)
        {
            out[di++] = ((unsigned long)s_in[i] >> 18) | 0xF0;
            out[di++] = ((s_in[i] >> 12) & 0x3F) | 0x80;
            out[di++] = ((s_in[i] >> 6) & 0x3F) | 0x80;
            out[di++] = (s_in[i] & 0x3F) | 0x80;
        }
    }

    if(bn)
    {
        *bn = nb;
    }

    return (out);
}



static int windows_slahses(unsigned char* s)
{
    if(s == NULL)
    {
        return (-1);
    }

    for(size_t i = 0; s[i]; i++)
    {
        if(s[i] == '/')
        {
            s[i] = '\\';
        }
    }

    return (0);
}


static size_t file_size(size_t low, size_t high)
{
    return (low | (high << 32));
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
    recycler_query_user(r);
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
#if 0
    SHQUERYRBINFO tsb = { 0 };

    tsb.cbSize = sizeof(SHQUERYRBINFO);

    if(SHQueryRecycleBinW(NULL, &tsb) == S_OK)
    {
        r->item_count = tsb.i64NumItems;
        r->recycler_size = tsb.i64Size;
    }
#else
recycler_query_user(r);
#endif
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

static unsigned char *recycler_query_user_sid(size_t *len)
{
    HANDLE hTok = NULL;
    char *str_sid=NULL;
    if( OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hTok) )
    {
        unsigned char *buf = NULL;
        DWORD  token_sz = 0;
        GetTokenInformation(hTok, TokenUser, NULL, 0, &token_sz);

        if(token_sz)
            buf = (LPBYTE)LocalAlloc(LPTR, token_sz);


        if( GetTokenInformation(hTok, TokenUser, buf, token_sz, &token_sz) )
        {
            ConvertSidToStringSid(((PTOKEN_USER)buf)->User.Sid,&str_sid);
            if(str_sid)
                *len=string_length(str_sid);
        }
        LocalFree(buf);
        CloseHandle(hTok);
    }
    return(str_sid);
}

/*In-house replacement for SHQueryRecycleBinW*/
static void recycler_query_user(recycler *r)
{
    /*Get the user SID*/

    size_t sid_len=0;
    size_t file_count=0;
    size_t size=0;

    char *str_sid=recycler_query_user_sid(&sid_len);

    if(str_sid==NULL)
    {
        r->item_count=0;
        r->recycler_size=0;
    }

    char rroot[]=":\\$Recycle.Bin\\";
    unsigned char *buf=zmalloc(sizeof(rroot)+sid_len+3);
    snprintf(buf+1,sizeof(rroot)+sid_len+3,"%s%s",rroot,str_sid);

    for(char ch='A'; ch<='Z'; ch++)
    {
        buf[0]=ch;

        unsigned char *file=buf;
        list_entry qbase= {0};
        list_entry_init(&qbase);

        while(file)
        {
            size_t fpsz =0;
            WIN32_FIND_DATAW wfd = { 0 };
            void* fh =NULL;

            if(r->kill==0)
            {
                fpsz= string_length(file);
                unsigned char* filtered = zmalloc(fpsz + 6);
                snprintf(filtered,fpsz+6,"%s/$R*.*",file);

                unsigned short* filtered_uni = utf8_to_ucs(filtered);
                sfree((void**)&filtered);

                fh= FindFirstFileExW(filtered_uni,FindExInfoBasic, &wfd,FindExSearchNameMatch,NULL,2);
                sfree((void**)&filtered_uni);
            }

            do
            {
                if(r->kill||fh==INVALID_HANDLE_VALUE)
                {
                    break;
                }

                unsigned char* res = ucs_to_utf8(wfd.cFileName, NULL, 0);

                if(!strcasecmp(res, ".") || !strcasecmp(res, ".."))
                {
                    sfree((void**)&res);
                    continue;
                }

                if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    if(r->query_inf)
                    {
                        size_t res_sz = string_length(res);
                        unsigned char* ndir = zmalloc(res_sz + fpsz + 2);
                        snprintf(ndir,res_sz+fpsz+2,"%s/%s",file,res);
                        recycler_dir_list *fdl=zmalloc(sizeof(recycler_dir_list));
                        list_entry_init(&fdl->current);
                        linked_list_add(&fdl->current,&qbase);
                        fdl->dir=ndir;
                        windows_slahses(ndir);
                    }
                }
                else
                {
                    size+=file_size(wfd.nFileSizeLow,wfd.nFileSizeHigh);
                }

                if(file==buf) //we only count what is in root
                {
                    file_count++;
                }

                sfree((void**)&res);

            }
            while(r->kill==0&&FindNextFileW(fh, &wfd));

            if(fh!=NULL&&fh!=INVALID_HANDLE_VALUE)
            {
                FindClose(fh);
                fh=NULL;
            }

            if(buf!=file)
            {
                sfree((void**)&file);
            }

            if(linked_list_empty(&qbase)==0)
            {
                recycler_dir_list *fdl=element_of(qbase.prev,recycler_dir_list,current);
                file=fdl->dir;
                linked_list_remove(&fdl->current);
                sfree((void**)&fdl);
            }
            else
            {
                file=NULL;
                break;
            }
        }
    }

    LocalFree(str_sid);
    sfree((void**)&buf);

    pthread_mutex_lock(&r->mtx);
    r->item_count=file_count;
    r->recycler_size=size;
    pthread_mutex_unlock(&r->mtx);
}
