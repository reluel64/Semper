#pragma once
void  *watcher_init(unsigned char *dir);
void *watcher_get_wait(void *wt);
void watcher_next(void *wt);
