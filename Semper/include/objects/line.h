#pragma once
#include <objects/object.h>
#include <linked_list.h>

typedef struct
{
    double value;
    list_entry current;

} line_value;

typedef struct
{
    double max;
    double min;
    size_t index;            /*line index*/
    unsigned int line_color; /*line color*/
    list_entry values;       /*values list*/

    list_entry current; /*this entry*/

} line_data;

typedef struct _line_obj
{
    size_t max_pts;
    size_t v_count; /*number of values in list*/

    unsigned char flip;
    unsigned char reverse;
    unsigned char vertical;
    list_entry lines;
    long tempw;
    long temph;
} line_object;

void line_destroy(object* o);
void line_init(object* o);
void line_reset(object* o);
int line_update(object* o);
int line_render(object* o, cairo_t* cr);
