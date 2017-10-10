/* Ping source
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */

#include <SDK/extension.h>
#include <stdlib.h>
#include <stdio.h>
#include <Iphlpapi.h>
#include <Icmpapi.h>
#include <string.h>
#include <pthread.h>

typedef struct
{
    unsigned char th_active;
    unsigned char solve_active;
    unsigned char* exec;
    unsigned char *link;
    unsigned int addr;
    size_t timeout;
    size_t period;
    size_t current;
    size_t rep_timeout;
    size_t th;
    void* ip;
    double ping_val;
    pthread_mutex_t mutex;
} ping;


static void *ping_solve_address(void *spv)
{
    ping* p = spv;
    p->solve_active=1;
    unsigned int addr=0;
    unsigned char buf[256]= {0};
    pthread_mutex_lock(&p->mutex);

    strncpy(buf,p->link,255);
    pthread_mutex_unlock(&p->mutex);

    addr = inet_addr(buf);

    if(addr == INADDR_NONE)
    {
        WSADATA wsaData= {0};

        if(WSAStartup(0x0101, &wsaData) == 0)
        {
            struct hostent* host = gethostbyname(buf);
            if(host)
            {
                addr = *(DWORD*)host->h_addr;
            }
            WSACleanup();
        }
    }

    pthread_mutex_lock(&p->mutex);
    p->addr=addr;
    pthread_mutex_unlock(&p->mutex);
    p->solve_active=0;
    return(NULL);
}

static void *ping_calculate(void *spv)
{
    ping* p = spv;
    p->th_active = 1;
    unsigned int addr=0;
    unsigned int timeout=0;
    unsigned int def_timeout=0;

    pthread_mutex_lock(&p->mutex);
    addr=p->addr;
    timeout=p->timeout;
    def_timeout=p->rep_timeout;
    pthread_mutex_unlock(&p->mutex);

    if(p->addr==INADDR_NONE||p->addr==0)
    {
        p->ping_val = -1;
        p->th_active = 0;

        pthread_exit(NULL);
    }

    unsigned int buf_sz = sizeof(ICMP_ECHO_REPLY32);
    ICMP_ECHO_REPLY32 buf = { 0 };
    void *icmp_handle= IcmpCreateFile();

    if(icmp_handle != INVALID_HANDLE_VALUE)
    {
        if(IcmpSendEcho(icmp_handle, addr, NULL, 0, NULL, &buf, buf_sz, timeout))
        {
            p->ping_val = (double)buf.RoundTripTime;
        }
        else
        {
            p->ping_val = (double)def_timeout;
        }
        IcmpCloseHandle(icmp_handle);
    }
    else
    {
        p->ping_val = (double)def_timeout;
    }

    p->th_active = 0;
    return(NULL);
}
void extension_init_func(void** spv, void* ip)
{
    ping* p = malloc(sizeof(ping));
    memset(p, 0, sizeof(ping));
    pthread_mutexattr_t mutex_attr=0;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_RECURSIVE_NP);

    pthread_mutex_init(&p->mutex,&mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);
    p->ip = ip;
    *spv = p;
}

void extension_reset_func(void* spv, void* ip)
{
    ping* p = spv;
    unsigned char* t=NULL;
    pthread_mutex_lock(&p->mutex);
    free(p->link);
    p->link=NULL;

    if((t = extension_string("Address", 0x3, ip, "127.0.0.1")) != NULL)
    {
        p->link=strdup(t);
    }

    free(p->exec);
    p->exec = NULL;
    p->timeout = extension_size_t("Timeout", ip, 30000);
    p->period = extension_size_t("Period", ip, 64);
    p->rep_timeout = extension_size_t("TimeoutValue", ip, 30000);
    pthread_mutex_unlock(&p->mutex);
    if((t = extension_string("FinishAction", 0x3, ip, NULL))!=NULL)
    {
        p->exec = strdup(t);
    }

    if(p->solve_active==0)
    {
        size_t solve_th=0;
        pthread_attr_t th_att= {0};
        pthread_attr_init(&th_att);
        pthread_attr_setdetachstate(&th_att,PTHREAD_CREATE_DETACHED);
        pthread_create(&solve_th,&th_att,ping_solve_address,spv);
        pthread_attr_destroy(&th_att);
    }
}

double extension_update_func(void* spv)
{
    ping* p = spv;

    pthread_mutex_lock(&p->mutex);
    unsigned int addr=p->addr;
    pthread_mutex_unlock(&p->mutex);

    if(addr!=0&&addr!=-1)
    {
        if(p->current == 0&&p->th==0)
        {
            p->th_active = 1;
            pthread_create(&p->th, NULL, ping_calculate, p);

        }
        else if(p->th_active == 0 && p->th)
        {
            pthread_join(p->th, NULL);
            extension_send_command(p->ip, p->exec);
            p->th = 0;
        }
        if(++p->current == p->period)
            p->current = 0;
    }
    else if(p->solve_active==0)
    {
        size_t solve_th=0;
        pthread_create(&solve_th,NULL,ping_solve_address,spv);
        pthread_detach(solve_th);
    }

    return (p->ping_val);
}

void extension_destroy_func(void** spv)
{
    ping* p = *spv;

    if(p->th)
    {
        pthread_join(p->th, NULL);
        p->th = 0;
    }

    pthread_mutex_unlock(&p->mutex); //Unlocking
    pthread_mutex_destroy(&p->mutex); //destroying

    free(p->exec);
    free(*spv);
    *spv = NULL;
}
