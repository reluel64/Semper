/* Ping source
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */


/*
 * The Ping source uses TCP SYN/ACK
 * pinging method because the normal method
 * using ICMP Echo requires raw sockets under Linux
 * The issue is that creating a raw socket requires
 * root access. Therefore instead of requiring root
 * access we try to do a compromise and use TCP SYN/ACK
 */

#if defined(WIN32)
#include <Winsock2.h>
#include <windows.h>

#include <Iphlpapi.h>
#include <Icmpapi.h>
#include <Ws2tcpip.h>
#elif defined(__linux__)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <semper_api.h>
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
        pthread_t th;
        void* ip;
        double ping_val;
        pthread_mutex_t mutex;
        int port;
        int sock;
} ping;


#if defined(WIN32)
PCSTR WSAAPI inet_ntop(
        INT        Family,
        const VOID *pAddr,
        PSTR       pStringBuf,
        size_t     StringBufSize
);
#endif

static void *ping_calculate(void *spv);

void ping_init(void** spv, void* ip)
{
#if defined(WIN32)
    WSADATA wsaData= {0};
    WSAStartup(0x0101, &wsaData);
#endif
    ping* p = malloc(sizeof(ping));
    memset(p, 0, sizeof(ping));
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&p->mutex,&mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);
    p->th_active=semper_safe_flag_init();
    p->ip = ip;
    *spv = p;
}

void ping_reset(void* spv, void* ip)
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
    p->port = param_size_t("Port",ip,80);
    if((t = param_string("FinishAction", 0x3, ip, NULL))!=NULL)
    {
        p->exec = strdup(t);
    }
    pthread_mutex_unlock(&p->mutex);
}

double ping_update(void* spv)
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

void ping_destroy(void** spv)
{
    ping* p = *spv;
    pthread_mutex_lock(&p->mutex);
    if(p->sock>=0)
#if defined(WIN32)
        shutdown(p->sock,SD_BOTH); /*this should wake up the thread*/
#elif defined(__linux__)
    shutdown(p->sock,SHUT_RDWR); /*this should wake up the thread*/
#endif
    pthread_mutex_unlock(&p->mutex);


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
#if defined(WIN32)
    WSACleanup();
#endif
}

static void *ping_calculate(void *spv)
{
    ping* p = spv;
    semper_safe_flag_set(p->th_active, 2);
    struct addrinfo hint;
    struct addrinfo *res = NULL;
    struct in_addr *addr = NULL;
    unsigned char ip_addr[64]={0};
    unsigned char link[256]= {0};
    unsigned int def_timeout=0;

    unsigned char *finish_act=NULL;
    unsigned char port[32] ={0};
    int sock_fd = 0;
    struct timeval end;
   // struct timeval start;

    struct hostent *host = NULL;
    int status = 0;
   // fd_set rset;
   // fd_set wset;
   // FD_ZERO(&rset);
  //  FD_ZERO(&wset);
    pthread_mutex_lock(&p->mutex);

    def_timeout=p->rep_timeout;
    if(p->exec)
    {
        finish_act=strdup(p->exec);
    }
    strncpy(link,p->link,255);
    end.tv_sec = p->timeout / 1000;
    end.tv_usec = (p->timeout % 1000) * 1000;
   // start.tv_sec = p->timeout / 1000;
   // start.tv_usec = (p->timeout % 1000) * 1000;
    snprintf(port,32,"%d",p->port);
    pthread_mutex_unlock(&p->mutex);

    memset(&hint,0,sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = 0;
    hint.ai_protocol = 0;

    host = gethostbyname(link);



    if(host == NULL)
    {
        pthread_mutex_lock(&p->mutex);
        p->ping_val = -1;
        semper_safe_flag_set(p->th_active,0);
        free(finish_act);
        pthread_mutex_unlock(&p->mutex);
        return(NULL);
    }

    addr = (struct in_addr*)host->h_addr_list[0];

    if((inet_ntop(AF_INET,addr,(char*)&ip_addr,64) == NULL) || getaddrinfo(ip_addr,port, &hint, &res))
    {
        pthread_mutex_lock(&p->mutex);
        p->ping_val = def_timeout;
        semper_safe_flag_set(p->th_active,0);
        free(finish_act);
        pthread_mutex_unlock(&p->mutex);

        return(NULL);
    }


    sock_fd = socket(AF_INET,SOCK_STREAM ,0);

    if(sock_fd < 0)
    {
        pthread_mutex_lock(&p->mutex);
        p->ping_val = -1;
        semper_safe_flag_set(p->th_active,0);
        free(finish_act);
        pthread_mutex_unlock(&p->mutex);
        freeaddrinfo(res);
        return(NULL);
    }

    pthread_mutex_lock(&p->mutex);
    p->sock = sock_fd;
    pthread_mutex_unlock(&p->mutex);

   // FD_SET(sock_fd,&wset);
  //  FD_SET(sock_fd,&rset);
    setsockopt (sock_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&end, sizeof(end));
    setsockopt (sock_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&end, sizeof(end));
#if 0
#if defined(WIN32)
    unsigned long nblocking = 1;
    ioctlsocket(sock_fd, FIONBIO, &nblocking);
#elif defined(__linux__)
    int flags = fcntl(sock_fd, F_GETFL, 0);
    flags |=O_NONBLOCK;
    fcntl(sock_fd, F_SETFL, flags);
#endif
#endif

    struct timespec t1 = {0};
    struct timespec t2 = {0};

    clock_gettime(CLOCK_MONOTONIC, &t1);
    status = connect(sock_fd,res->ai_addr,res->ai_addrlen);
    clock_gettime(CLOCK_MONOTONIC, &t2);
    //status = select(sock_fd+1,&rset,&wset,NULL,&end);

    if(status == 0 || status == 111)
    {
        pthread_mutex_lock(&p->mutex);
#if 0
        if(status > 0)
            p->ping_val =(double) ((start.tv_sec - end.tv_sec) * 1000 + ((start.tv_usec - end.tv_usec)/1000));
        else
            p->ping_val = (double) def_timeout;
#else
        p->ping_val = (double)((t2.tv_nsec - t1.tv_nsec) / 1000000 + (t2.tv_sec - t1.tv_sec) * 1000);
#endif
        pthread_mutex_unlock(&p->mutex);

        send_command(p->ip, finish_act);
    }
    else
    {
        p->ping_val = -1;
    }

    pthread_mutex_lock(&p->mutex);
    p->sock = -1;
    pthread_mutex_unlock(&p->mutex);
    free(finish_act);
    semper_safe_flag_set(p->th_active, 0);
    freeaddrinfo(res);
#if defined(WIN32)
    shutdown(p->sock,SD_BOTH); /*this should wake up the thread*/
    closesocket(sock_fd);
#elif defined(__linux__)
    shutdown(p->sock,SHUT_RDWR); /*this should wake up the thread*/
    close(sock_fd);
#endif


    return(NULL);
}

