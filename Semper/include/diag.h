#pragma once
#include <pthread.h>
#include <stdio.h>

typedef struct
{
    unsigned char *log_buf;
    list_entry current;
} diag_mem_log;

typedef struct
{
    void *cd;           //pointer to the control data structure
    unsigned char *fp;
    unsigned char level;
    unsigned char ltf;  //log to file
    unsigned char init;
    FILE *fh;
    list_entry mem_log; //just a circular buffer for memory logs
    size_t max_mem_log; //maximum entries in the ring
    size_t mem_log_elem; //count of entries in the buffer
    pthread_mutex_t mutex; /*multi-threading mutex*/
    struct timespec t1;

} diag_status;

int diag_log(unsigned char lvl,char *fmt, ...);
#define diag_error(x...) diag_log(0x4,"[ERROR] "x)
#define diag_info(x...)  diag_log(0x1,"[INFO] "x)
#define diag_warn(x...)  diag_log(0x2,"[WARN] "x)
#define diag_crit(x...)  diag_log(0x8,"[CRIT] "x)
#define diag_verb(x...)  diag_log(0x10,"[VERB] "x)
