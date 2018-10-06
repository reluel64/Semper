/* External command listener
*/

#include <semper.h>
#include <event.h>
#include <surface.h>
#include <mem.h>
#include <string_util.h>
#ifdef __linux__
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#endif
typedef struct
{
    char buf[32 * 1024];
#ifdef __linux__
  sem_t sem_wake;
    sem_t sem_write;
    size_t timestamp; //we will use this under Linux to avoid a nasty limitation
#endif
} listener_data;

typedef struct
{
    surface_data *sd;
    unsigned char *cmd;
} semper_listener_callback_data;



typedef struct
{
    #ifdef WIN32
    void *sem_wake;
    void *sem_write;
    #endif
    void *mmap;
    listener_data *ld;
    event_queue *eq;
    event_queue *meq;
    pthread_t th;
    surface_data *dummy;
    void *kill;
} semper_listener_data;

static size_t semper_listener_wait_fcn(void *pv, size_t timeout)
{
    semper_listener_data *sld = pv;
    #ifdef WIN32
    WaitForSingleObject(sld->sem_wake, -1);
    #elif __linux__
    sem_wait(&sld->ld->sem_wake);
    #endif
    return(0);
}

static void semper_listener_wake_fcn(void *pv)
{
    semper_listener_data *sld = pv;
#ifdef WIN32
    ReleaseSemaphore(sld->sem_wake, 1, NULL);
#elif __linux__
      sem_post(&sld->ld->sem_wake);
#endif
}


static int semper_listener_callback(semper_listener_callback_data *slcd)
{
    if(!slcd)
    {
        return (-1);
    }

    command(slcd->sd, &slcd->cmd);
    sfree((void**)&slcd->cmd);
    sfree((void**)&slcd);
    return (0);
}

static int semper_listener_prepare(void *pv)
{
    semper_listener_data *sld = pv;
    semper_listener_callback_data* slcd = zmalloc(sizeof(semper_listener_callback_data));
    slcd->sd = sld->dummy;
    slcd->cmd = zmalloc(32 * 1024 + 4);
    memcpy(slcd->cmd, sld->ld->buf, 32 * 1024);
    memset(sld->ld->buf, 0, 32 * 1024);
    event_push(sld->meq, (event_handler)semper_listener_callback, (void*)slcd, 0, 0); //we will queue this event to be processed later
#ifdef WIN32
    ReleaseSemaphore(sld->sem_write, 1, NULL);
#elif __linux__
     sem_post(&sld->ld->sem_write);
#endif
    return(0);
}



static void *semper_listener_thread(void *p)
{
    semper_listener_data *sld = p;

    while(safe_flag_get(sld->kill) == 0)
    {
        event_push(sld->eq, semper_listener_prepare, sld, 0, EVENT_NO_WAKE);
        event_wait(sld->eq);
        event_process(sld->eq);
        if(safe_flag_get(sld->kill))
        {
            break;
        }
    }

    return(NULL);
}

void *semper_listener_init(control_data *cd)
{

    semper_listener_data *sld = zmalloc(sizeof(semper_listener_data));
#ifdef WIN32
    sld->mmap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(listener_data), "Local\\SemperCommandListener");
    sld->sem_wake = CreateSemaphoreA(NULL, 0, 1, "Local\\SemperCommandListenerWake");
    sld->sem_write = CreateSemaphoreA(NULL, 1, 1, "Local\\SemperCommandListenerWrite");
    sld->ld = (listener_data*) MapViewOfFile(sld->mmap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(listener_data));
#elif __linux__
    unsigned char *usr=expand_env_var("$LOGNAME");
    unsigned char buf[280] = {0};
    snprintf(buf, 280, "/SemperCommandListener_%s", usr);
    sfree((void**)&usr);
    sld->mmap =(void*)(size_t)shm_open(buf, O_RDWR | O_CREAT, 0777);
    if(sld->mmap >= 0)
    {
        ftruncate((int)(size_t)sld->mmap, sizeof(listener_data));
        sld->ld = mmap(NULL, sizeof(listener_data), PROT_READ | PROT_WRITE, MAP_SHARED, (int)(size_t)sld->mmap, 0);
    }
    sem_init(&sld->ld->sem_wake,1,0);
    sem_init(&sld->ld->sem_write,1,1);
#endif
    sld->kill = safe_flag_init();
    sld->dummy = zmalloc(sizeof(surface_data));
    sld->dummy->cd = cd;
    list_entry_init(& sld->dummy->sources);
    list_entry_init(& sld->dummy->objects);
    list_entry_init(& sld->dummy->skhead);
    sld->meq = cd->eq;
    sld->eq = event_queue_init(semper_listener_wait_fcn, semper_listener_wake_fcn, sld);


    if(pthread_create(&sld->th, NULL, semper_listener_thread, sld))
    {
        diag_error("Failed to create listener thread");
    }

    return(sld);
}


void semper_listener_destroy(void **p)
{
    semper_listener_data *sld = *p;
     unsigned char *usr=expand_env_var("$LOGNAME");
    unsigned char buf[280] = {0};
    snprintf(buf, 280, "/SemperCommandListener_%s", usr);
    sfree((void**)&usr);
    safe_flag_set(sld->kill, 1);
    event_wake(sld->eq);
    pthread_join(sld->th, NULL);
#ifdef WIN32
    CloseHandle(sld->sem_wake);
    CloseHandle(sld->sem_write);
    UnmapViewOfFile(sld->ld);
    CloseHandle(sld->mmap);
#elif __linux__
    shm_unlink(buf);
    sem_destroy(&sld->ld->sem_wake);
    sem_destroy(&sld->ld->sem_write);
    munmap(sld->ld, sizeof(listener_data));
    close((int)(size_t)sld->mmap);

#endif
    sfree((void**)&sld->dummy);
    safe_flag_destroy(&sld->kill);
    event_queue_destroy(&sld->eq);
    sfree(p);
}

int semper_listener_writer(unsigned char *comm, size_t len)
{
#ifdef WIN32
    void *pp = OpenFileMapping(FILE_MAP_WRITE, 0, "Local\\SemperCommandListener");
    void *sem_wake = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, 0, "Local\\SemperCommandListenerWake");
    void *sem_write = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, 0, "Local\\SemperCommandListenerWrite");

    if(pp)
    {
        WaitForSingleObject(sem_write, -1);
        void *pmap = MapViewOfFile(pp, FILE_MAP_WRITE, 0, 0, sizeof(listener_data));

        if(pmap)
        {
            listener_data *p = pmap;
            memcpy(p->buf, comm, min(len,32 * 1024));
            UnmapViewOfFile(pmap);
        }

        CloseHandle(pp);
    }

    ReleaseSemaphore(sem_wake, 1, NULL);

#elif __linux__
    unsigned char *usr=expand_env_var("$LOGNAME");
    unsigned char buf[280] = {0};
    snprintf(buf, 280, "/SemperCommandListener_%s", usr);
    sfree((void**)&usr);
    int mem_fd = shm_open(buf, O_RDWR, 0777);


   if(mem_fd >= 0)
    {
        listener_data *p = mmap(NULL, sizeof(listener_data), PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0);
        if(p)
        {
        sem_wait(&p->sem_write);
        memcpy(p->buf, comm, min(len,32 * 1024));
        sem_post(&p->sem_wake);
        munmap(p, sizeof(listener_data));
        }

        close(mem_fd);
    }
#endif
    return(0);
}
