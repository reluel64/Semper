/*
 * Image object
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 * */

#include <surface.h>
#include <objects/image.h>
#include <string.h>
#include <image/image_cache.h>
#include <mem.h>
#include <parameter.h>
#include <bind.h>

void image_destroy(object* o)
{
    image_object* io = o->pv;
    surface_data *sd = o->sd;

    if(io)
    {
        image_cache_unref_image(sd->cd->ich, &io->ia, 1);

        if(io->ia.path != io->image_path)
        {
            sfree((void**)&io->ia.path);
        }

        io->ia.path = NULL;
        sfree((void**)&io->image_path);
        sfree((void**)&o->pv);
    }
}
void image_init(object* o)
{
    if(o && o->pv == NULL)
    {
        o->pv = zmalloc(sizeof(image_object));
    }
}

void image_reset(object* o)
{
    image_object* io = o->pv;
    io->ia.path = NULL;
    sfree((void**)&io->image_path);
    io->image_path = parameter_string(o, "ImagePath", "%0", XPANDER_OBJECT);
    image_cache_image_parameters(o, &io->ia, XPANDER_OBJECT, NULL);
}

int image_update(object* o)
{
    surface_data* sd = o->sd;
    image_object* io = o->pv;
    string_bind sb = { 0 };
    image_cache_unref_image(sd->cd->ich, &io->ia, 0);

    if(io->ia.path != io->image_path)
    {
        sfree((void**)&io->ia.path);
    }

    io->ia.path = NULL;
    sb.s_in = io->image_path;
    bind_update_string(o, &sb);

    if(sb.s_out == NULL)
    {
        return (-1);
    }

    io->ia.path = sb.s_out;
    image_cache_query_image(sd->cd->ich, &io->ia, NULL, o->w, o->h);


    o->auto_w = io->ia.width;
    o->auto_h = io->ia.height;
    return (1);
}

int image_render(object* o, cairo_t* cr)
{
    unsigned char *px = NULL;
    surface_data *sd = o->sd;
    image_object* io = o->pv;


    image_cache_query_image(sd->cd->ich, &io->ia, &px, o->w, o->h);


    if(px == NULL)
    {
        return(-1);
    }

    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, o->auto_w);
    cairo_surface_t* image = cairo_image_surface_create_for_data(px, CAIRO_FORMAT_ARGB32, o->auto_w, o->auto_h, stride);

    cairo_set_source_surface(cr, image, 0, 0);
    cairo_paint(cr);
    cairo_surface_destroy(image);
    image_cache_unref_image(sd->cd->ich, &io->ia, 0);
    return (0);
}
