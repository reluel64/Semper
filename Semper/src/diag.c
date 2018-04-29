/*
 * Diagnostic messages provider
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */

#include <semper.h>
#include <diag.h>

#include <linked_list.h>
#include <string_util.h>
#include <mem.h>
#include <stdarg.h>
#define DIAG_MEM_ENTRY_LENGTH 65535

#ifdef WIN32
#ifdef SEMPER_API
#undef SEMPER_API
#define SEMPER_API __attribute__((dllexport))
#else
#define SEMPER_API __attribute__((dllexport))
#endif
#else
#define SEMPER_API
#endif
diag_status *diag_get_struct(void)
{
    static diag_status sts= {0};
    return(&sts);
}

int diag_init(control_data *cd)
{
    key k=NULL;
    diag_status *ds=diag_get_struct();
    ds->init=1;
    list_entry_init(&ds->mem_log);
    cd->diag_prov=ds;
    ds->cd=cd;
    ds->level=0;
    ds->max_mem_log=20;
    ds->ltf=0;

    ds->fp=zmalloc(cd->root_dir_length+20);
    snprintf(ds->fp,cd->root_dir_length+20,"%s/Semper.log",cd->root_dir);
    clock_gettime(CLOCK_MONOTONIC,&ds->t1);
#ifndef DEBUG
    if((k=skeleton_get_key(cd->smp,"LogLevel"))!=NULL)
    {
        ds->level =(unsigned char)compute_formula(skeleton_key_value(k));
    }

    if((k=skeleton_get_key(cd->smp,"LogToFile"))!=NULL)
    {
        ds->ltf =compute_formula(skeleton_key_value(k))!=0.0;
    }
#else
    ds->ltf=1;
    ds->level=0xf;
#endif
    if((k=skeleton_get_key(cd->smp,"LogMaxEntries"))!=NULL)
    {
        ds->max_mem_log=(size_t)compute_formula(skeleton_key_value(k));
    }

    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&ds->mutex,&mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);
    return(0);
}
#if 0
int diag_print(void)
{
    diag_status *ds=diag_get_struct();
    diag_mem_log *dml=NULL;
    list_enum_part_backward(dml,&ds->mem_log,current)
    {
        printf("%s\n",dml->log_buf);
    }
    return(0);
}
#endif
static void diag_open_file(diag_status *ds)
{
    if(ds->fh==NULL&&ds->fp)
    {
#ifndef DEBUG
#ifdef WIN32
        unsigned short *uc=utf8_to_ucs(ds->fp);
        ds->fh=_wfopen(uc,L"a+");
        sfree((void**)&uc);
#elif __linux__
        ds->fh=fopen(ds->fp,"a+");
#endif
#else
        ds->fh=stdout;
#endif
        if(ds->fh)
        {
            unsigned char ts[64]= {0};
            size_t utf8_bom = 0xBFBBEF;
            size_t check_bom=0;
            fread(&check_bom,1,3,ds->fh);
            fseek(ds->fh,0,0);
            tzset();
            time_t ct=time(NULL);
            struct tm *fmt_time=localtime(&ct);
            size_t date_len=strftime(ts,64,"Log started at %Y-%m-%d %H:%M",fmt_time);

            if(check_bom!=utf8_bom)
            {
#ifndef DEBUG
                fwrite(&utf8_bom,1,3,ds->fh);
#endif
            }
            else
            {
                fputc('\n',ds->fh);
                fputc('\n',ds->fh);
            }

            for(size_t i=0; i<32; i++)
            {
                if(i==16)
                {
                    fwrite(ts,1,date_len,ds->fh);
                }

                fputc('*',ds->fh);
            }

            fputc('\n',ds->fh);
        }
    }
}

static void diag_close_file(diag_status *ds)
{
    if(ds->fh)
    {
#ifndef DEBUG
        fclose(ds->fh);
#endif
        ds->fh=NULL;
    }
}

void diag_end(diag_status *ds)
{
    ds->cd=NULL;
    pthread_mutex_lock(&ds->mutex);
    diag_mem_log *tdml=NULL;
    diag_mem_log *dml=NULL;
    ds->init=0;
    list_enum_part_safe(dml,tdml,&ds->mem_log,current)
    {
        sfree((void**)&dml->log_buf);
        sfree((void**)&dml);
    }
    diag_close_file(ds);

    sfree((void**)&ds->fp);

    pthread_mutex_unlock(&ds->mutex);
    pthread_mutex_destroy(&ds->mutex);

}

static int diag_log_write_to_file(unsigned char *buf,size_t buf_len)
{
    diag_status *ds=diag_get_struct();
    diag_open_file(ds);

    if(ds->fh)
    {
        fwrite(buf,1,buf_len,ds->fh);
        fputc('\n',ds->fh);
        fflush(ds->fh);
        return(0);
    }

    return(-1);
}


SEMPER_API int diag_log(unsigned char lvl,char *fmt, ...)
{

    diag_status *ds=diag_get_struct();

    if(!(lvl&ds->level))
    {
        return(-5); //not set
    }

    if(ds->cd==NULL)
    {
        return(-2); /*log not activated*/
    }

    if(ds->init==0)
    {
        return(-3); //it looks like the library was not initialized
    }

    int r=0;
    size_t buf_start=0;
    pthread_mutex_lock(&ds->mutex);
    unsigned char buf[DIAG_MEM_ENTRY_LENGTH]= {0};

    struct timespec t2= {0};
    clock_gettime(CLOCK_MONOTONIC,&t2);
    buf_start=snprintf(buf,DIAG_MEM_ENTRY_LENGTH,"[%.8llu.%.4lu] ",
                       t2.tv_sec-ds->t1.tv_sec,
                       (t2.tv_nsec>ds->t1.tv_nsec?t2.tv_nsec-ds->t1.tv_nsec:0)/1000000);


    va_list ifmt;
    va_start(ifmt,fmt);
    r=vsnprintf(buf+buf_start,DIAG_MEM_ENTRY_LENGTH-buf_start,fmt,ifmt);
    va_end(ifmt);

    if(r<0)
    {
        pthread_mutex_unlock(&ds->mutex);
        return(-1);
    }

    if(ds->mem_log_elem<ds->max_mem_log)
    {
        diag_mem_log *l=zmalloc(sizeof(diag_mem_log));
        l->log_buf=clone_string(buf);
        list_entry_init(&l->current);
        linked_list_add(&l->current,&ds->mem_log);
        ds->mem_log_elem++;
    }
    else
    {
        if(linked_list_empty(&ds->mem_log)==0)
        {
            diag_mem_log *l=element_of(ds->mem_log.prev,diag_mem_log,current);
            linked_list_remove(&l->current);
            sfree((void**)&l->log_buf);
            sfree((void**)&l);
            l=zmalloc(sizeof(diag_mem_log));
            l->log_buf=clone_string(buf);
            list_entry_init(&l->current);
            linked_list_add(&l->current,&ds->mem_log);
        }
    }

    if(ds->ltf)
    {
        diag_log_write_to_file(buf,r+buf_start);
    }

    pthread_mutex_unlock(&ds->mutex);
    return(0);
}
