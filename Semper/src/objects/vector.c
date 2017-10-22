
#include <objects/object.h>
#include <cairo/cairo.h>
#include <mem.h>
#include <objects/vector/vector_clipper_glue.h>


typedef struct
{
    int i;
} vector;


void vector_init(object *o)
{
    o->pv=zmalloc(sizeof(vector));
}

void vector_reset(object *o)
{
}

int vector_update(object *o)
{
    return(0);
}

int vector_render(object *o,cairo_t *cr)
{

    void *clipper=vector_init_clipper();
    void *paths=vector_init_paths();


   // cairo_arc(cr,170,180,150,0,DEG2RAD(360));

/*cairo_rectangle(cr,50,50,200,200);
    cairo_close_path(cr);
    vector_cairo_to_clip(cr,paths);
    vector_add_paths(clipper,paths,0,1);
    cairo_new_path(cr);


    //cairo_arc(cr,270,180,150,0,DEG2RAD(360));
    cairo_rectangle(cr,180,180,200,200);
    cairo_close_path(cr);
    vector_cairo_to_clip(cr,paths);
    vector_add_paths(clipper,paths,1,1);
    vector_clip_paths(clipper,paths,3);

    cairo_new_path(cr);

    vector_clip_to_cairo(cr,paths);
    cairo_set_source_rgba(cr, 1, 1, 0, 1);
    cairo_fill_preserve (cr);
    cairo_set_source_rgba(cr, 0, 0, 0, 1);
    cairo_stroke (cr);
    vector_destroy_clipper(&clipper);
    vector_destroy_paths(&paths);
*/
double x=25.6,  y=128.0;
double x1=102.4, y1=230.4,
       x2=153.6, y2=25.6,
       x3=230.4, y3=128.0;

cairo_move_to (cr, x, y);
cairo_curve_to (cr, x1, y1, x2, y2, x3, y3);
cairo_set_color(cr,0xff00ff00);
cairo_fill_preserve(cr);
cairo_set_color(cr,0xff000000);
cairo_stroke(cr);

    return(0);
}

void vector_destroy(object *o)
{
    sfree((void**)&o->pv);
}
