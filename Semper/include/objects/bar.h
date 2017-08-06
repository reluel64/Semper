#pragma once
#include <objects/object.h>
#include <objects/image.h>

typedef struct
{
    unsigned char flip;
    unsigned char vertical;
    unsigned int bar_color;
    unsigned int bar_color_2;
    unsigned int b_over_col;
    float percents;
    float percents_2;
    long temph;
    long tempw;
    image_attributes b1ia;
    image_attributes b2ia;
    image_attributes bcia;
} bar_object;

void bar_init(object* o);
void bar_destroy(object* o);
void bar_reset(object* o);
int bar_update(object* o);
int bar_render(object* o, cairo_t* cr);
