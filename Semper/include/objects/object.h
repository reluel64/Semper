#pragma once
#include <stdio.h>
#include <surface.h>
#include <skeleton.h>
#include <linked_list.h>
#include <mouse.h>

#define DEG2RAD(x) ((x)*0.0174532925)

typedef struct
{
    long left;
    long top;
    long right;
    long bottom;

} object_padding;

typedef struct
{
    unsigned char *title;
    unsigned char *text;
    double scale;
    unsigned int scaling;
    unsigned char decimals;
    unsigned char percentual;
    unsigned int color;
    size_t timeout;
} object_tooltip;

typedef struct _object object;

struct _object
{
    list_entry bindings;
    list_entry current;
    void *ttip;
    object_tooltip ot;
    unsigned char object_type; // object type(string,image,etc.)

    /*******Object Parameters*******/
    long x;
    long y;
    long h;
    long w;
    long auto_w;
    long auto_h;
    double angle;
    double back_color_angle;
    unsigned int back_color;
    unsigned int back_color_2;
    unsigned char vol_var;
    unsigned char enabled;
    unsigned char hidden;
    unsigned char* sx;
    unsigned char* sy;
    unsigned char* team;
    unsigned char* update_act; // update action
    unsigned char update_act_lock: 1;
    object_padding op;
    mouse_actions ma;
    /*****************************/
    size_t divider;
    section os; // object section
    void* sd; // parent surface
    void* pv; // private information (content depends on object type)
    /*************************/
    unsigned char die;
    /*function pointers*/
    int (*object_render_rtn)(object* o, cairo_t* cr);
    int (*object_update_rtn)(object* o);
    void (*object_destroy_rtn)(object* o);
    void (*object_init_rtn)(object* o);
    void (*object_reset_rtn)(object* o);

};

int object_update(object* o);
object* object_by_name(surface_data* sd, unsigned char* on, size_t len);
void object_destroy(object** o);
int object_init(section s, surface_data* sd);
int object_hit_testing(surface_data* sd, mouse_status* ms);
void object_render(surface_data* sd, cairo_t* cr);
void object_reset(object* o);
int object_calculate_coordinates(object* o);
