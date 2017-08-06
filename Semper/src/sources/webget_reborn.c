#define CURL_STATICLIB
#ifdef WIN32
#include <winsock2.h>
#include <wchar.h>
#elif __linux__
#include <sys/stat.h>
#endif
#include <sources/extension.h>
#include <pcre.h>
#include <sources/source.h>
#include <stdio.h>
#include <stdlib.h>

#include <string_util.h>
#include <mem.h>
#include <pthread.h>
#include <curl/curl.h>
#define OVECTOR_LIMIT 300
typedef struct
{
    unsigned char *buf;
    size_t buf_pos;
    unsigned char *stop;
} webget_curl;

typedef enum
{
    webget_none_type=0,
    webget_parent_type=1,
    webget_child_type=2
} webget_holder_type;

typedef struct webget webget;

typedef struct webget
{

    pthread_t dt;        //downloader thread
    pthread_mutex_t mtx; //mutex to protect a resource to be accessed while it is modified
    unsigned char *address; //could be a url or a file
    unsigned char *regexp; //regular expression
    list_entry children;
    list_entry current;
    unsigned char stop;
    unsigned char work;
    void *ip;
    //TODO:
    unsigned char *success_act;
    unsigned char *parse_fail_act;
    unsigned char *dwl_fail;
    unsigned char *connect_fail;
    unsigned char dwl;
    unsigned char *ret_str;
    size_t string_1_index;
    size_t string_2_index;
    unsigned char *string_1;
    unsigned char *string_2;
    webget *parent;
} webget;



static int  webget_is_thread_running(pthread_t *t)
{
    static pthread_t empty_thread;
    memset(&empty_thread,0,sizeof(pthread_t));
    return(memcmp(&empty_thread,t,sizeof(pthread_t))!=0);
}

static inline int  webget_is_mutex_init(pthread_mutex_t *m)
{
    static pthread_mutex_t empty_mutex;
    memset(&empty_mutex,0,sizeof(pthread_mutex_t));
    return(memcmp(&empty_mutex,m,sizeof(pthread_mutex_t))!=0);
}




void webget2_init(void **spv,void *ip)
{
    unused_parameter(ip);
    webget *w=zmalloc(sizeof(webget));
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&w->mtx,&mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);
    list_entry_init(&w->children);
    list_entry_init(&w->current);
    *spv=w;
}




void webget2_reset(void *spv,void *ip)
{
    webget *w=spv;
    void *pip=NULL;
    unsigned char *temp=NULL;


    pthread_mutex_lock(&w->mtx);
    sfree((void**)&w->address);
    sfree((void**)&w->regexp);
    sfree((void**)&w->success_act);
    sfree((void**)&w->parse_fail_act);
    sfree((void**)&w->dwl_fail);
    sfree((void**)&w->connect_fail);

    w->regexp=clone_string(extension_string("Regexp",EXTENSION_XPAND_ALL,ip,NULL));

    w->string_2_index=extension_size_t("StringIndex2",ip,0);
    w->string_1_index=extension_size_t("StringIndex",ip,1);
    temp=extension_string("URL",EXTENSION_XPAND_VARIABLES,ip,NULL);

    if((pip=extension_get_parent(temp,ip))!=NULL&&w->parent==NULL)
    {
        w->parent=extension_private(pip);
        if((w->current.next==NULL||linked_list_empty(&w->current)))
        {
            list_entry_init(&w->current);
            linked_list_add_last(&w->current,&w->parent->children);
        }

    }
    else if(pip==NULL)
    {
        linked_list_remove(&w->current);
        w->address=clone_string(temp);
        w->success_act=clone_string(extension_string("SuccessAction",EXTENSION_XPAND_ALL,ip,NULL));
        w->parse_fail_act=clone_string(extension_string("ParseFailAction",EXTENSION_XPAND_ALL,ip,NULL));
        w->dwl_fail=clone_string(extension_string("DownloadFailAction",EXTENSION_XPAND_ALL,ip,NULL));
        w->connect_fail=clone_string(extension_string("ConnetionFailAction",EXTENSION_XPAND_ALL,ip,NULL));
    }
    pthread_mutex_unlock(&w->mtx);
}


static int webget_apply_regexp(unsigned char *buffer,size_t buf_sz,unsigned char *regexp,int *ovector,size_t ovec_sz,int *match_count)
{
    if(regexp==NULL||buffer==NULL||ovector==NULL||ovec_sz<1)
    {
        return(-1);
    }

    int err_off = 0;
    const char* err = NULL;

    pcre* pc = pcre_compile((char*)regexp, 0, &err, &err_off, NULL);

    if(pc==NULL)
    {
        return(-2);
    }

    //int buf_sz=string_length(buffer);
    *match_count = pcre_exec(pc, NULL, (char*)buffer, buf_sz, 0, 0, ovector, ovec_sz / sizeof(int)); /*good explanation at http://libs.wikia.com/wiki/Pcre_exec */
    pcre_free(pc);

    if(*match_count < 1)
    {
        *match_count=0;
        return (-3);
    }
    /*
        *out_buf=zmalloc(sizeof(unsigned char *)*match_count);
        *matches=match_count;

        for(int i=0; i<match_count; i++)
        {
            size_t offset=ovector[2*i];
            size_t str_len=ovector[(2*i)+1]-ovector[(2*i)];
            (*out_buf)[i]=zmalloc(str_len+1);
            strncpy((*out_buf)[i],buffer+offset,str_len);
        }
    */
    return(0);
}

static size_t webget_curl_callback(char *buf, size_t size, size_t nmemb, void *pv)
{
    webget_curl *wcrl=pv;
//------------------------------
    unsigned char *temp=realloc(wcrl->buf,wcrl->buf_pos+size*nmemb+1);

    if(temp==NULL||*wcrl->stop==1)
    {
        sfree((void**)&wcrl->buf);
        wcrl->buf_pos=0;
        return(0);
    }
    else
    {
        memmove(temp+wcrl->buf_pos,buf,size*nmemb);
        wcrl->buf_pos+=size*nmemb;
        wcrl->buf=temp;
        wcrl->buf[wcrl->buf_pos]=0;
        return(size*nmemb);
    }
}



static int webget_curl_download(unsigned char *link,webget_curl *wcrl)
{
    int ret=1;
    CURL *c_state=curl_easy_init();
    curl_easy_setopt(c_state,CURLOPT_URL,link);
    curl_easy_setopt(c_state,CURLOPT_WRITEFUNCTION,webget_curl_callback);
    curl_easy_setopt(c_state,CURLOPT_WRITEDATA,wcrl);
    curl_easy_setopt(c_state,CURLOPT_FOLLOWLOCATION,1);
    curl_easy_setopt(c_state,CURLOPT_MAXREDIRS,4096);
    curl_easy_setopt(c_state, CURLOPT_SSL_VERIFYPEER, 0);

    ret=curl_easy_perform(c_state);
    curl_easy_cleanup(c_state);

    if(ret)
    {
        sfree((void**)&wcrl->buf);
        wcrl->buf_pos=0;
    }
    return(ret);
}



static void *webget_worker(void *p)
{
    webget *w=p;
    int ret=0;
    unsigned char *link=NULL;
    unsigned char *regexp=NULL;
    int ovector[OVECTOR_LIMIT]= {0};
    int match_count=0;
    webget_curl wc=
    {
        .buf=NULL,
        .buf_pos=0,
        .stop=&w->stop
    };

    w->work=2; //thread has been started sw we can let the main thread do it's job

    pthread_mutex_lock(&w->mtx);
    link=w->address;            //avoid the link to be freed by saving it locally and setting the original pointer to null
    regexp=w->regexp;
    w->address=NULL;            //when we finish with the pointer, we'll restore it if it wasn't modified
    w->regexp=NULL;
    pthread_mutex_unlock(&w->mtx);

    if(link&&(ret=webget_curl_download(link,&wc))!=0)
    {
        pthread_mutex_lock(&w->mtx);
        if(ret==CURLE_OPERATION_TIMEDOUT)
            extension_send_command(w->ip,w->connect_fail);
        else if(w->stop==0)
            extension_send_command(w->ip,w->dwl_fail);
        w->work=0;

        if(w->address!=NULL)
            sfree((void**)&link);
        else
            w->address=link;

        if(w->regexp!=NULL)
            sfree((void**)&regexp);
        else
            w->regexp=regexp;
        pthread_mutex_unlock(&w->mtx);
        return(NULL);
    }

    if(w->parent==NULL&&(w->stop==1||webget_apply_regexp(wc.buf,wc.buf_pos-1,regexp,(int*)ovector,300,&match_count)<0))
    {
        pthread_mutex_lock(&w->mtx);

        sfree((void**)&wc.buf);
        wc.buf_pos=0;
        w->stop==0?extension_send_command(w->ip,w->parse_fail_act):0;
        w->work=0;

        if(w->address!=NULL)
            sfree((void**)&link);
        else
            w->address=link;

        if(w->regexp!=NULL)
            sfree((void**)&regexp);
        else
            w->regexp=regexp;

        pthread_mutex_unlock(&w->mtx);
        return(NULL);
    }

    if(w->parent==NULL)
    {
        webget *wch=NULL;
        list_enum_part(wch,&w->children,current)
        {
            if(w->stop)
                break;
            pthread_mutex_lock(&wch->mtx);



            sfree((void**)&wch->string_1);
            sfree((void**)&wch->string_2);


            if(wch->string_1_index<match_count&&wch->string_1_index!=0)
            {

                size_t index=wch->string_1_index;
                size_t len=ovector[index*2-1]-ovector[index*2];

                wch->string_1=zmalloc(len+1);
                strncpy(wch->string_1,wc.buf+ovector[index*2],len);


                if(wch->regexp)
                {
                    int lovector[300]= {0};
                    int lmatch_count=0;

                    if(webget_apply_regexp(wch->string_1,len,wch->regexp,(int*)lovector,300,&lmatch_count)==0)
                    {
                        if(wch->string_2_index<lmatch_count&&wch->string_2_index!=0)
                        {

                            size_t index2=wch->string_2_index;
                            size_t len2=lovector[index2*2-1]-lovector[index2*2];
                            wch->string_2=zmalloc(len2+1);
                            strncpy(wch->string_2,wch->string_1+lovector[index2*2],len2);

                        }
                    }
                }
            }
            pthread_mutex_unlock(&wch->mtx);
        }
    }
    if(w->address!=NULL)
        sfree((void**)&link);
    else
        w->address=link;

    if(w->regexp!=NULL)
        sfree((void**)&regexp);
    else
        w->regexp=regexp;
    return(NULL);

}

/*
static void *webget_parent_worker(void *p)
{
    webget *w=p;
    webget_curl wcrl= {0};
    wcrl.stop=&wp->stop;
    webget_child *wc=NULL;
    int ovector[300]= {0};
    int match_count=0;
    wp->work=2;

    int ret=0;

    if((ret=webget_curl_download(wp,wp->address,&wcrl))!=0)
    {
        pthread_mutex_lock(&w->mtx);
        if(ret==CURLE_OPERATION_TIMEDOUT)
            extension_send_command(wp->ip,wp->connect_fail);
        else if(wp->stop==0)
            extension_send_command(wp->ip,wp->dwl_fail);
        wp->work=0;
        pthread_mutex_unlock(&w->mtx);
        return(NULL);
    }

    if(wp->stop==1||webget_apply_regexp(wcrl.buf,wcrl.buf_pos-1,wp->regexp,(int*)ovector,300,&match_count)<0)
    {
        sfree((void**)&wcrl.buf);
        wcrl.buf_pos=0;
        extension_send_command(wp->ip,wp->parse_fail_act);
        wp->work=0;
        return(NULL);
    }

    //start updating the children
    list_enum_part(wc,&wp->children,current)
    {
        if(wp->stop)
            break;

        pthread_mutex_lock(&wc->cmtx);
        sfree((void**)&wc->string_1);
        sfree((void**)&wc->string_2);
        pthread_mutex_unlock(&wc->cmtx);

        if(wc->string_1_index<match_count&&wc->string_1_index!=0)
        {
            pthread_mutex_lock(&wc->cmtx);
            size_t index=wc->string_1_index;
            size_t len=ovector[index*2-1]-ovector[index*2];

            wc->string_1=zmalloc(len+1);
            strncpy(wc->string_1,wcrl.buf+ovector[index*2],len);
            pthread_mutex_unlock(&wc->cmtx);

            if(wc->regexp)
            {
                int lovector[300]= {0};
                int lmatch_count=0;

                if(webget_apply_regexp(wc->string_1,len,wc->regexp,(int*)lovector,300,&lmatch_count)==0)
                {
                    if(wc->string_2_index<lmatch_count&&wc->string_2_index!=0)
                    {
                        pthread_mutex_lock(&wc->cmtx);
                        size_t index2=wc->string_2_index;
                        size_t len2=lovector[index2*2-1]-lovector[index2*2];
                        wc->string_2=zmalloc(len2+1);
                        strncpy(wc->string_2,wc->string_1+lovector[index2*2],len2);
                        pthread_mutex_unlock(&wc->cmtx);
                    }
                }
            }
        }
    }
    sfree((void**)&wcrl.buf);
    wcrl.buf_pos=0;
    wp->work=0;
    return(NULL);
}
*/
double webget2_update(void *spv)
{
    webget *w=spv;

    if(w->work==0&&w->dt)
    {
        pthread_join(w->dt,NULL);
        memset(&w->dt,0,sizeof(pthread_t));
    }
    if(w->parent==NULL&&w->dt==NULL)
    {
        w->work=1;
        pthread_attr_t th_att= {0};
        pthread_attr_init(&th_att);
        pthread_attr_setdetachstate(&th_att,PTHREAD_CREATE_JOINABLE);
        pthread_create(&w->dt, NULL, webget_worker, w);
        pthread_attr_destroy(&th_att);

        while(w->work==1)
        {
            sched_yield();
        }
    }

    return(0.0);
}

unsigned char *webget2_string(void *spv)
{
    webget *w=spv;
    /*
    webget_holder *w=spv;
    webget_child *wc=wh->type==webget_child_type?wh->wg:NULL;
    if(wc)
    {
        unsigned char *ret=NULL;

        pthread_mutex_lock(&wc->cmtx);
        ret=wc->string_2?wc->string_2:wc->string_1;
        pthread_mutex_unlock(&wc->cmtx);
        return(ret);
    }*/
    return(NULL);
}
