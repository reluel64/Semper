/*
Bar Object
Part of Project 'Semper'
Written by Alexandru-Daniel Mărgărit
*/
#include <surface.h>
#ifdef WIN32
#include <windows.h>
#include <commctrl.h>
#endif

#include <bind.h>
#include <objects/bar.h>
#include <mem.h>
#include <string_util.h>
#include <parameter.h>
#include <objects/object.h>


void bar_init(object* o)
{
    if(o && o->pv == NULL)
    {
        o->pv = zmalloc(sizeof(bar_object));
    }
}

void bar_destroy(object* o)
{
    bar_object* bo = o->pv;
    surface_data *sd=o->sd;

    if(bo)
    {
        image_cache_unref_image(sd->cd->ich, &bo->b1ia,1);
        image_cache_unref_image(sd->cd->ich, &bo->b2ia,1);
        image_cache_unref_image(sd->cd->ich, &bo->bcia,1);
        sfree((void**)&bo->b1ia.path);
        sfree((void**)&bo->b2ia.path);
        sfree((void**)&bo->bcia.path);
        sfree((void**)&o->pv);
    }
}

void bar_reset(object* o)
{
    bar_object* bo = o->pv;
    surface_data *sd=o->sd;
    image_cache_unref_image(sd->cd->ich, &bo->b1ia,0);
    image_cache_unref_image(sd->cd->ich, &bo->b2ia,0);
    image_cache_unref_image(sd->cd->ich, &bo->bcia,0);
    sfree((void**)&bo->b1ia.path);
    sfree((void**)&bo->b2ia.path);
    sfree((void**)&bo->bcia.path);

    bo->bar_color = parameter_color(o, "BarColor", 0xff00ff00, XPANDER_OBJECT);
    bo->bar_color_2 = parameter_color(o, "Bar2Color", 0xffff0000, XPANDER_OBJECT);
    bo->b_over_col = parameter_color(o, "BarCommonColor", 0xffffff00, XPANDER_OBJECT);
    bo->vertical = parameter_bool(o, "Vertical", 0, XPANDER_OBJECT);
    bo->flip = parameter_bool(o, "Flip", 0, XPANDER_OBJECT);

    bo->bcia.path = parameter_string(o, "CommonImagePath", NULL, XPANDER_OBJECT);
    bo->b1ia.path = parameter_string(o, "BarImagePath", NULL, XPANDER_OBJECT);
    bo->b2ia.path = parameter_string(o, "Bar2ImagePath", NULL, XPANDER_OBJECT);

    if(bo->b1ia.path)
    {
        image_cache_image_parameters(o, &bo->b1ia, XPANDER_OBJECT, "Bar");
    }

    if(bo->b2ia.path)
    {
        image_cache_image_parameters(o, &bo->b2ia, XPANDER_OBJECT, "Bar2");
    }

    if(bo->bcia.path)
    {
        image_cache_image_parameters(o, &bo->bcia, XPANDER_OBJECT, "Common");
    }
}

int bar_update(object* o)
{
    o->auto_w=o->w;
    o->auto_h=o->h;
    ///  surface_data* sd = o->sd;
    bar_object* bo = o->pv;
    bind_numeric bn = { 0 };

    bind_update_numeric(o, &bn);
    bo->percents = bind_percentual_value(bn.val, bn.min, bn.max);

    if(bo->bar_color_2)
    {
        bind_numeric bn_2 = { 0 };
        bn_2.index = 1;
        bind_update_numeric(o, &bn_2);
        bo->percents_2 = bind_percentual_value(bn_2.val, bn_2.min, bn_2.max);
    }

    /*
        image_cache_unref_image(sd->cd->ich, &bo->b1ia,0);
        image_cache_unref_image(sd->cd->ich, &bo->b2ia,0);
        image_cache_unref_image(sd->cd->ich, &bo->bcia,0);

        image_cache_query_image(sd->cd->ich, &bo->b1ia, NULL, o->w, o->h);
        image_cache_query_image(sd->cd->ich, &bo->b2ia, NULL, o->w, o->h);
        image_cache_query_image(sd->cd->ich, &bo->bcia, NULL, o->w, o->h);
    */
    return (1);
}

int bar_render(object* o, cairo_t* cr)
{
    bar_object* bo = o->pv;
    cairo_surface_t* image_surface = NULL;
    surface_data *sd=o->sd;
    unsigned char *px=NULL;
    // long iw=0;
    // long ih=0;
    long cx = 0;
    long cy = 0;
    long cw = 0;
    long ch = 0;

    image_cache_query_image(sd->cd->ich,&bo->bcia,&px,o->w,o->h);

    if(px == NULL)
    {
        cairo_set_color(cr,bo->b_over_col);
    }
    else
    {
        int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, o->auto_w);
        image_surface = cairo_image_surface_create_for_data(px, CAIRO_FORMAT_ARGB32, o->w, o->h, stride);
        cairo_set_source_surface(cr, image_surface, 0.0, 0.0);
    }

    double percentage = min(bo->percents, bo->percents_2);

    if(bo->vertical && !bo->flip)
    {
        cx = 0;
        cy = o->h - o->h * percentage;
        cw = o->w;
        ch = ceil((o->h * percentage));
    }
    else if(bo->vertical && bo->flip)
    {
        ch = o->h * percentage;
        cw = o->w;
    }
    else if(bo->flip)
    {
        cx = o->w - o->w * percentage;
        cw = o->w * percentage;
        ch = o->h;
    }
    else
    {
        cw = o->w * percentage;
        ch = o->h;
    }

    cairo_rectangle(cr, cx, cy, cw, ch);
    cairo_fill(cr);

    if(image_surface)
    {
        cairo_surface_destroy(image_surface);
        image_surface = NULL;
    }

    for(unsigned char b = 0; b < 2; b++)
    {
        long rx = 0;
        long ry = 0;
        long rw = 0;
        long rh = 0;
        percentage = (b == 0 ? bo->percents : bo->percents_2);


        image_cache_query_image(sd->cd->ich,(b?&bo->b1ia:&bo->b2ia),&px,o->w,o->h);

        if(bo->vertical && !bo->flip)
        {
            rx = 0;
            ry = o->h - o->h * percentage;
            rw = o->w;
            rh = cy - ry;
        }
        else if(bo->vertical && bo->flip)
        {
            rh = o->h * percentage;
            ry = ch;
            rh = (long)((float)o->h * percentage) - ry;
            rw = o->w;
        }
        else if(bo->flip)
        {
            /*The starting point tends to 0 when percentage tends to 1*/
            /*the ending point is the difference between the common point and the starting point*/
            rx = o->w - o->w * percentage;
            rw = cx - rx;
            rh = o->h;
        }
        else
        {
            /*The X is has the starting point at the end of the common bar (CW)*/
            /*The width of the current bar has the width of percent of total width minus starting point*/
            rx = cw;
            rw = (long)((float)o->w * percentage) - rx;
            rh = o->h;
        }

        if(px)
        {
            cairo_set_color(cr,b == 0 ? bo->bar_color : bo->bar_color_2);
        }
        else
        {
            int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, o->auto_w);
            image_surface = cairo_image_surface_create_for_data(  px, CAIRO_FORMAT_ARGB32, o->w,o->h, stride);
            cairo_set_source_surface(cr, image_surface, 0.0, 0.0);
        }

        cairo_rectangle(cr, rx, ry, rw, rh);
        cairo_fill(cr);

        if(image_surface)
        {
            cairo_surface_destroy(image_surface);
            image_surface = NULL;
        }
    }

    image_cache_unref_image(sd->cd->ich, &bo->b1ia,0);
    image_cache_unref_image(sd->cd->ich, &bo->b2ia,0);
    image_cache_unref_image(sd->cd->ich, &bo->bcia,0);
    return (0);
}
