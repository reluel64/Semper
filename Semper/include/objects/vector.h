#pragma once
#include <objects/object.h>
void vector_init(object *o);
void vector_reset(object *o);
int vector_update(object *o);
int vector_render(object *o,cairo_t *cr);
void vector_destroy(object *o);
/*not API*/
int vector_parser_init(object *o);
void vector_parser_destroy(object *o);
typedef enum
{
    vector_path_unknown,
    vector_path_line,
    vector_path_ellipse,
    vector_path_arc,
    vector_path_rectangle,
    vector_path_curve,
    vector_path_join,
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
    /*Attributes*/
    unsigned int stroke_color;
    unsigned int fill_color;
    cairo_line_cap_t cap;
    cairo_line_join_t join;
    double stroke_w;
   cairo_pattern_t *gradient;


    size_t join_cnt;
    unsigned char must_join;
    cairo_rectangle_t ext;
    void *cr_path; /*cairo path*/
    size_t index;
    list_entry current;
} vector_path_common;

typedef enum
{
    vector_clip_intersect=1,
    vector_clip_union,
    vector_clip_diff,
    vector_clip_xor,
    vector_clip_diff_b
} vector_clip_type;


typedef struct
{
    list_entry paths;
    unsigned char  check_join;
} vector;
