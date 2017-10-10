
#include <objects/object.h>
#include <cairo/cairo.h>
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
    vector *v=o->pv;
}

int vector_update(object *o)
{
    vector *v=o->pv;
    return(0);
}

int vector_render(object *o,cairo_t *cr)
{
    vector *v=o->pv;
  cairo_arc(cr,100,100,50,0,6.28);
    cairo_path_t *p=cairo_copy_path(cr);


    for(size_t i=0;i<p->num_data;i+=p->data[i].header.length)
    {
        cairo_path_data_t *data=&p->data[i];
        printf("%d %lf %lf\n",data[0].header.type,data[1].point.x,data[1].point.y);
        printf("%d %lf %lf\n",data[0].header.type,data[2].point.x,data[2].point.y);
        printf("%d %lf %lf\n",data[0].header.type,data[3].point.x,data[3].point.y);
    }
    cairo_set_color(cr,0xff00ff00);
    cairo_fill(cr);
    return(0);
}

void vector_destroy(object *o)
{
    vector *v=o->pv;
    sfree((void**)&o->pv);
}
