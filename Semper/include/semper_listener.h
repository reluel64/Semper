#pragma once
#include <semper.h>
#if defined(__linux__)
#include <semaphore.h>
#endif
void *semper_listener_init(control_data *cd);
void semper_listener_destroy(void **p);
int semper_listener_writer(unsigned char *comm,size_t len);

typedef struct
{
        char buf[32 * 1024];
#if defined(__linux__)
        sem_t sem_wake;
        sem_t sem_write;
        size_t timestamp; //we will use this under Linux to avoid a nasty limitation
#endif
} listener_data;
