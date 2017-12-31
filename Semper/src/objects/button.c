/*Button Object
 *Part of Project 'Semper'
 *Written by Alexandru-Daniel Mărgărit
 */

#include <objects/button.h>

#include <semper.h>
#include <mem.h>
#include <xpander.h>
#include <bind.h>
#include <parameter.h>
#include <mouse.h>
#include <event.h>

/*This routine handles the mouse events for the button as it has
 *to be the most responvive object when it comes to transitions
 */
int button_mouse(object *o,mouse_status *ms)
{
    surface_data *sd=o->sd;
    button_object *bto=o->pv;


    if(ms->state<=0&&ms->hover)
        bto->im_index=1;

    else if(ms->hover&&ms->button)
        bto->im_index=2;

    else
        bto->im_index=0;

    surface_adjust_size(sd);
    event_push(sd->cd->eq,(event_handler)crosswin_draw,sd->sw,0,0);
    return(0);
}

void button_init(object *o)
{
    if(o)
    {
        o->pv=zmalloc(sizeof(button_object));
    }
}

void button_reset(object *o)
{
    button_object *bto=o->pv;
    sfree((void**)&bto->image_path);
    bto->image_path=parameter_string(o,"ButtonImage","%0",XPANDER_OBJECT);
    image_cache_image_parameters(o,&bto->ia,XPANDER_OBJECT,"Button");
    bto->ia.keep_ratio=1;
}

int button_update(object *o)
{
    surface_data* sd = o->sd;
    button_object *bto=o->pv;
    string_bind sb = { 0 };

    image_cache_unref_image(sd->cd->ich, &bto->ia,0);

    sfree((void**)&bto->ia.path);
    sb.s_in = bto->image_path;

    bind_update_string(o, &sb);

    if(sb.s_out == NULL)
    {
        return (-1);
    }

    image_cache_query_image(sd->cd->ich, &bto->ia, NULL, o->w*3, o->h*3);

    //sfree((void**)&sb.s_out);

    if(bto->ia.width>bto->ia.height)
    {
        o->auto_w = bto->ia.width/3;
        o->auto_h = bto->ia.height;
    }
    else
    {
        o->auto_h = bto->ia.height/3;
        o->auto_w = bto->ia.width;
    }

    return (1);
}

int button_render(object *o,cairo_t *cr)
{
    button_object* bto = o->pv;
    surface_data *sd=o->sd;
    unsigned char *px=NULL;
    long w = bto->ia.width;
    long h = bto->ia.height;
    image_cache_query_image(sd->cd->ich, &bto->ia, &px, o->w*3, o->h*3);

    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, bto->ia.width);

    cairo_surface_t* image = cairo_image_surface_create_for_data(px, CAIRO_FORMAT_ARGB32, bto->ia.width, bto->ia.height, stride);

    if(bto->ia.width>bto->ia.height)
    {
        cairo_rectangle(cr,0,0,w/3.0,h);
        cairo_clip(cr);
        cairo_new_path(cr);
        cairo_translate(cr,(double)(bto->im_index*(-w/3)),0);
    }
    else
    {
        cairo_rectangle(cr,0,0,w,h/3.0);
        cairo_clip(cr);
        cairo_new_path(cr);
        cairo_translate(cr,0.0,(double)(bto->im_index*(h/3)));
    }

    cairo_set_source_surface(cr, image, 0, 0);

    if(bto->ia.width>bto->ia.height)
    {
        cairo_translate(cr,(double)(bto->im_index*(w/3)),0.0);
    }
    else
    {
        cairo_translate(cr,0.0,(double)(bto->im_index*(-h/3)));
    }

    cairo_paint(cr);
    cairo_surface_destroy(image);
    image_cache_unref_image(sd->cd->ich, &bto->ia,0);
    return (0);
}


void button_destroy(object *o)
{
    if(o)
    {
        button_object *bto=o->pv;
        sfree((void**)&bto->image_path);
        sfree((void**)&bto->ia.path);
        sfree((void**)&o->pv);
    }
}
