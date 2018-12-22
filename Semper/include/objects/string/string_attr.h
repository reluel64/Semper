#pragma once
#include <objects/string.h>
#include <objects/object.h>
#include <linked_list.h>

#define STRING_ATTR_CASE     0x1000000 /*value taken from the pango source code*/

#define STRING_ATTR_GRADIENT 0x1000001
#define STRING_ATTR_OUTLINE  0x1000002
#define STRING_ATTR_COLOR    0x1000003
#define STRING_ATTR_SHADOW    0x1000004

#define STRING_CASE_NORMAL 0x1
#define STRING_CASE_LOWER  0x2
#define STRING_CASE_UPPER  0x3

#define ATTR_COLOR_SHADOW   (1<<0)
#define ATTR_COLOR_BASE     (1<<1)
#define ATTR_COLOR_OUTLINE  (1<<2)

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
    size_t gradient_len;
    double gradient_ang;
    unsigned int *gradient;
} string_attributes;




void string_attr_init(object *o);
int string_attr_update(string_object *so);
void string_attr_destroy(string_object *so);
