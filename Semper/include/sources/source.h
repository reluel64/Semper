#pragma once
#define PREPARE_FORMULA_CMODE 0x1
#define PREPARE_FORMULA_SMODE 0x0
#include <stddef.h>
#include <surface.h>
#include <skeleton.h>
#include <sources/average.h>
#include <linked_list.h>
#define SOURCE_VARIABLE_EXPAND 0x1
#define SOURCE_VARIABLE_DOUBLE 0x2

typedef struct _source
{
    list_entry current;
    void* sa; // source action
    section cs;
    /**************Source Routines**************/
    void* library;
    void (*source_init_rtn)(void** spv, void* ip);
    void (*source_destroy_rtn)(void** spv);
    double (*source_update_rtn)(void* spv);
    void (*source_reset_rtn)(void* spv, void* ip);
    unsigned char* (*source_string_rtn)(void* spv);
    void (*source_command_rtn)(void* spv, unsigned char* comm);
    /*****************Source Parameters***************/
    size_t divider;
    size_t cycle_div; // cycle dividers
    double max_val;
    double min_val;
    unsigned char* extension;
    unsigned char vol_var;
    unsigned char inverted;
    unsigned char disabled;
    unsigned char type;
    unsigned char* replacements;
    unsigned char paused;
    unsigned char hold_min;
    unsigned char hold_max;
    unsigned char regexp;
    unsigned char always_do;
    unsigned char* team;
    size_t avg_count;
    average* avg_struct;
    unsigned char* update_act; // action to be perfomed when the source is updated
    unsigned char* change_act; // action to be performed when the value is changed (can be the numerical or the string)
    unsigned char change_act_lock;
    unsigned char update_act_lock;

    /************************************************/
    void* sd; // parent surface
    void* pv; // source private data
    unsigned char die; // if set to 1 then the source is dead
    /*****************Source information********************/
    unsigned char* s_info;
    unsigned char* inf_exp; // source information with replacements
    unsigned char inf_double[64]; // source information that must have only the double value not the string one
    size_t s_info_len;
    size_t update_cycle;
    size_t inf_double_len;
    size_t inf_exp_len;
    unsigned char* ext_str; // extension string - here the extension API will store a string if requested
    double d_info;
    /*******************************************************/
} source;

int source_init(section cs, surface_data* sd);
source* source_by_name(surface_data* sd, unsigned char* sn,size_t len);
void source_destroy(source** s);
int source_update(source* s);
int update_sources(surface_data* sd);
void update_source_table(source* s);
unsigned char* source_variable(source* s, size_t* len, unsigned char flags);
unsigned char* formula(void* sd, unsigned char* in);
void source_reset(source* s);
