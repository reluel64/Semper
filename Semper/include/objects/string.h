#pragma once
#include <objects/object.h>

typedef struct
{
    unsigned char align;
    unsigned char ellipsize;
    unsigned char decimals;
    unsigned char italic;
    unsigned char font_outline;
    unsigned char font_shadow;
    unsigned char percentual;
    unsigned char* font_name;
    unsigned char* string;
    unsigned char* bind_string;
    unsigned int font_size;
    unsigned int font_color;
    unsigned int shadow_color;
    unsigned int scaling;
    unsigned int weight;
    unsigned int outline_color;
    double scale;
    double angle;
    /*Pango stuff*/
    void* context;
    void* layout;
    void* font_map;
    void* font_desc;
} string_object;

void string_reset(object* o);
void string_destroy(object* o);
void string_init(object* o);
int string_update(object* o);
int string_render(object* o, cairo_t* cr);
