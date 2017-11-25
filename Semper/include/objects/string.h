#pragma once
#include <objects/object.h>

#define PADDING_H 4
#define PADDING_W 4

void string_reset(object* o);
void string_destroy(object* o);
void string_init(object* o);
int string_update(object* o);
int string_render(object* o, cairo_t* cr);


typedef struct
{
    unsigned char was_outlined;
    unsigned char align;
    unsigned char ellipsize;
    unsigned char decimals;
    unsigned char percentual;
    unsigned char* string;
    unsigned char* bind_string;

    unsigned int scaling;

    /*Pango stuff*/
    double scale;
    void* context;
    void* layout;
    void* font_map;
    void* font_desc;
    void *attr_list;
    size_t bind_string_len;
    list_entry attr;
} string_object;
