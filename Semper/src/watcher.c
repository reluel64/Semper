/*
 * Surface watcher
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */
#include <semper.h>
#include <linked_list.h>
#include <string_util.h>
#include <mem.h>
#include <stdint.h>
#include <stdio.h>
#ifdef WIN32
#include <windows.h>
#elif __linux__
#include <unistd.h>
#include <sys/inotify.h>
#include <dirent.h>
#endif

typedef struct
{
    void *base_fd;
    int *wtch;
    size_t wtch_c;
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

void *watcher_get_wait(void *wt)
{
    watcher_data *wd = wt;
    return(wd->base_fd);
}

void watcher_next(void *wt)
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

void  *watcher_init(unsigned char *dir)
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
    wd->base_fd = base_fd;
#ifdef __linux__
    wd->dir = clone_string(dir);
    watcher_fill_list(dir, wd);
#endif


    return(wd);
}

int watcher_destroy(void **wd)
{
    watcher_data *wdd = *wd;
#ifdef WIN32
    FindCloseChangeNotification(wdd->base_fd);
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
    sfree((void**)&wdd->wtch);
    sfree(wd);
    return(0);
}
