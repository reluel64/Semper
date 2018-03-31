/*
 * Memory based I/O
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <semper.h>
#include <memf.h>
#include <mem.h>
struct _memf
{
    size_t pos;
    size_t len;
    unsigned char *buf;
    pthread_mutex_t mtx;
};

memf *mopen(size_t init)
{
    memf *m=calloc(sizeof(memf),1);


    if(m)
    {
        if(init!=0)
        {
            m->buf=calloc(init,1);
            m->len=init;
        }
        pthread_mutexattr_t mutex_attr;
        pthread_mutexattr_init(&mutex_attr);
        pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&m->mtx,&mutex_attr);
        pthread_mutexattr_destroy(&mutex_attr);

    }
    return(m);
}


size_t mwrite(const void *buf,size_t size,size_t count,memf *mf)
{
    size_t blen=size*count;
    pthread_mutex_lock(&mf->mtx);
    if(mf->pos+blen>=mf->len)
    {
        unsigned char *nbuf=realloc(mf->buf,mf->pos+blen+5);
        nbuf[mf->pos+blen]=0;
        mf->len=mf->pos+blen;
        mf->buf=nbuf;
    }

    memcpy(mf->buf+mf->pos,buf,blen);
    mf->pos+=blen;

    pthread_mutex_unlock(&mf->mtx);
    return(blen);
}


size_t mread(void *buf,size_t size,size_t count,memf *mf)
{
    pthread_mutex_lock(&mf->mtx);
    size_t blen=size*count;
    size_t to_read=min(mf->len-mf->pos,blen);

    memcpy(buf,mf->buf+mf->pos,to_read);
    mf->pos+=to_read;
    pthread_mutex_unlock(&mf->mtx);
    return(to_read);
}

size_t mseek(memf *mf,long off,int origin)
{
    pthread_mutex_lock(&mf->mtx);
    int ret=-1;
    switch(origin)
    {
    case SEEK_SET:
        if(off<mf->len)
        {
            mf->pos=off;
            ret=0;
        }
        break;
    case SEEK_CUR:
        if(mf->pos+off<mf->len)
        {
            mf->pos+=off;
            ret=0;
        }
        break;
    case SEEK_END:
        if(mf->len+off<=mf->len)
        {
            mf->pos=mf->len+off;
            ret=0;
        }
        break;
    }

    pthread_mutex_unlock(&mf->mtx);
    return(ret);
}

int mprintf(memf *mf,const char *format,...)
{

    pthread_mutex_lock(&mf->mtx);
    va_list args;
    va_start(args,format);
    int len=vsnprintf(NULL,0,format,args);
    va_end(args);
    if(len>0)
    {
        va_start(args,format);
        void *b=calloc(len+1,1);
        vsnprintf(b,len+1,format,args);
        mwrite(b,len,1,mf);
        free(b);
        va_end(args);
        pthread_mutex_unlock(&mf->mtx);

        return(len);
    }
    pthread_mutex_unlock(&mf->mtx);
    return(-1);
}


char *mgets(char *str,int num,memf *mf)
{
    if(num==0||str==NULL||mf==NULL)
        return(NULL);

    unsigned char b=0;
    int rd=0;
    while(mread(&b,1,1,mf))
    {
        str[rd++]=b;
        str[rd]=0;
        if(rd>=num-1||b=='\n')
            return(str);
    }
    if(rd==0)
        return(NULL);
    return(str);
}

long int mtell(memf *mf)
{
    if(mf)
    {
        return(mf->pos);
    }
    return(-1);
}

void mclose(memf **mf)
{
    memf *lmf=*mf;
    pthread_mutex_destroy(&lmf->mtx);
    sfree((void**)&lmf->buf);
    sfree((void**)mf);
}
