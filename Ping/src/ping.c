/* Ping source
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */

#ifdef WIN32
#include <stdlib.h>
#include <stdio.h>
#include <Iphlpapi.h>
#include <Icmpapi.h>
#include <string.h>
#include <pthread.h>

typedef struct
{
    void *th_active;
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

static void *ping_calculate(void *spv);

void init(void** spv, void* ip)
{
    ping* p = malloc(sizeof(ping));
    memset(p, 0, sizeof(ping));
    pthread_mutexattr_t mutex_attr=0;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&p->mutex,&mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);
    p->th_active=semper_safe_flag_init();
    p->ip = ip;
    *spv = p;
}

void reset(void* spv, void* ip)
{
    ping* p = spv;
    unsigned char* t=NULL;
    pthread_mutex_lock(&p->mutex);
    free(p->link);
    p->link=NULL;

    if((t = param_string("Address", 0x3, ip, "127.0.0.1")) != NULL)
    {
        p->link=strdup(t);
    }

    free(p->exec);
    p->exec = NULL;
    p->addr = 0;
    p->timeout = param_size_t("Timeout", ip, 30000);
    p->period = param_size_t("Period", ip, 64);
    p->rep_timeout = param_size_t("TimeoutValue", ip, 30000);

    if((t = param_string("FinishAction", 0x3, ip, NULL))!=NULL)
    {
        p->exec = strdup(t);
    }
    pthread_mutex_unlock(&p->mutex);
}

double update(void* spv)
{
    ping* p = spv;

    if(p->current == 0&&p->th==0)
    {
        semper_safe_flag_set(p->th_active, 1);
        int status=pthread_create(&p->th, NULL, ping_calculate, p);

        if(!status)
        {
            while(semper_safe_flag_get(p->th_active)==1);
        }
        else
        {
            semper_safe_flag_set(p->th_active, 0);
            diag_error("Failed to start ping thread");
        }
    }
    else if(semper_safe_flag_get(p->th_active) == 0 && p->th)
    {
        pthread_join(p->th, NULL);
        p->th = 0;
    }
    if(++p->current == p->period)
        p->current = 0;

    return (p->ping_val);
}

void destroy(void** spv)
{
    ping* p = *spv;

    if(p->th)
    {
        pthread_join(p->th, NULL);
        p->th = 0;
    }

    pthread_mutex_destroy(&p->mutex); //destroying
    semper_safe_flag_destroy(&p->th_active);
    free(p->exec);
    free(*spv);
    *spv = NULL;
}

static void *ping_calculate(void *spv)
{
    ping* p = spv;
    semper_safe_flag_set(p->th_active, 2);
    unsigned int addr=0;
    unsigned int timeout=0;
    unsigned int def_timeout=0;
    unsigned char *finish_act=NULL;
    pthread_mutex_lock(&p->mutex);
    addr=p->addr;
    timeout=p->timeout;
    def_timeout=p->rep_timeout;
    finish_act=strdup(p->exec);
    pthread_mutex_unlock(&p->mutex);

    if(p->addr==INADDR_NONE||p->addr==0)
    {
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

        if(addr==INADDR_NONE||addr==0)
        {
            pthread_mutex_lock(&p->mutex);
            p->ping_val = -1;
            p->addr=0;
            semper_safe_flag_set(p->th_active,0);
            free(finish_act);
            pthread_mutex_unlock(&p->mutex);
            return(NULL);
        }

        pthread_mutex_lock(&p->mutex);
        p->addr=addr;
        pthread_mutex_unlock(&p->mutex);
    }

    unsigned int buf_sz = sizeof(ICMP_ECHO_REPLY32);
    ICMP_ECHO_REPLY32 buf = { 0 };
    void *icmp_handle= IcmpCreateFile();

    if(icmp_handle != INVALID_HANDLE_VALUE)
    {
        if(IcmpSendEcho(icmp_handle, addr, NULL, 0, NULL, &buf, buf_sz, timeout))
        {
            p->ping_val = (double)buf.RoundTripTime;
            send_command(p->ip, finish_act);
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


    free(finish_act);
    semper_safe_flag_set(p->th_active, 0);
    return(NULL);
}


#endif
