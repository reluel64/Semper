/*
 * Directory watcher
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */
#include <semper.h>
#include <linked_list.h>
#include <string_util.h>
#include <mem.h>
#include <stdint.h>
#include <stdio.h>
#include <event.h>
#ifdef WIN32
#include <windows.h>
#elif __linux__
#include <poll.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <dirent.h>
#include <sys/eventfd.h>
#endif

typedef struct
{
    void *base_fd;
    int *wtch;
    size_t wtch_c;
    event_queue *eq; //the event queue of the watcher
    event_queue *meq;  //the main event queue where the watcher will push the events
    pthread_t th;
    event_handler eh;
    void *pveh;
    void *kill;
    void *wake_event;
#ifdef __linux__
    unsigned char *dir;
#endif
} watcher_data;


#ifdef __linux__
static int watcher_fill_list(unsigned char *dir, watcher_data *wd)
{
    if(dir == NULL || wd == NULL)
    {
        return (-1);
    }

    size_t len = string_length(dir);

    DIR* dh = opendir(dir);
    struct dirent* fi = NULL;

    if(dh == NULL)
        return (-1);

    while((fi = readdir(dh)) != NULL)
    {
        if(!strcasecmp(fi->d_name, ".") || !strcasecmp(fi->d_name, ".."))
            continue;

        if(fi->d_type == DT_DIR)
        {
            size_t flen = string_length(fi->d_name);
            unsigned char *ch = zmalloc(flen + len + 2);
            snprintf(ch, flen + len + 2, "%s/%s", dir, fi->d_name);
            int fd = inotify_add_watch((int)(size_t)wd->base_fd, ch, IN_MODIFY | IN_ONESHOT);

            if(fd > 0)
            {
                int *temp = realloc(wd->wtch, (wd->wtch_c + 1) * sizeof(int));

                if(temp)
                {
                    wd->wtch = temp;
                    wd->wtch[wd->wtch_c++] = fd;
                }
                else
                {
                    inotify_rm_watch((int)(size_t)wd->base_fd, fd);
                }
            }

            sfree((void**)&ch);
        }
    }

    closedir(dh);

    return (0);
}
#endif

static void watcher_next(void *wt)
{
    watcher_data *wd = wt;
#ifdef WIN32
    FindNextChangeNotification(wd->base_fd);
#elif __linux__
    size_t len = sizeof(struct inotify_event) + NAME_MAX + 1;
    unsigned char buf[sizeof(struct inotify_event) + NAME_MAX + 1] = {0};

    while(read((int)(size_t)wd->base_fd, buf, len) > 0);

    for(size_t i = 0; i < wd->wtch_c; i++)
    {
        inotify_rm_watch((int)(size_t)wd->base_fd, wd->wtch[i]);
    }

    sfree((void**)&wd->wtch);
    wd->wtch_c = 0;
    watcher_fill_list(wd->dir, wd);

    while(read((int)(size_t)wd->base_fd, buf, len) > 0);

#endif

}


static void *watcher_thread(void *pv)
{
    watcher_data *wd = pv;

    while(safe_flag_get(wd->kill) == 0)
    {
        event_push(wd->eq, (event_handler)watcher_next, wd, 0, EVENT_NO_WAKE);
        event_wait(wd->eq);

        if(safe_flag_get(wd->kill))
            break;

        event_push(wd->meq, wd->eh, wd->pveh, 0, EVENT_PUSH_HIGH_PRIO);
        event_process(wd->eq);

    }

    pthread_exit(NULL);
    return(NULL);
}


static size_t watcher_event_wait(void *pv, size_t timeout)
{
    watcher_data *wd = pv;
#ifdef WIN32
    void *harr[2] = { wd->base_fd, wd->wake_event};
    WaitForMultipleObjects(2, harr, 0, -1);
#elif __linux__

 struct pollfd events[2];
    memset(events,0,sizeof(events));
    events[0].fd=(int)(size_t)wd->base_fd;
    events[0].events = POLLIN;
    events[1].fd=(int)(size_t)wd->wake_event;
    events[1].events = POLLIN;
    poll(events, 2, -1);
    if(events[1].revents)
    {
        eventfd_t dummy;
    eventfd_read((int)(size_t)wd->wake_event, &dummy); //consume the event
    }

    #endif
    return(0);
}

static void watcher_event_wake(void *pv)
{
    watcher_data *wd = pv;
#ifdef WIN32
    SetEvent(wd->wake_event);
#elif __linux__
  eventfd_write((int)(size_t)wd->wake_event, 0x3010);
  #endif
}


void  *watcher_init(unsigned char *dir, event_queue *meq, event_handler eh, void *pveh)
{
    void *base_fd = NULL;
#ifdef WIN32

    unsigned short *uni = utf8_to_ucs(dir);
    base_fd = FindFirstChangeNotificationW(uni, 1, 0x1 | 0x2 | 0x8 | 0x10);
    sfree((void**)&uni);

#elif __linux__
    base_fd = (void*)(size_t)inotify_init1(IN_NONBLOCK);
#endif

    if(base_fd == (void*) - 1)
    {
        return(NULL);
    }

    watcher_data *wd = zmalloc(sizeof(watcher_data));

    if(wd)
    {
        wd->kill = safe_flag_init();
        wd->eq = event_queue_init(watcher_event_wait, watcher_event_wake, wd);
        wd->base_fd = base_fd;
        wd->eh = eh;
        wd->pveh = pveh;
        wd->meq = meq;
#ifdef WIN32
        wd->wake_event = CreateEvent(NULL, 0, 0, NULL);
#elif __linux__
        wd->wake_event = (void*)(size_t)eventfd(0x2712, EFD_NONBLOCK);
#endif
        pthread_create(&wd->th, NULL, watcher_thread, wd);
    }

#ifdef __linux__
    wd->dir = clone_string(dir);
    watcher_fill_list(dir, wd);
#endif


    return(wd);
}

int watcher_destroy(void **wd)
{
    watcher_data *wdd = *wd;
    safe_flag_set(wdd->kill, 1);
    event_wake(wdd->eq);
    pthread_join(wdd->th, NULL);
    safe_flag_destroy(&wdd->kill);
#ifdef WIN32
    FindCloseChangeNotification(wdd->base_fd);
    CloseHandle(wdd->wake_event);
#elif __linux__

    for(size_t i = 0; i < wdd->wtch_c; i++)
    {
        inotify_rm_watch((int)(size_t)wdd->base_fd, wdd->wtch[i]);
    }

    sfree((void**)&wdd->wtch);
    wdd->wtch_c = 0;
    close((int)(size_t)wdd->base_fd);
    sfree((void**)&wdd->dir);
#endif
    event_queue_destroy(&wdd->eq);
    sfree((void**)&wdd->wtch);
    sfree(wd);
    return(0);
}
