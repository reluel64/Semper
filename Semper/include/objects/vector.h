#pragma once
#include <objects/object.h>
void vector_init(object *o);
void vector_reset(object *o);
int vector_update(object *o);
int vector_render(object *o,cairo_t *cr);
void vector_destroy(object *o);
/*not API*/
int vector_parser_init(object *o);

typedef enum
{
    vector_path_unknown,
    vector_path_line,
    vector_path_ellipse,
    vector_path_arc,
    vector_path_rectangle,
    vector_path_curve,
    vector_path_set,
    vector_path_set_arc_to,
    vector_path_set_line_to,
    vector_path_set_curve_to
} vector_path_type;

typedef enum
{
    vector_param_none=vector_path_set_curve_to+1,
    vector_param_done,
    vector_param_shared,
    vector_param_stroke_width,
    vector_param_stroke,
    vector_param_fill,
    vector_param_join,
    vector_param_cap,
    vector_param_rotate,
    vector_param_scale,
    vector_param_skew,
    vector_param_offset
} vector_param_type;


typedef struct
{
    vector_path_type vpt;
    size_t index;
    double stroke_w;
    unsigned int stroke_color;
    unsigned int fill_color;
    cairo_line_cap_t cap;
    cairo_line_join_t join;
    list_entry colors;
    cairo_matrix_t mtx;
    list_entry current;
} vector_path_common;

typedef struct
{
    vector_path_common vpc;
    double x;
    double y;
    double w;
    double h;
    double rx;
    double ry;
} vector_rectangle;

typedef struct
{
    vector_path_common vpc;
    double xc;
    double yc;
    double rx;
    double ry;
} vector_ellipse;


typedef struct
{
    vector_path_common vpc;
    double sx;
    double sy;
    double dx;
    double dy;
} vector_line;

typedef struct
{
    vector_path_common vpc;
    double sx;
    double sy;
    double ex;
    double ey;
    unsigned char sweep;
    unsigned char large;
    double angle;
    double rx;
    double ry;
    unsigned char closed;
} vector_arc;

typedef struct
{
    vector_path_common vpc;
    double sx;
    double sy;
    double ex;
    double ey;
    double cx1;
    double cy1;
    double cx2;
    double cy2;
    unsigned char closed;
} vector_curve;


typedef struct
{
    vector_path_common vpc;
    double x;
    double y;
    list_entry path_sets; //ArcTo, CurveTo, etc.
    unsigned char closed;
} vector_path;


/*****************SUB PATHS******************/

typedef struct
{
    vector_path_type vpt;
    list_entry current;
} vector_path_set_common;

typedef struct
{
    vector_path_set_common vsc;
    double dx;
    double dy;
} vector_path_line_to;

typedef struct
{
    vector_path_set_common vsc;
     double ex;
    double ey;
    unsigned char sweep;
    unsigned char large;
    double angle;
    double rx;
    double ry;
} vector_path_arc_to;

typedef struct
{
    vector_path_set_common vsc;
    double dx1;
    double dy1;
    double dx2;
    double dy2;
    double dx3;
    double dy3;
} vector_path_curve_to;


typedef struct
{
    vector_path_type vpt;
    list_entry current;
} vector_paths;

typedef struct
{
    unsigned char joined;
    list_entry paths;
} vector;
