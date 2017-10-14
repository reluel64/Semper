
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

int vector_render(object *o,cairo_t *cr)
{
  /*  cairo_arc(cr,100,100,40,0,DEG2RAD(360));
    cairo_rectangle(cr,100,100,200,200);
    cairo_set_color(cr,0xffff0000);
    cairo_path_t *p=cairo_copy_path(cr);

    cairo_path_data_t *data;
    for (size_t i=0; i < p->num_data; i += p->data[i].header.length)
    {
        data = &p->data[i];
        switch (data->header.type)
        {
        case CAIRO_PATH_MOVE_TO:
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
    }
    cairo_stroke(cr);*/
    /*
        for(size_t i=0; i<p->num_data; i+=p->data[i].header.length)
        {
            cairo_path_data_t *data=&p->data[i];
            printf("%d %lf %lf\n",data[0].header.type,data[1].point.x,data[1].point.y);
            printf("%d %lf %lf\n",data[0].header.type,data[2].point.x,data[2].point.y);
            printf("%d %lf %lf\n",data[0].header.type,data[3].point.x,data[3].point.y);
        }*/

    return(0);
}

void vector_destroy(object *o)
{
    sfree((void**)&o->pv);
}
