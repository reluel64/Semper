
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
    /* list_enum_part(vpc,&v->paths,current)
     {
         cairo_new_path(cr);
         if(vpc->vpt==vector_path_path)
         {

             vector_path *vp=(vector_path*)vpc;
             vector_subpath_common *vsc=NULL;
             list_enum_part(vsc,&vp->paths,current)
             {
                 printf("vpt %d\n",vsc->vpt);
             }
         }
         else if(vpc->vpt==vector_path_rectangle)
         {
             vector_rectangle *vr=(vector_rectangle*)vpc;
             cairo_rectangle(cr,vr->x,vr->y,vr->w,vr->h);

         }
         else if(vpc->vpt==vector_path_line)
         {
             vector_line *vl=(vector_line*)vpc;
             cairo_move_to(cr,vl->sx,vl->sy);
             cairo_line_to(cr,vl->dx,vl->dy);
         }
         else if(vpc->vpt==vector_path_arc)
         {
             vector_arc *va=(vector_arc*)vpc;
             cairo_arc(cr,va->xc,va->yc,va->radius,va->sa,va->ea);
         }

         cairo_set_line_cap(cr,vpc->cap);
         cairo_set_line_join(cr,vpc->join);
         cairo_set_line_width(cr,vpc->stroke_w>1.0? vpc->stroke_w:1.0);

         if(vpc->fill_color)
         {
             cairo_set_color(cr,vpc->fill_color);
             cairo_fill_preserve(cr);
         }
         if(vpc->fill_color)
         {
             cairo_set_color(cr,vpc->stroke_color);
             cairo_stroke(cr);
         }
     }*/
    /*
    cairo_set_color(cr,0xff00ff00);
    void *p1=_2geom_rectangle(20,20,200,200);
    void *p2=_2geom_rectangle(180,180,300,300);
    _2geom_path_inter(p1,p2,cr);
    */


    cairo_rectangle(cr,20,20,20,20);
    cairo_path_t *pt=cairo_copy_path(cr);
    cairo_new_path(cr);



    cairo_matrix_t mtx;
    cairo_matrix_init_identity(&mtx);
    static double i=0;
    cairo_append_path(cr,pt);
    cairo_matrix_translate(&mtx,40,40);
    cairo_matrix_rotate(&mtx,DEG2RAD(i++));
    cairo_matrix_translate(&mtx,-40,-40);
    cairo_set_matrix(cr,&mtx);





    cairo_set_color(cr,0xff00ff00);
    cairo_stroke(cr);
    vector_arc va= {0};

    /*cairo_translate(cr,0.0,300.0);

    va.sx=250.0;
    va.sy=100.0;
    va.ex=40.0;
    va.ey=108.0;
    va.rx=41.0;
    va.ry=134.0;
    va.sweep=0;
    va.large=0;
    cairo_set_line_width(cr,5.0);
    va.angle=DEG2RAD(i+=10.0);
    cairo_move_to(cr,va.sx,va.sy);
    vector_arc_path(cr,&va);*/


#if 0
    void *clipper=vector_init_clipper();
    void *paths=vector_init_paths();

    cairo_save(cr);

    cairo_rectangle(cr,2,2,100,100);
    cairo_move_to(cr,102,102);
    cairo_curve_to(cr,200,200,300,100,400,200);


    cairo_set_line_cap(cr,CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(cr,CAIRO_LINE_JOIN_ROUND);
    cairo_close_path(cr);
    vector_cairo_to_clip(cr,paths);
    vector_add_paths(clipper,paths,0,1);
    cairo_restore(cr);
    cairo_new_path(cr);


    cairo_arc(cr,270,180,150,0,DEG2RAD(360));
    cairo_close_path(cr);
    vector_cairo_to_clip(cr,paths);
    vector_add_paths(clipper,paths,1,1);
    vector_clip_paths(clipper,paths,1);

    cairo_new_path(cr);

    vector_clip_to_cairo(cr,paths);
    cairo_set_source_rgba(cr, 1, 1, 0, 1);
    cairo_fill_preserve (cr);
    cairo_set_source_rgba(cr, 0, 0, 0, 1);
    cairo_stroke (cr);
    vector_destroy_clipper(&clipper);
    vector_destroy_paths(&paths);
#endif

    return(0);
}

void vector_destroy(object *o)
{
    sfree((void**)&o->pv);
}
