/*Vector object
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */
#include <objects/object.h>
#include <cairo/cairo.h>
#include <mem.h>
#include <objects/vector/vector_clipper_glue.h>
#include <objects/vector.h>
#define TESTING 0

void vector_init(object *o)
{
    o->pv=zmalloc(sizeof(vector));
    vector *v=o->pv;
    list_entry_init(&v->paths);
}

void vector_reset(object *o)
{
    vector *v=o->pv;
    vector_parser_destroy(o);
    vector_parser_init(o);

    vector_path_common *vpc = NULL;
    o->auto_h=o->h;
    o->auto_w=o->w;
    list_enum_part(vpc,&v->paths,current)
    {
        o->auto_h=max(o->auto_h,vpc->ext.height+vpc->ext.y);
        o->auto_w=max(o->auto_w,vpc->ext.width+vpc->ext.x);
    }

}

int vector_render(object *o,cairo_t *cr)
{
    vector *v=o->pv;
    vector_path_common *vpc = NULL;
#if TESTING == 0

    list_enum_part(vpc,&v->paths,current)
    {
        cairo_set_line_width(cr,vpc->stroke_w);
        cairo_set_line_join(cr,vpc->join);
        cairo_set_line_cap(cr,vpc->cap);
        cairo_new_path(cr);
        cairo_append_path(cr,vpc->cr_path);

        if(vpc->fill_gradient)
        {
            cairo_set_source(cr,vpc->fill_gradient);
            cairo_fill_preserve(cr);
        }
        else if(vpc->fill_color&(0xff<<24))
        {
            cairo_set_color(cr,vpc->fill_color);
            cairo_fill_preserve(cr);
        }

        if(vpc->stroke_gradient)
        {
            cairo_set_source(cr,vpc->stroke_gradient);
            cairo_stroke(cr);
        }
        else if(vpc->stroke_color&(0xff<<24))
        {
            cairo_set_color(cr,vpc->stroke_color);
            cairo_stroke(cr);
        }
    }

#else
#if 0
    cairo_rectangle(cr,100,100,200,100);
    cairo_matrix_t mtx;
    cairo_matrix_init_identity(&mtx);
    cairo_matrix_translate(&mtx,-200,-150);
    cairo_matrix_translate(&mtx,200,150);
    cairo_matrix_scale(&mtx,(1.0/20.0),1.0/10.0);
    cairo_matrix_translate(&mtx,-200,-150);

    cairo_pattern_t *patt=cairo_pattern_create_radial(0.0,0.0,0.0,0.0,0.0,1.0);

    cairo_pattern_add_color_stop_rgba(patt,0.33,0.0,0.0,0.0,1.0);
    cairo_pattern_add_color_stop_rgba(patt,0.66,1.0,1.0,0.0,1.0);
    cairo_pattern_add_color_stop_rgba(patt,1.0,0.0,0.0,1.0,1.0);
    cairo_pattern_set_matrix(patt,&mtx);
    cairo_set_source(cr,patt);
    cairo_fill(cr);
    cairo_pattern_destroy(patt);
#else
    static double i=0;

    cairo_rectangle_t rect= {0};
    cairo_matrix_t m= {0};

    //cairo_set_matrix(cr,&m);
    cairo_rectangle(cr,100,100,100,100);

    cairo_path_extents(cr,&rect.x,&rect.y,&rect.width,&rect.height);
    cairo_matrix_init_identity(&m);
    cairo_matrix_translate(&m,(rect.x+rect.width)/2.0,(rect.y+rect.height)/2.0);
    m.yx=tan(DEG2RAD(20));
    m.xy=tan(DEG2RAD(0));
    cairo_matrix_translate(&m,-(rect.x+rect.width)/2.0,-(rect.y+rect.height)/2.0);
    vector_parser_apply_matrixd(cr,&m);

// cairo_identity_matrix(cr);
    cairo_pattern_t * pat = cairo_pattern_create_linear( rect.x, rect.y, rect.width,rect.y);


    cairo_pattern_add_color_stop_rgba (pat, 0.0, 1.0, 0, 0,1.0);
    cairo_pattern_add_color_stop_rgba (pat, 1.0,  0.0, 1, 0,1.0);


//   cairo_matrix_init_translate(&mtx,-center_x,-center_y);
    // cairo_matrix_translate(&mtx,350+00,250+00);
    //cairo_matrix_scale(&mtx,1.5,1.5);
    //cairo_matrix_translate(&mtx,-350-00,-250-00);
//cairo_matrix_translate(&mtx,50,50);
    // cairo_pattern_set_matrix(pat,&mtx);


    cairo_set_source(cr,pat);



    cairo_fill (cr);
    cairo_pattern_destroy (pat);
#endif
#endif
    return(0);
}

void vector_destroy(object *o)
{
    vector_parser_destroy(o);
    sfree((void**)&o->pv);
}
