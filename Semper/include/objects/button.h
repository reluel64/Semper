#pragma once
#include <objects/object.h>
#include <image/image_cache.h>

typedef struct
{
    image_attributes ia;
    unsigned char *image_path;
    unsigned char im_index;
} button_object;

void button_reset(object *o);
void button_destroy(object *o);
void button_init(object *o);
int  button_update(object *o);
int  button_render(object *o,cairo_t *cr);
int  button_mouse(object *o,mouse_status *ms); //the special function
