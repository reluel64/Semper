#pragma once
#include <objects/object.h>
#include <image/image_cache.h>

typedef struct
{
    unsigned char* spinner_img_path;

    double start_angle;
    double spin_angle;
    double current_angle;
    long ox;
    long oy;
    image_attributes sia;

} spinner_object;

int spinner_render(object* o, cairo_t* cr);
int spinner_update(object* o);
void spinner_reset(object* o);
void spinner_destroy(object* o);
void spinner_init(object* o);
