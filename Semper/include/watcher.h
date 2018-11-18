#pragma once
#include <event.h>
void  *watcher_init(unsigned char *dir, event_queue *meq, event_handler eh, void *pveh);
int watcher_destroy(void **wd);
void watcher_wake(void *pv);
