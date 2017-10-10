#pragma once
#include <objects/object.h>
void vector_init(object *o);
void vector_reset(object *o);
int vector_update(object *o);
int vector_render(object *o,cairo_t *cr);
void vector_destroy(object *o);
