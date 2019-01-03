#pragma once
#if defined(WIN32)
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#endif

#include <stdio.h>
#include <math.h>
#include <stddef.h>
#include <skeleton.h>
#include <linked_list.h>
#include <event.h>
#include <crosswin/crosswin.h>
#include <diag.h>
#ifndef unused_parameter
#define unused_parameter(p) ((void)(p))
#endif

#ifndef min
#undef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#else
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#undef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#else
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#undef CLAMP
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))


typedef struct
{

    void* pv;
    unsigned char* kv; // this will store the key value
    key lk;
    section ls;
    size_t depth;
} skeleton_handler_data;

typedef struct
{
    crosswin c;
    unsigned char *app_dir;
    unsigned char* root_dir;            // root directory path
    unsigned char* cf;                  // configuration file
    unsigned char* surface_dir;         // surfaces directory
    unsigned char* ext_dir;             // extensions directory
    unsigned char *ext_app_dir;
    unsigned char* font_cache_dir;
    list_entry shead;                   // config skeleton
    list_entry surfaces;                // surfaces head
    section smp;                        //[Semper]
    void *srf_reg;                      // surface registry
    size_t surface_dir_length;
    size_t ext_dir_length;
    size_t ext_app_dir_len;
    size_t root_dir_length;
    size_t app_dir_len;
    event_queue* eq;                    //main event queue
    event_queue* leq;                   //listener event queue
    void *ich;                          //image cache holder
    unsigned char shutdown;
    void *fmap;
    void *listener;
    void *quit_flag;
    /*Surface Watcher*/

    void *watcher;
    /*Diagnostic provider*/
    void  *diag_prov;

} control_data;

typedef struct
{
    size_t tm1;
    size_t tm2;
} semper_timestamp;

int semper_save_configuration(control_data* cd);
int semper_get_file_timestamp(unsigned char *file, semper_timestamp *st);
int diag_init(control_data *cd);
int diag_log(unsigned char lvl, char *fmt, ...);
int diag_print(void);
/**************************************************************************/
size_t tss();
