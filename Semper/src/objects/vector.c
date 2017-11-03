
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
    list_enum_part(vpc,&v->paths,current)
    {
        if(vpc->vpt==vector_path_path)
        {

            vector_path *vp=(vector_path*)vpc;
            vector_subpath_common *vsc=NULL;
            list_enum_part(vsc,&vp->paths,current)
            {

                printf("vpt %d\n",vsc->vpt);
            }
        }
    }


    /*
        void *clipper=vector_init_clipper();
        void *paths=vector_init_paths();

        cairo_save(cr);

        cairo_arc(cr,170,180,150,0,DEG2RAD(360));
        cairo_set_line_cap(cr,CAIRO_LINE_CAP_ROUND);
        cairo_close_path(cr);
        vector_cairo_to_clip(cr,paths);
        vector_add_paths(clipper,paths,0,1);
        cairo_restore(cr);
        cairo_new_path(cr);


        cairo_arc(cr,270,180,150,0,DEG2RAD(360));
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

    return(0);
}

void vector_destroy(object *o)
{
    sfree((void**)&o->pv);
}
