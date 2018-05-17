#pragma once
#include <cairo.h>
typedef enum  { ctIntersection, ctUnion, ctDifference, ctXor } ClipType;
typedef enum  { ptSubject, ptClip } PolyType;
void *vector_init_clipper(void);
void *vector_init_paths(void);
void vector_destroy_paths(void **p);
void vector_destroy_clipper(void **c);
void vector_add_paths(void *c, void *p, PolyType t, int closed);
void vector_clip_paths(void *c, void *p, ClipType t);
void vector_cairo_to_clip(cairo_t *cr, void *p);
void vector_clip_to_cairo(cairo_t *cr, void *p);
