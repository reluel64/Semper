/* WebGet source
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */

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
#define BUFFER_SIZE 1048576 //1MB

typedef struct
{
    FILE *fd;               		//it will be used in case the buffer is bigger than in_memory_buffer_size
    unsigned char *buf;           	//the actual buffer
    size_t buf_sz;              	//allocated memory for the buffer
    size_t rec_sz;              	//if this is bigger than BUFFER_SIZE, the buffer will be dumped into a file
    size_t buf_pos;             	//current buffer position
    unsigned char dwl;          	//writer mode
} curl_writer_state;


typedef struct _webget_data webget_data;

typedef struct _webget_data
{
    surface_data *sd;
    unsigned char *url;                 //this acts both as a url or as a string index
    unsigned char *regexp;              //regular expression

    unsigned char **substr;             //substrings
    unsigned char *dwl_fail_act;
    unsigned char *parse_fail_act;
    unsigned char *connect_fail_act;
    unsigned char *success_act;
    unsigned char *str_ret;				//holder for strings that are returned
    unsigned char dwl_act;				//downloader active flag
    unsigned char to_file;
    size_t substr_index;                //substring index
    size_t substr_index2;               //substring index 2 (valid for children)
    size_t download_rate;               //target value for counter
    size_t current_rate;                //current counter
    size_t substr_count;                //substring count
    size_t in_memory_buffer_size;       //size for the buffer allocated for the downloader thread

    unsigned char *data_buffer;              //working buffer (the result of downloader)
    size_t data_buf_len;                     //buffer length
    unsigned char *data_link;               //the address to retreive data from
    pthread_mutex_t mutex;              //mutex to protect parameters from being altered while accessing them
    pthread_mutex_t dwl_mutex;          //mutex to prevent two downloaders to run at the same time
    webget_data *parent;               //parent (if source is a child)
    size_t update_cycle;				//valid only for parent
    void *ip;                           //internal pointer (used by extension_* routines)

} webget_data;

static FILE *webget_create_file(unsigned char *root,unsigned char **outf)
{
    static size_t oldr=0;
    static size_t otime=0;
    size_t c_time=time(NULL);
    FILE *fh=NULL;
    size_t rlen=string_length(root)+sizeof("webget")+260;
    unsigned char *dir=zmalloc(rlen);
    snprintf(dir,rlen,"%s/webget",root);
#ifdef WIN32
    unsigned short *uc=utf8_to_ucs(dir);
    _wmkdir(uc);
    sfree((void**)&uc);
#elif __linux__
    mkdir(dir,S_IRUSR | S_IWUSR  | S_IRGRP | S_IWGRP  | S_IROTH | S_IWOTH );
#endif

    if(otime==0)
    {
        otime=time(NULL);
    }
    else
    {
        if(c_time!=otime)
        {
            oldr=0;
        }
        else
        {
            oldr++;
        }
    }

    snprintf(dir,rlen,"%s/webget/%llu-%llu.tmp",root,otime,oldr);
    otime=c_time;
    //create the file
#ifdef WIN32
    uc=utf8_to_ucs(dir);
    fh=_wfopen(uc,L"wb");
    sfree((void**)&uc);
#elif __linux__
    fh=fopen(dir,"wb");
#endif

    if(fh==NULL)
    {
        sfree((void**)&dir);
        return(NULL);
    }
    *outf=dir;
    return(fh);
}
/*
static void webget_delete_file(unsigned char *f)
{
#ifdef WIN32
    unsigned short *uc=utf8_to_ucs(f);
    _wremove(uc);
    sfree((void**)&uc);
#elif __linux__
    remove(f);
#endif
}
*/
size_t write_callback(char *buf, size_t size, size_t nmemb, void *pv)
{
    curl_writer_state *cws=pv;
    size_t ret=0;

//------------------------------

    if(cws->dwl==0)
    {
        if(cws->buf_pos+size*nmemb<=cws->buf_sz)
        {
            memcpy(cws->buf+cws->buf_pos,buf,size*nmemb);
            cws->buf_pos+=size*nmemb;
        }
        ret=size*nmemb;
    }
    else
    {
        ret= fwrite(buf,size,nmemb,cws->fd);
        fflush(cws->fd);
    }
    return(ret);
}


void webget_init(void **spv,void *ip)
{
    unused_parameter(ip);
    webget_data *wd=zmalloc(sizeof(webget_data));
    wd->sd=((source*)ip)->sd;
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&wd->mutex,&mutex_attr);
    pthread_mutex_init(&wd->dwl_mutex,&mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);
    *spv=wd;
}

void webget_reset(void *spv,void *ip)
{
    webget_data *wd=spv;
    pthread_mutex_lock(&wd->mutex);

    sfree((void**)&wd->url);
    sfree((void**)&wd->regexp);
    sfree((void**)&wd->success_act);
    sfree((void**)&wd->parse_fail_act);
    sfree((void**)&wd->dwl_fail_act);
    wd->url=clone_string(extension_string("URL",EXTENSION_XPAND_VARIABLES,ip,NULL));

    void *parent=extension_get_parent(wd->url,ip);

    if(parent)
    {
        wd->parent=extension_private(parent);
        sfree((void**)&wd->url);
    }

    wd->success_act=clone_string(extension_string("SuccessAction",0x3,ip,NULL));
    wd->parse_fail_act=clone_string(extension_string("ParseFailAction",0x3,ip,NULL));
    wd->dwl_fail_act=clone_string(extension_string("DownloadFailAction",0x3,ip,NULL));
    wd->current_rate=wd->download_rate=extension_size_t("DownloadRate",ip,600);
    wd->in_memory_buffer_size=extension_size_t("MaxInMemoryBuffer",ip,BUFFER_SIZE);

    wd->regexp=clone_string(extension_string("RegExp",0x3,ip,NULL));
    wd->substr_index=extension_size_t("StringIndex",ip,1);
    wd->substr_index2=extension_size_t("StringIndex2",ip,(size_t)-1);
    wd->to_file=extension_bool("Download",ip,0);
    wd->ip=ip;
    pthread_mutex_unlock(&wd->mutex);
}

static int webget_parse(unsigned char *buffer,unsigned char *regexp,size_t *matches,unsigned char ***out_buf)
{
    if(regexp==NULL||buffer==NULL||matches==NULL||out_buf==NULL)
    {
        return(-1);
    }

    int err_off = 0;
    const char* err = NULL;
    int match_count = 0;
    int ovector[300] = { 0 };


    pcre* pc = pcre_compile((char*)regexp, 0, &err, &err_off, NULL);

    if(pc==NULL)
    {
        return(-2);
    }

    int buf_sz=string_length(buffer);
    match_count = pcre_exec(pc, NULL, (char*)buffer, buf_sz, 0, 0, ovector, sizeof(ovector) / sizeof(int)); /*good explanation at http://libs.wikia.com/wiki/Pcre_exec */
    pcre_free(pc);

    if(match_count < 1)
    {
        return (-3);
    }

    *out_buf=zmalloc(sizeof(unsigned char *)*match_count);
    *matches=match_count;

    for(int i=0; i<match_count; i++)
    {
        size_t offset=ovector[2*i];
        size_t str_len=ovector[(2*i)+1]-ovector[(2*i)];
        (*out_buf)[i]=zmalloc(str_len+1);
        strncpy((*out_buf)[i],buffer+offset,str_len);
    }

    return(0);
}


static void *webget_download(void *spv) /*download and/or parse*/
{
    int ret=1;
    webget_data *wd=spv;
    unsigned char *fn=NULL;
    wd->dwl_act=1;

    pthread_mutex_lock(&wd->dwl_mutex);
    CURL *c_state=curl_easy_init();
    curl_writer_state cws= {0};
    cws.dwl=wd->to_file;

    if(wd->to_file)
    {
        cws.fd=webget_create_file(wd->sd->sp.surface_dir,&fn);
    }

    if(c_state)
    {
        double clength=0.0;
        pthread_mutex_lock(&wd->mutex);
        curl_easy_setopt(c_state,CURLOPT_URL,wd->data_link);
        pthread_mutex_unlock(&wd->mutex);

        curl_easy_setopt(c_state,CURLOPT_WRITEFUNCTION,write_callback);
        curl_easy_setopt(c_state,CURLOPT_WRITEDATA,&cws);
        curl_easy_setopt(c_state,CURLOPT_FOLLOWLOCATION,1);
        curl_easy_setopt(c_state,CURLOPT_MAXREDIRS,4096);
        curl_easy_setopt(c_state, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(c_state, CURLOPT_NOBODY, 1);

        ret=curl_easy_perform(c_state);

        if(!ret)
        {
            curl_easy_setopt(c_state, CURLOPT_NOBODY, 0);
            curl_easy_getinfo(c_state,CURLINFO_CONTENT_LENGTH_DOWNLOAD,&clength);

            if(wd->to_file==0)
            {
                if(clength>0.0)
                {
                    cws.buf=zmalloc((size_t)clength+1);
                    cws.buf_sz=(size_t)clength;
                }
                else
                {
                    cws.buf=zmalloc((size_t)wd->in_memory_buffer_size+1);
                    cws.buf_sz=(size_t)wd->in_memory_buffer_size;
                }
            }

            ret=curl_easy_perform(c_state);
            curl_easy_cleanup(c_state);
        }
    }

    pthread_mutex_lock(&wd->mutex);
    sfree((void**)&wd->data_buffer);

    if(wd->to_file==1)
    {
        wd->data_buffer=fn;
        wd->data_buf_len=string_length(fn);
    }

    if(wd->substr)
    {
        for(size_t i=0; i<wd->substr_count; i++)
        {
            sfree((void**)&wd->substr[i]);
        }
        sfree((void**)&wd->substr);
    }

    wd->substr_count=0;

    if(wd->to_file==0)
    {
        if(!ret)
        {
            wd->data_buffer=cws.buf;
            wd->data_buf_len=cws.buf_sz;

            if(webget_parse(wd->data_buffer,wd->regexp,&wd->substr_count,&wd->substr))
            {
                extension_send_command(wd->ip,wd->parse_fail_act);
            }
            else
            {
                extension_send_command(wd->ip,wd->success_act);
                wd->update_cycle++;
            }
        }

    }
    else if(!ret)
    {
        fclose(cws.fd);
    }
    else
    {
        extension_send_command(wd->ip,wd->dwl_fail_act);
        if(cws.buf)
        {
            sfree((void**)&cws.buf);
        }
    }
    wd->current_rate=0;
    pthread_mutex_unlock(&wd->mutex);

    wd->dwl_act=2;
    pthread_mutex_unlock(&wd->dwl_mutex);
    return(NULL);
}


int webget_parent_download(webget_data *wd)
{
    /*Start the downloader*/
    pthread_t dtid=0; //dummy thread id
    pthread_attr_t th_att= {0};
    if(wd->current_rate>=wd->download_rate&&wd->dwl_act==0)
    {
        wd->data_link=wd->url;
        pthread_attr_init(&th_att);
        pthread_attr_setdetachstate(&th_att,PTHREAD_CREATE_DETACHED);
        pthread_create(&dtid,&th_att,webget_download,wd);
        pthread_attr_destroy(&th_att);
    }
    else if(wd->dwl_act==2)
    {
        wd->update_cycle++;
        wd->dwl_act=0;
        return(0);
    }
    else
    {
        return(0);
    }

    pthread_mutex_lock(&wd->mutex);
    sfree((void**)&wd->str_ret);
    wd->str_ret=clone_string(wd->data_buffer);
    pthread_mutex_unlock(&wd->mutex);

    return(0);
}

int webget_child_download(webget_data *wd)
{
    pthread_t dtid=0; //dummy thread id
    pthread_attr_t th_att= {0};
    pthread_mutex_lock(&wd->parent->mutex);         //gain exclusive access

    if(wd->parent->update_cycle==wd->update_cycle||
       wd->parent->data_buffer==NULL             ||
       wd->parent->substr==NULL                  ||
       wd->parent->substr_count<wd->substr_index ||
       wd->parent->dwl_act==1)
    {
        pthread_mutex_unlock(&wd->parent->mutex);       //nothing interesting - release the lock
        return(0);
    }

    sfree((void**)&wd->str_ret);

    if(wd->substr_index2==(size_t)-1&&wd->to_file==0)
    {
        if(wd->substr_index<wd->parent->substr_count)
        {
            wd->str_ret=clone_string(wd->parent->substr[wd->substr_index]);
        }
        pthread_mutex_unlock(&wd->parent->mutex);
        return(0);
    }
    else
    {
        /*Clean the old values*/
        if(wd->substr)
        {
            for(size_t i=0; i<wd->substr_count; i++)
            {
                sfree((void**)&wd->substr[i]);
            }
            sfree((void**)&wd->substr);
        }

        if(wd->substr_index<wd->parent->substr_count)
        {
            if(wd->dwl_act==0)
            {
                wd->data_link=wd->parent->substr[wd->substr_index];
                pthread_attr_init(&th_att);
                pthread_attr_setdetachstate(&th_att,PTHREAD_CREATE_DETACHED);
                pthread_create(&dtid,&th_att,webget_download,wd);
                pthread_attr_destroy(&th_att);
                pthread_mutex_unlock(&wd->parent->mutex);
                return(0);
            }

            if(wd->substr==NULL)
            {
                if(wd->dwl_act==2)
                {
                    if(wd->substr==NULL)
                    {
                        webget_parse(wd->parent->substr[wd->substr_index],wd->regexp,&wd->substr_count,&wd->substr);
                    }
                    if(wd->substr==NULL) //we could not get substrings
                    {
                        wd->str_ret=clone_string(wd->data_buffer);
                    }

                    wd->dwl_act=0;
                }

                if(wd->str_ret==NULL&&wd->substr_count>wd->substr_index2)
                {
                    wd->str_ret=clone_string(wd->substr[wd->substr_index2]);
                }
            }
        }
    }
    if(wd->dwl_act==0)
    {
        wd->update_cycle=wd->parent->update_cycle;
    }
    pthread_mutex_unlock(&wd->parent->mutex);
    return(0);
}

double webget_update(void *spv)
{
    webget_data *wd=spv;
    double val=0.0;

    /*Deal with the parent*/
    if(wd->parent==NULL)
    {
        if(wd->dwl_act==0)
        {
            wd->current_rate++;
        }
        webget_parent_download(wd);
    }

    else if(wd->parent)
    {
        webget_child_download(wd);
    }

    return(val);
}

unsigned char *webget_string(void *spv)
{
    webget_data *wd=spv;
    return(wd->str_ret);
}


void webget_destroy(void**spv) //to be honest, I do not know how to clean this mess
{
    webget_data *wd=*spv;
    pthread_mutex_lock(&wd->dwl_mutex);
    pthread_mutex_unlock(&wd->dwl_mutex);
    pthread_mutex_destroy(&wd->dwl_mutex);
    pthread_mutex_destroy(&wd->mutex);
    sfree((void**)&wd->str_ret);
    sfree((void**)&wd->url);
    sfree((void**)&wd->regexp);
    sfree((void**)&wd->success_act);
    sfree((void**)&wd->parse_fail_act);
    sfree((void**)&wd->dwl_fail_act);
    sfree((void**)&wd->data_buffer);

    if(wd->substr)
    {
        for(size_t i=0; i<wd->substr_count; i++)
        {
            sfree((void**)&wd->substr[i]);
        }
        sfree((void**)&wd->substr);
    }

    sfree(spv);
}
