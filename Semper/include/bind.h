#pragma once

#include <stdio.h>
#include <sources/source.h>
#include <surface.h>
#include <objects/object.h>
#include <linked_list.h>
typedef struct _bind_index
{
    size_t index;
    list_entry current;
} bind_index;
typedef struct _binding
{
    source* s; // source
    unsigned char* bs; // binded string
    size_t bsl; // binded string length
    list_entry current;
    list_entry index;
} binding;

typedef struct _string_bind_param
{
    unsigned char* s_in;
    unsigned char* s_out;
    unsigned char percentual;
    unsigned char decimals;
    unsigned char self_scaling;
    double scale;
} string_bind;

typedef struct _bind_numeric
{
    double min;
    double max;
    double val;
    size_t index;
} bind_numeric;


void bind_init(object* o);
void bind_destroy(object* o);
void bind_set_raw(void* soi, unsigned char* pn, unsigned char* pv);
void bind_reset(object* o);

void bind_unbind(surface_data* sd, unsigned char* sn);
int bind_update_string(object* o, string_bind* sb);
int bind_update_numeric(object* o, bind_numeric* bn);
double bind_percentual_value(double val, double min_val, double max_val);
unsigned char *bind_source_name(object *o,size_t index);
