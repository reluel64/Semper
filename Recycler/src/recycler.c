/*
* Recycler monitor source
* Part of Project 'Semper'
* Written by Alexandru-Daniel Mărgărit
*/
#ifdef WIN32
#include <SDK/semper_api.h>
#include <pthread.h>
#include <windows.h>
#include <stdio.h>
#include <shellapi.h>
#include <pthread.h>
#include <linked_list.h>
#include <Sddl.h>
#include <io.h>


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
    unsigned char kill;        //kill the query
    unsigned char can_empty;
    unsigned char mon_mode;    //monitoring mode
    unsigned char qa;          //query active
    unsigned char *cq_cmd;     //command when query is completed
} recycler;

typedef struct
{
    unsigned char *dir;
    list_entry current;
} recycler_dir_list;


static int recycler_notifier_check(recycler *r);
static void *recycler_query_thread(void *p);
static int recycler_query_user(recycler *r);
static unsigned char *recycler_query_user_sid(size_t *len);
static void recycler_notifier_destroy(recycler *r);
static int recycler_notifier_setup(recycler *r);

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
    diag_info("Initializing Recycler with context 0x%p",ip);
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
    pthread_mutex_lock(&r->mtx);
    unsigned char *temp=param_string("Type",EXTENSION_XPAND_ALL,ip,"Items");
    r->rq=recycler_query_items;

    if(temp && strcasecmp(temp,"Size")==0)
    {
        r->rq=recycler_query_size;
    }

    sfree((void**)&r->cq_cmd);

    r->cq_cmd=strdup(param_string("CompleteQuery",EXTENSION_XPAND_ALL,ip,NULL));

    pthread_mutex_unlock(&r->mtx);

    /*Check if there is a monitor running and if it is, try to stop it*/
    char new_monitor=param_size_t("MonitorMode",ip,0)!=0;

    if(r->mon_mode>new_monitor&&r->qth)
    {
        r->mon_mode=new_monitor;
        if(r->me)
        {
            SetEvent(r->me);
        }
        r->kill=1;

        pthread_join(r->qth,NULL);
        r->qth=0;
        CloseHandle(r->me);
    }

    if(r->mon_mode<new_monitor&&r->me==NULL)
    {
        r->me=CreateEvent(NULL,0,0,NULL);
        r->mon_mode=new_monitor;
        r->qa=1;
        if(pthread_create(&r->qth,NULL,recycler_query_thread,r)==0)
        {
            while(r->qa==1)
                sched_yield(); /*be kind - lend CPU cycles to other processes*/
        }
        else
        {
            r->qa=0;
        }
    }
}

double update(void *spv)
{
    recycler *r=spv;
    double lret=0.0;


    if(r->mon_mode==0)
    {
        if(r->mh==NULL||recycler_notifier_check(r))
        {
            recycler_notifier_destroy(r);
            recycler_notifier_setup(r);
            r->cc++;
        }

        if( r->lc!=r->cc && r->qth==0)
        {
            r->lc=r->cc;
            r->qa=1;

            if(pthread_create(&r->qth,NULL,recycler_query_thread,r)==0)
            {
                while(r->qa==1)
                    sched_yield(); /*be kind - lend CPU cycles to other processes*/
            }
            else
                r->qa=0;
        }

        if(r->qa==0&&r->qth)
        {
            pthread_join(r->qth,NULL);
            r->qth=0;
        }
    }
    pthread_mutex_lock(&r->mtx);
    lret=r->inf;
    pthread_mutex_unlock(&r->mtx);
    return(lret);
}

static void *dialog_thread(void *p)
{
    SHEmptyRecycleBin(NULL,NULL,0);
    return(NULL);
}

void command(void *spv,unsigned char *cmd)
{
    recycler *r=spv;
    if(cmd&&spv)
    {
        if(r->can_empty)
        {
            if(!strcasecmp("Empty",cmd))
            {
                /*Shitty workaround as the SHEmptyRecycleBin() will put the thread
                 * in a non-alertable state which will cause the event queue to stall while
                 * the dialog is up
                 * This issue could be handled in two ways:
                 * 1) Start a separate thread to handle the dialog - this is actually implemented
                 * 2) Redesign the event processing mechanism - hell no*/
                pthread_t dummy;
                pthread_attr_t th_att= {0};
                pthread_attr_init(&th_att);
                pthread_attr_setdetachstate(&th_att,PTHREAD_CREATE_DETACHED);
                pthread_create(&dummy,&th_att,dialog_thread,spv);
                pthread_attr_destroy(&th_att);
            }

            else if(!strcasecmp("EmptySilent",cmd))
            {
                SHEmptyRecycleBin(NULL,NULL,SHERB_NOCONFIRMATION|SHERB_NOPROGRESSUI|SHERB_NOSOUND);
            }
        }
        else if(!strcasecmp("Open",cmd))
        {
            ShellExecuteW(NULL, L"open", L"explorer.exe", L"/N,::{645FF040-5081-101B-9F08-00AA002F954E}", NULL, SW_SHOW);
        }
    }
}

void destroy(void **spv)
{
    recycler *r=*spv;
    r->kill=1;
    if(r->me)
    {
        SetEvent(r->me);
        CloseHandle(r->me);
    }

    if(r->qth)
        pthread_join(r->qth,NULL);


    pthread_mutex_destroy(&r->mtx);
    sfree((void**)&r->cq_cmd);
    recycler_notifier_destroy(r);
    sfree(spv);
}
//---------------------------------------------------------------


static void *recycler_query_thread(void *p)
{
    char first=0;
    recycler *r=p;
    do
    {
        r->qa=2;
        if(r->mon_mode)
        {
            if(first!=0)
            {
                recycler_notifier_check(r);
            }

            recycler_notifier_destroy(r);
            recycler_notifier_setup(r);
        }

        recycler_query_user(r);

        pthread_mutex_lock(&r->mtx);
        send_command_ex(r->ip,r->cq_cmd,100,1);
        pthread_mutex_unlock(&r->mtx);

        r->qa=0;
        if(first==0)
            first=1;
    }
    while(r->mon_mode&&r->kill==0);
    return (NULL);
}


static void recycler_notifier_destroy(recycler *r)
{
    for(size_t i=0; i<r->mc; i++)
    {
        if(r->mh[i]&&r->mh[i]!=INVALID_HANDLE_VALUE)
        {
            FindCloseChangeNotification(r->mh[i]);
        }
    }
    r->mc=0;
    sfree((void**)&r->mh);
}

static int recycler_notifier_setup(recycler *r)
{
    char buf[256]= {0};
    size_t sid_len=0;
    char *str_sid=recycler_query_user_sid(&sid_len);

    if(str_sid==NULL)
        return(-1);

    snprintf(buf+1,255,":\\$Recycle.Bin\\%s",str_sid);

    for(char i='A'; i<='Z'; i++)
    {
        char root[]= {i,':','\\',0};
        buf[0]=i;

        if(GetDriveTypeA(root)!=3)
        {
            continue;
        }
        void *tmh=FindFirstChangeNotificationA(buf, 1, 0x1 | 0x2 | 0x4 | 0x8 | 0x10);
        if(tmh!=INVALID_HANDLE_VALUE&&tmh!=NULL)
        {
            void *tmp=realloc(r->mh,sizeof(void*)*(r->mc+1));

            if(tmp)
            {
                r->mh=tmp;
                r->mh[r->mc++] = tmh;
            }
        }
    }

    if(r->mon_mode)
    {
        void *tmp=realloc(r->mh,sizeof(void*)*(r->mc+1));

        if(tmp)
        {
            r->mh=tmp;
            r->mh[r->mc]=r->me;
        }
    }

    LocalFree(str_sid);
}

static int recycler_notifier_check(recycler *r)
{
    if(r->mon_mode)
    {
        WaitForMultipleObjects(r->mc+1,r->mh,0,-1);
    }
    else
    {
        for(size_t i=0; i<r->mc; i++)
        {
            if(r->mh[i]&&r->mh[i]!=INVALID_HANDLE_VALUE)
            {
                if(WaitForSingleObject(r->mh[i],0)==0)
                    return(1);
            }
        }
    }
    return(0);
}

static unsigned char *recycler_query_user_sid(size_t *len)
{
    void *hTok = NULL;
    char *str_sid=NULL;
    if( OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hTok) )
    {
        unsigned char *buf = NULL;
        unsigned long  token_sz = 0;
        GetTokenInformation(hTok, TokenUser, NULL, 0, &token_sz);

        if(token_sz)
            buf = (unsigned char*)LocalAlloc(LPTR, token_sz);


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
static int recycler_query_user(recycler *r)
{
    /*Get the user SID*/

    size_t sid_len=0;
    size_t file_count=0;
    size_t size=0;

    char *str_sid=recycler_query_user_sid(&sid_len);

    if(str_sid==NULL)
    {
        diag_error("%s %d Failed to get user SID",__FUNCTION__,__LINE__);
        r->inf=0.0;
        return(-1);
    }

    char rroot[]=":\\$Recycle.Bin\\";
    unsigned char *buf=zmalloc(sizeof(rroot)+sid_len+3);
    snprintf(buf+1,sizeof(rroot)+sid_len+3,"%s%s",rroot,str_sid);

    for(char ch='A'; ch<='Z'; ch++)
    {
        buf[0]=ch;
        char root[]= {ch,':','\\',0};
        if(GetDriveTypeA(root)!=3)
        {
            continue;
        }
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

                if(file==buf)
                    snprintf(filtered,fpsz+6,"%s/$I*.*",file);
                else
                    snprintf(filtered,fpsz+6,"%s/*.*",file);

                unsigned short* filtered_uni = utf8_to_ucs(filtered);
                sfree((void**)&filtered);

                fh= FindFirstFileExW(filtered_uni,FindExInfoBasic, &wfd,FindExSearchNameMatch,NULL,2);
                sfree((void**)&filtered_uni);
            }

            do
            {
                if(r->kill||fh==INVALID_HANDLE_VALUE)
                    break;

                unsigned char* res = ucs_to_utf8(wfd.cFileName, NULL, 0);

                if(!strcasecmp(res, ".") || !strcasecmp(res, ".."))
                {
                    sfree((void**)&res);
                    continue;
                }

                if(file==buf)
                {
                    char valid=0;
                    size_t res_len=string_length(res);
                    unsigned char *s=zmalloc(res_len+fpsz+6);
                    snprintf(s,res_len+fpsz+6,"%s\\%s",file,res);
                    windows_slahses(s);
                    s[fpsz+2]='R';

                    if(access(s,0)==0)
                    {
                        s[fpsz+2]='I';
                        FILE *f=fopen(s,"rb");

                        if(f)
                        {
                            size_t sz=0;
                            fseek(f,8,SEEK_SET);
                            fread(&sz,sizeof(size_t),1,f);
                            fclose(f);
                            size+=sz;
                            valid=1;
                        }
                    }

                    sfree((void**)&s);

                    if(valid==0)
                    {
                        diag_warn("%s %d Entry %s is not valid",__FUNCTION__,__LINE__,res);
                        sfree((void**)&res);
                        continue;
                    }
                }

#if 0
                if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
#if 0
                    if(r->rq==recycler_query_size)
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
#endif
                }
                else
                {

                    size+=file_size(wfd.nFileSizeLow,wfd.nFileSizeHigh);
                }
#endif
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
    r->can_empty=(file_count!=0);
    switch(r->rq)
    {
    case recycler_query_items:
        r->inf=(double)file_count;
        break;
    case recycler_query_size:
        r->inf=(double)size;
        break;
    }
    pthread_mutex_unlock(&r->mtx);
    return(1);
}
#endif
