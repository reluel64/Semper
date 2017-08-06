#pragma once
#include <objects/object.h>
#include <objects/image.h>
#include <linked_list.h>
typedef struct
{
    double value;
    list_entry current;
} histogram_value;

typedef struct
{
    unsigned char straight;
    size_t v_count;
    size_t max_hist;
    double max_val_1;
    double min_val_1;
    double max_val_2;
    double min_val_2;
    unsigned int hist_color; // histogram color
    unsigned int hist_2_color;
    unsigned int hist_over_color;
    unsigned char reverse_origin;
    unsigned char vertical;
    unsigned char flip;

    list_entry val_1;
    list_entry val_2;

    image_attributes h1ia;
    image_attributes h2ia;
    image_attributes hcia;

} histogram_object;

void histogram_reset(object* o);
void histogram_destroy(object* o);
void histogram_init(object* o);
int histogram_update(object* o);
int histogram_render(object* o, cairo_t* cr);
