#pragma once
#include <objects/object.h>
#include <image/image_cache.h>

typedef struct _image_object
{
    image_attributes ia;
    unsigned char *image_path;
} image_object;

int image_render(object* o, cairo_t* cr);
void image_destroy(object* o);
void image_init(object* o);
void image_reset(object* o);
int image_update(object* o);
