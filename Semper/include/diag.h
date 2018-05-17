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

int diag_log(unsigned char lvl, char *fmt, ...);


#ifndef diag_info
#define diag_info(x...)  diag_log(0x1,"[INFO] "x)
#endif

#ifndef diag_warn
#define diag_warn(x...)  diag_log(0x2,"[WARN] "x)
#endif

#ifndef diag_error
#define diag_error(x...) diag_log(0x4,"[ERROR] "x)
#endif


#ifndef diag_crit
#define diag_crit(x...)  diag_log(0x8,"[CRIT] "x)
#endif

#ifndef diag_verb
#define diag_verb(x...)  diag_log(0x10,"[VERB] "x)
#endif
