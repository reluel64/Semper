#pragma once

#include <time.h>
#include <semper.h>
#include <stdio.h>
#include <skeleton.h>
#include <image/image_cache.h>
#include <crosswin/crosswin.h>
#include <linked_list.h>
#include <mouse.h>

#define cairo_set_color(cr,color) \
    { \
        double alpha=((double)(((color)&0xff000000)>>24)) /255.0; \
        double red=  ((double)(((color)&0x00ff0000)>>16)) /255.0; \
        double green=((double)(((color)&0x0000ff00)>>8))  /255.0; \
        double blue= ((double)((color)&0x000000ff))       /255.0; \
        cairo_set_source_rgba((cr),red,green,blue,alpha); \
    }

typedef struct _surface_paths
{
    unsigned char* data_dir;                        // absolute data directory
    unsigned char* file_path;                       // absolute file path
    unsigned char* surface_dir;                     // absolute surface directory
    unsigned char* surface_file;                    // file name
    unsigned char* surface_rel_dir;                 // surface relative name (acts as the surface name)
    // unsigned char* inc;                             // surface header directory
    size_t variant;                                 // this is used to obtain file name
} surface_paths;

typedef struct _surface_data
{
    surface_paths sp;
    control_data* cd;
    list_entry current;
    list_entry objects;
    list_entry sources;
    list_entry skhead; // skeleton head
    image_attributes ia;

    //Surface Parameters
    long order;
    long x;
    long y;
    long h;
    long w;
    unsigned int srf_col;      //main color of the surface - if not specified by the user, this will be 0
    unsigned int srf_col_2;    //second surface color - setting this will result in a gradient background
    unsigned char hidden:1;
    unsigned char draggable:1;
    unsigned char wsz:2;        // volatile window size bit 0 - AutoSize; bit 1 - Need resize;
    unsigned char ro;          // required opacity
    unsigned char co;          // current opacity
    unsigned char clkt:1;        // click through
    unsigned char snp:1;         // store new position
    unsigned char rim:1;         // reload if modified
    unsigned char keep_on_screen:1;
    unsigned char* team;
    unsigned char* focus_act;
    unsigned char* unfocus_act;
    unsigned char* update_act;
    unsigned char* reload_act;
    unsigned char* unload_act;
    unsigned char mouse_hover;
    unsigned char old_mouse_hover;
    unsigned char update_act_lock:1;
    unsigned char reload_act_lock:1;
    unsigned char unload_act_lock:1;
    unsigned char zorder;
    unsigned char visible:1;
    unsigned char lock_w;
    unsigned char lock_h;
    //*************************************************
    size_t cycle;             // update cycle counter (it is used for dividers)
    section spm;              //[surface]
    section sv;               //[surface-Variables]
    section scd;              // section from main_skeleton
    size_t def_divider;       //default update divider
    size_t uf;                // update frequency (miliseconds)
    mouse_actions ma;         // mouse actions
    void* sw;                 // surface window
    char fade_direction;      //does this need explanation? Ok - it is used during fade operations to tell how we should fade the window (close fade, open fade)
    semper_timestamp st;      //used by ReloadIfModified option
#ifdef __linux__
    int inotify_watch_id;
#endif
} surface_data;

surface_data* surface_by_name(control_data* cd, unsigned char* sn);
void surface_reset(surface_data* sd);
int surface_destroy(surface_data* sd);
size_t surface_load(control_data* cd, unsigned char* sdir, size_t variant);
void surface_reload(surface_data* sd);
int surface_signal_handler(surface_data* sd);
int command(surface_data* sd, unsigned char **pa);
int surface_update(surface_data* sd);
void surface_queue_update(surface_data* sd);
int surface_modified(surface_data *sd);
size_t surface_file_variant(unsigned char* sd, unsigned char* file);
int surface_change_variant(surface_data* sd, unsigned char* vf);
void surface_fade(surface_data* sd);
surface_data *surface_load_memory(control_data *cd,unsigned char *buf,size_t buf_sz,surface_data **sd);
int surface_adjust_size(surface_data *sd);
