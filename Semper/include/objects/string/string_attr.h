#pragma once
#include <objects/string.h>
#include <objects/object.h>
#include <linked_list.h>

#define STRING_ATTR_CASE     0x1000000 /*value taken from the pango source code*/

#define STRING_ATTR_GRADIENT 0x1000001
#define STRING_ATTR_OUTLINE  0x1000002
#define STRING_ATTR_COLOR    0x1000003

#define STRING_CASE_NORMAL 0x1
#define STRING_CASE_LOWER  0x2
#define STRING_CASE_UPPER  0x3

#define ATTR_COLOR_BASE     (1<<0)
#define ATTR_COLOR_SHADOW   (1<<1)
#define ATTR_COLOR_OUTLINE  (1<<2)
#define ATTR_COLOR_GRADIENT (1<<3)

typedef struct
{
    size_t index;
    unsigned char strikethrough;
    unsigned char underline;
    unsigned char style;
    unsigned char stretch;
    unsigned char font_shadow;
    unsigned char font_outline;
    unsigned char underline_style;
    unsigned char str_case;
    unsigned char* font_name;
    unsigned int strikethrough_color;
    unsigned int font_color;
    unsigned int underline_color;
    unsigned char has_spacing;

    int rise;
    int spacing;

    unsigned int shadow_color;
    unsigned int outline_color;
    double font_size;
    float shadow_x;
    float shadow_y;
    unsigned int weight;
    unsigned char *pattern;
    list_entry current;
    unsigned int *range;
    size_t range_len;
    size_t gradient_len;
    double gradient_ang;
    unsigned int *gradient;
} string_attributes;



void string_fill_attrs(object *o);
int string_apply_color_attr(string_object *so,string_attributes *sa,int shadow);
int string_apply_attr(string_object *so);
void string_destroy_attrs(string_object *so);
