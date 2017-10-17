
#include <objects/object.h>
#include <cairo/cairo.h>

#include <mem.h>
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






void _union(void *cr)
{
    cairo_set_line_width(cr,20.0);
    cairo_set_color(cr,0xaa000000);
    cairo_rectangle(cr,100,100,200,200);
    cairo_arc(cr,100,100,40,DEG2RAD(0),DEG2RAD(360));
    cairo_stroke_preserve(cr);
    cairo_set_operator(cr,CAIRO_OPERATOR_CLEAR);
    cairo_fill_preserve(cr);
    cairo_set_operator(cr,CAIRO_OPERATOR_OVER);

    cairo_set_color(cr,0xaaff0000);
    cairo_fill(cr);
}
#include "../gfxpoly/poly.h"
#include "../gfxpoly/gfxpoly.h"
int vector_render(object *o,cairo_t *cr)
{

    gfxcanvas_t*canvas = gfxcanvas_new(0.05);
    canvas->cr=cr;
    canvas->lineTo(canvas, 100,0);
    canvas->lineTo(canvas, 100,100);
    canvas->splineTo(canvas, 50,50, 0,100);
    canvas->lineTo(canvas, 0,0);
    canvas->close(canvas);

    gfxpoly_t* poly1 = (gfxpoly_t*)canvas->result(canvas);


    canvas = gfxcanvas_new(0.05);
    canvas->lineTo(canvas, 50,0);
    canvas->lineTo(canvas, 150,0);
    canvas->lineTo(canvas, 150,150);
    canvas->lineTo(canvas, 50,150);
    canvas->lineTo(canvas, 50,0);
    canvas->close(canvas);

    gfxpoly_t* poly2 = (gfxpoly_t*)canvas->result(canvas);

    gfxpoly_t *r=gfxpoly_union(poly1,poly2);

    gfxline_t *x=gfxline_from_gfxpoly_with_direction(r);





    //gfxline_print(x);
    gfxline_t*l = gfxline_rewind(x);



    while(l)
    {
        if(l->type == gfx_moveTo)
        {
            cairo_move_to(cr, l->x, l->y);
        }
        if(l->type == gfx_lineTo)
        {
            cairo_line_to(cr, l->x, l->y);
        }
        if(l->type == gfx_splineTo)
        {
            printf("splineTo %.2f,%.2f %.2f,%.2f\n", l->sx, l->sy, l->x, l->y);
        }
        l = l->next;
    }
    cairo_set_color(cr,0xaaff0000);
    cairo_stroke(cr);

     cairo_set_color(cr,0xaaff0000);
    cairo_stroke(cr);
    //
    // cairo_fill(cr);

    //cairo_set_color(cr,0xff00ff00);
    //  cairo_paint(cr);

//   cairo_rectangle_int_t r1=
//   {
//       .x=60,
//       .y=60,
//       .width=140,
//        .height=140
//   };
    //  cairo_rectangle_int_t r2=
    //  {
    //      .x=100,
    //      .y=100,
    //      .width=200,
    //     .height=200
//   };
    // cairo_region_t *reg=cairo_region_create_rectangle(&r1);

    // cairo_region_intersect_rectangle(reg,&r2);
    // cairo_region_get_extents(reg,&r1);

    /* for (size_t i=0; i < p->num_data; i += p->data[i].header.length)
     {
         data = &p->data[i];
         switch (data->header.type)
         {
         case CAIRO_PATH_MOVE_TO:
    //if(data[1].point.x==100)
    //            data[1].point.x=140;
             printf("CAIRO_PATH_MOVE_TO %lf %lf\n",data[1].point.x,data[1].point.y);
             break;
         case CAIRO_PATH_LINE_TO:

             printf("CAIRO_PATH_LINE_TO %lf %lf\n",data[1].point.x,data[1].point.y);
             break;
         case CAIRO_PATH_CURVE_TO:
             printf("CAIRO_PATH_CURVE_TO %lf %lf\n",data[1].point.x,data[1].point.y);
             printf("CAIRO_PATH_CURVE_TO %lf %lf\n",data[2].point.x,data[2].point.y);
             printf("CAIRO_PATH_CURVE_TO %lf %lf\n",data[3].point.x,data[3].point.y);
             break;
         case CAIRO_PATH_CLOSE_PATH:
             break;
         }
         printf("--------\n");
     }*/
    //  cairo_new_path(cr);
    // cairo_append_path(cr,p);


    // cairo_path_destroy(p);

    return(0);
}

void vector_destroy(object *o)
{
    sfree((void**)&o->pv);
}
