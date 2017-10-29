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

} vector_path_type;

typedef enum
{
    vector_path_set_none=vector_path_path+1,
    vector_path_set_arc_to,
    vector_path_set_line_to,
    vector_path_set_curve_to
}vector_path_set;

typedef enum
{
    vector_param_none=vector_path_set_curve_to+1,
    vector_param_stroke_width,
    vector_param_stroke_color,
    vector_param_fill
} vector_param_type;

typedef enum
{
    vcm_none,
    vcm_fill,
    vcm_stroke
} vector_path_colorization_mode;

typedef struct
{
    double stroke_w;
    cairo_line_cap_t cap;
    cairo_line_join_t join;
    list_entry colors;
    cairo_matrix_t mtx;
    vector_path_colorization_mode vcm;
} vector_path_common;

typedef struct
{
    double x;
    double y;
    double w;
    double h;
    vector_path_common vpc;
} vector_rectangle;

typedef struct
{
    double sx;
    double sy;
    double dx;
    double dy;
    vector_path_common vpc;
} vector_line;

typedef struct
{
    double dx;
    double dy;
} vector_path_line_to;

typedef struct
{
    double radius;
    double start_angle;
    double end_angle;
} vector_path_arc_to;

typedef struct
{
    double dx1;
    double dy1;
    double dx2;
    double dy2;
    double dx3;
    double dy3;
} vector_path_curve_to;

typedef struct
{
    double x;
    double y;
    list_entry paths;
    vector_path_common vpc;
} vector_path;

typedef struct
{
    size_t index;
    vector_path_type path_type;

    list_entry current;
    void *path_data;
} vector_paths;

typedef struct
{
    unsigned char joined;
    list_entry paths_sets;
} vector;
