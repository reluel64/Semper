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
    vector_path_path,
    vector_subpath_none,
    vector_subpath_arc_to,
    vector_subpath_line_to,
    vector_subpath_curve_to
} vector_path_type;

typedef enum
{
    vector_param_none=vector_subpath_curve_to+1,
    vector_param_done,
    vector_param_stroke_width,
    vector_param_stroke,
    vector_param_fill,
    vector_param_join,
    vector_param_cap,
    vector_param_matrix
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
} vector_rectangle;

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
    double xc;
    double yc;
    double radius;
    double sa;
    double ea;
    unsigned char inv;
} vector_arc;

typedef struct
{
    vector_path_common vpc;
    double x;
    double y;
    list_entry paths; //ArcTo, CurveTo, etc.
} vector_path;


/*****************SUB PATHS******************/

typedef struct
{
    vector_path_type vpt;
    list_entry current;
} vector_subpath_common;

typedef struct
{
    vector_subpath_common vsc;
    double dx;
    double dy;
} vector_path_line_to;

typedef struct
{
    vector_subpath_common vsc;
    double radius;
    double start_angle;
    double end_angle;
} vector_path_arc_to;

typedef struct
{
    vector_subpath_common vsc;
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
    size_t index;
    list_entry current;
} vector_paths;

typedef struct
{
    unsigned char joined;
    list_entry paths;
} vector;
