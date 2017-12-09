
#include <objects/object.h>
#include <cairo/cairo.h>
#include <mem.h>
#include <objects/vector/vector_clipper_glue.h>

#include <objects/vector.h>

void vector_init(object *o)
{
    o->pv=zmalloc(sizeof(vector));
    vector *v=o->pv;
    list_entry_init(&v->paths);
}

void vector_reset(object *o)
{
    vector_parser_destroy(o);
    vector_parser_init(o);
}

int vector_update(object *o)
{
    unused_parameter(o);
    return(0);
}


int vector_render(object *o,cairo_t *cr)
{
    vector *v=o->pv;
    vector_path_common *vpc = NULL;
#if 0
    list_enum_part(vpc,&v->paths,current)
    {
        cairo_set_line_width(cr,vpc->stroke_w);
        cairo_set_line_join(cr,vpc->join);
        cairo_set_line_cap(cr,vpc->cap);
        cairo_new_path(cr);
        cairo_append_path(cr,vpc->cr_path);

        if(vpc->fill_color&(0xff<<24))
        {
            cairo_set_color(cr,vpc->fill_color);
            cairo_fill_preserve(cr);
        }
        if(vpc->stroke_color&(0xff<<24))
        {
            cairo_set_color(cr,vpc->stroke_color);
            cairo_stroke(cr);
        }
    }
    #endif

    cairo_arc(cr,100,100,100,0,6.24);
    cairo_pattern_t *patt=cairo_pattern_create_radial(100.0,100.0,0.0,100.0,100.0,100.0);
    cairo_pattern_add_color_stop_rgba(patt,0.33,0.0,0.0,0.0,1.0);
    cairo_pattern_add_color_stop_rgba(patt,0.66,1.0,1.0,0.0,1.0);
    cairo_pattern_add_color_stop_rgba(patt,1.0,0.0,0.0,1.0,1.0);
    cairo_set_source(cr,patt);
    cairo_fill(cr);
    cairo_pattern_destroy(patt);
    return(0);
}

void vector_destroy(object *o)
{
    vector_parser_destroy(o);
    sfree((void**)&o->pv);
}
