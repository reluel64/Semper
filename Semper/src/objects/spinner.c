/*Spinner object
 *Part of Project 'Semper'
 *Written by Alexandru-Daniel Mărgărit
 */
#include <objects/spinner.h>
#include <mem.h>
#include <parameter.h>
#include <bind.h>
#include <cairo/cairo.h>
void spinner_init(object* o)
{
    if(o)
    {
        o->pv = zmalloc(sizeof(spinner_object));
    }
}

void spinner_destroy(object* o)
{
    if(o)
    {
        surface_data *sd=o->sd;
        spinner_object* spo = o->pv;
        image_cache_unref_image(sd->cd->ich, &spo->sia,1);
        sfree((void**)&spo->spinner_img_path);
        sfree((void**)&spo->sia.path);
        sfree((void**)&o->pv);
    }
}

void spinner_reset(object* o)
{
    spinner_object* spo = o->pv;

    sfree((void**)&spo->spinner_img_path);

    spo->ox = parameter_long_long(o, "XOFFSET", 0, XPANDER_OBJECT);
    spo->oy = parameter_long_long(o, "YOffset", 0, XPANDER_OBJECT);
    spo->spin_angle = parameter_double(o, "SpinAngle", 360.0, XPANDER_OBJECT);
    spo->start_angle = parameter_double(o, "StartAngle", 0, XPANDER_OBJECT);
    spo->spinner_img_path = parameter_string(o, "SpinnerImage", "%0", XPANDER_OBJECT);
    /*  image_cache_unref_image(sd->cd->ich, &spo->sia,0);*/
    image_cache_image_parameters(o, &spo->sia, XPANDER_OBJECT, "Spinner");
}

int spinner_update(object* o)
{
    if(o == NULL || o->pv == NULL)
    {
        return (-1);
    }

    spinner_object* spo = o->pv;
    string_bind sb = { 0 };
    bind_numeric bn = { 0 };
    surface_data* sd = o->sd;
    bind_update_numeric(o, &bn);
    image_cache_unref_image(sd->cd->ich, &spo->sia,0);

    if(spo->sia.path!=spo->spinner_img_path)
    {
        sfree((void**)&spo->sia.path);
    }

    sb.s_in = spo->spinner_img_path;

    bind_update_string(o, &sb);

    if(sb.s_out == NULL)
    {
        return (-1);
    }

    spo->sia.path=sb.s_out;
    image_cache_query_image(sd->cd->ich, &spo->sia, NULL, -1, -1);

    double percentage = bind_percentual_value(bn.val, bn.min, bn.max);

    long big_val = o->auto_w = labs(spo->sia.width + spo->ox);

    if(big_val < labs(spo->sia.height + spo->oy))
    {
        big_val = labs(spo->sia.height);
    }

    big_val *= 1.44;

    if(o->w < 0)
    {
        o->auto_w = big_val;
    }

    if(o->h < 0)
    {
        o->auto_h = big_val;
    }

    spo->current_angle = spo->start_angle + ((double)(spo->spin_angle) * percentage);
    return (0);
}

int spinner_render(object* o, cairo_t* cr)
{
    spinner_object* spo = o->pv;
    surface_data *sd=o->sd;
    unsigned char *pxx=NULL;
    image_cache_query_image(sd->cd->ich, &spo->sia, &pxx, -1, -1);
    long w = o->w < 0 ? o->auto_w : o->w;
    long h = o->h < 0 ? o->auto_h : o->h;
    double px = spo->ox;
    double py = spo->oy;
    cairo_translate(cr, -(double)spo->sia.width / 2.0, -(double)spo->sia.height / 2.0);
    cairo_translate(cr, ((double)w / 2.0), (double)h / 2.0);

    cairo_translate(cr, px, py);
    cairo_rotate(cr, DEG2RAD(spo->current_angle));
    cairo_translate(cr, -px, -py);

    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, spo->sia.width);
    cairo_surface_t* image = cairo_image_surface_create_for_data(pxx, CAIRO_FORMAT_ARGB32, spo->sia.width, spo->sia.height, stride);
    cairo_set_source_surface(cr, image, 0, 0);
    cairo_paint(cr);
    cairo_surface_destroy(image);
    image_cache_unref_image(sd->cd->ich, &spo->sia,0);
    return (0);
}
