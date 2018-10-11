/*
*Histogram object
*Part of Project 'Semper'
*Written by Alexandru-Daniel Mărgărit
*/
#include <surface.h>
#include <string.h>
#include <bind.h>
#include <objects/object.h>
#include <objects/histogram.h>
#include <mem.h>

#include <string_util.h>
#include <xpander.h>
#include <semper.h>
#include <image/image_cache.h>
#include <parameter.h>

#warning "Code needs testing"

void histogram_init(object* o)
{
    if(o && o->pv == NULL)
    {
        o->pv = zmalloc(sizeof(histogram_object));
        histogram_object* ho = o->pv;
        list_entry_init(&ho->val_1);
        list_entry_init(&ho->val_2);
    }
}

void histogram_reset(object* o)
{
    histogram_object* ho = o->pv;
    ho->hist_color = parameter_color(o, "HistogramColor", 0xff00ff00, XPANDER_OBJECT);
    ho->hist_2_color = parameter_color(o, "HistogramColor2", 0xffff0000, XPANDER_OBJECT);
    ho->hist_over_color = parameter_color(o, "HistogramColorOverlap", 0xffffff00, XPANDER_OBJECT);
    ho->reverse_origin = parameter_bool(o, "ReverseOrigin", 0, XPANDER_OBJECT);
    ho->vertical = parameter_bool(o, "Vertical", 0, XPANDER_OBJECT);
    ho->flip = parameter_bool(o, "Flip", 0, XPANDER_OBJECT);
    ho->straight = !(((long)o->angle) % 90);
    size_t dim = 0;

    if(ho->vertical == 0)
        dim = (size_t)labs(o->w);
    else
        dim = (size_t)labs(o->h);

    ho->max_hist = dim;

    if(ho->max_hist < ho->v_count)
    {
        histogram_value* hv = NULL;
        histogram_value* thv = NULL;
        size_t i = ho->v_count - ho->max_hist;

        list_enum_part_backward_safe(hv, thv, &ho->val_1, current)
        {
            if(i == 0)
                break;

            linked_list_remove(&hv->current);
            sfree((void**)&hv);
            i--;
        }

        i = ho->v_count - ho->max_hist;

        list_enum_part_backward_safe(hv, thv, &ho->val_2, current)
        {
            if(i == 0)
                break;

            linked_list_remove(&hv->current);
            sfree((void**)&hv);
            i--;
        }
         ho->v_count=ho->max_hist;
    }
#if 0
    image_cache_unref_image(sd->cd->ich, &ho->h1ia,0);
     image_cache_unref_image(sd->cd->ich, &ho->h2ia,0);
     image_cache_unref_image(sd->cd->ich, &ho->hcia,0);
#endif
    sfree((void**)&ho->h1ia.path);
    sfree((void**)&ho->h2ia.path);
    sfree((void**)&ho->hcia.path);

    ho->h1ia.path = parameter_string(o, "HistogramImagePath", NULL, XPANDER_OBJECT);
    ho->h2ia.path = parameter_string(o, "Histogram2ImagePath", NULL, XPANDER_OBJECT);
    ho->hcia.path = parameter_string(o, "OverlapImagePath", NULL, XPANDER_OBJECT);

    if(ho->h1ia.path)
    {
        image_cache_image_parameters(o, &ho->h1ia, XPANDER_OBJECT, "Histogram");
    }

    if(ho->h2ia.path)
    {
        image_cache_image_parameters(o, &ho->h2ia, XPANDER_OBJECT, "Histogram2");
    }

    if(ho->hcia.path)
    {
        image_cache_image_parameters(o, &ho->hcia, XPANDER_OBJECT, "Overlap");
    }
}

void histogram_destroy(object* o)
{
    histogram_object* ho = o->pv;
    surface_data *sd = o->sd;

    if(ho)
    {
        histogram_value* hv = NULL;
        histogram_value* thv = NULL;

        list_enum_part_safe(hv, thv, &ho->val_1, current)
        {
            linked_list_remove(&hv->current);
            sfree((void**)&hv);
        }

        list_enum_part_safe(hv, thv, &ho->val_2, current)
        {
            linked_list_remove(&hv->current);
            sfree((void**)&hv);
        }

        image_cache_unref_image(sd->cd->ich, &ho->h1ia, 1);
        image_cache_unref_image(sd->cd->ich, &ho->h2ia, 1);
        image_cache_unref_image(sd->cd->ich, &ho->hcia, 1);

        sfree((void**)&ho->h1ia.path);
        sfree((void**)&ho->h2ia.path);
        sfree((void**)&ho->hcia.path);
        sfree((void**)&o->pv);
    }
}


int histogram_update(object* o)
{
    surface_data* sd = o->sd;
    histogram_object* ho = o->pv;
    bind_numeric bn = { 0 };
    bind_numeric bn_2 = { 0 };

    bn.index = 0;
    bn_2.index = 1;
    bind_update_numeric(o, &bn);
    bind_update_numeric(o, &bn_2);

    if(bn.max > ho->max_val_1)
    {
        ho->max_val_1 = bn.max;
    }

    if(bn.min < ho->min_val_1)
    {
        ho->min_val_1 = bn.min;
    }

    if(bn_2.max > ho->max_val_2)
    {
        ho->max_val_2 = bn_2.max;
    }

    if(bn_2.min < ho->min_val_2)
    {
        ho->min_val_2 = bn_2.min;
    }

    if(ho->max_hist > ho->v_count)
    {
        histogram_value* hv1 = zmalloc(sizeof(histogram_value));
        histogram_value* hv2 = zmalloc(sizeof(histogram_value));
        list_entry_init(&hv1->current);
        list_entry_init(&hv2->current);
        hv1->value = bn.val;
        hv2->value = bn_2.val;
        linked_list_add(&hv1->current, &ho->val_1);
        linked_list_add(&hv2->current, &ho->val_2);
        ho->v_count++;
    }
    else
    {
        histogram_value* hv1 = NULL;
        histogram_value* hv2 = NULL;

        if(!linked_list_empty(&ho->val_1))
        {
            hv1 = element_of(ho->val_1.prev, hv1, current);
            linked_list_remove(&hv1->current);
            list_entry_init(&hv1->current);
            linked_list_add(&hv1->current,&ho->val_1);
            hv1->value = bn.val;
        }

        if(!linked_list_empty(&ho->val_2))
        {
            hv2 = element_of(ho->val_2.prev, hv2, current);
            linked_list_remove(&hv2->current);
            list_entry_init(&hv2->current);
            linked_list_add(&hv2->current,&ho->val_2);
            hv2->value = bn_2.val;

        }
    }

    image_cache_unref_image(sd->cd->ich, &ho->h1ia, 0);
    image_cache_unref_image(sd->cd->ich, &ho->h2ia, 0);
    image_cache_unref_image(sd->cd->ich, &ho->hcia, 0);

    image_cache_query_image(sd->cd->ich, &ho->h1ia, NULL, o->w, o->h);
    image_cache_query_image(sd->cd->ich, &ho->h2ia, NULL, o->w, o->h);
    image_cache_query_image(sd->cd->ich, &ho->hcia, NULL, o->w, o->h);

    return (1);
}

/*The rendering of the histogram should be like this:
 * 1)When there are two Sources present
 * 		a)Render the common part
 * 		b)Render the individual part(max value)
 * 2)When there is a single source present
 * 		a)Render the histogram that belongs to the source
 * 3)Aditionally, if the percentage is 0 at any given moment, corresponding bar of the histogram will not be rendered
 */

int histogram_render(object* o, cairo_t* cr)
{

    cairo_surface_t* image_surface = NULL;
    histogram_object* ho = o->pv;
    surface_data *sd = o->sd;
    unsigned char *px = NULL;
    unsigned char *px1 = NULL;
    unsigned char *px2 = NULL;


    if(ho->straight)
    {
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE); //causes shit to appear if it's enabled while drawing straight lines
    }

    cairo_set_line_width(cr, 1.0);

    image_cache_query_image(sd->cd->ich, &ho->hcia, &px,  o->w, o->h);
    image_cache_query_image(sd->cd->ich, &ho->h1ia, &px1, o->w, o->h);
    image_cache_query_image(sd->cd->ich, &ho->h2ia, &px2, o->w, o->h);

    if(px == NULL)
    {
        cairo_set_color(cr, ho->hist_over_color);
    }
    else
    {
        int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, o->w);
        image_surface = cairo_image_surface_create_for_data(px, CAIRO_FORMAT_ARGB32, o->w, o->h, stride);
        cairo_set_source_surface(cr, image_surface, 0.0, 0.0);
    }

    /*Render the common part*/
    histogram_value* hv1 = element_of(ho->val_1.next, hv1, current);
    histogram_value* hv2 = element_of(ho->val_2.next, hv2, current);

    for(size_t i = 0; i < ho->v_count; i++)
    {

        double p1 = bind_percentual_value(hv1->value, ho->min_val_1, ho->max_val_1);
        double p2 = bind_percentual_value(hv2->value, ho->min_val_2, ho->max_val_2);

        double common = min(p1, p2);
        double cy = o->h;
        double cx = (ho->max_hist - i);
        double cw = -1.0;
        double ch = -((double)o->h * common);

        if(ho->flip)
        {
            cy = 0.0;
            ch = -ch;
        }

        if(ho->reverse_origin)
        {
            cx = i;
            cw = 1.0;
        }

        if(ho->vertical)
        {
            cx = o->w;
            cy = (ho->max_hist - i);
            ch = -1.0;
            cw = -((double)o->w * common);

            if(ho->flip)
            {
                cx = 0.0;
                cw = -cw;
            }

            if(ho->reverse_origin)
            {
                cy = i;
                ch = 1.0;
            }
        }

        cairo_rectangle(cr, cx, cy, cw, ch);
        hv1 = element_of(hv1->current.next, hv1, current);
        hv2 = element_of(hv2->current.next, hv2, current);
    }

    cairo_fill(cr);

    if(image_surface)
    {
        cairo_surface_destroy(image_surface);
        image_surface = NULL;
    }

    for(unsigned char h = 0; h < 2; h++)
    {

        hv1 = element_of(ho->val_1.next, hv1, current);
        hv2 = element_of(ho->val_2.next, hv2, current);

        for(size_t i = 0; i < ho->v_count; i++)
        {
            double p1 = bind_percentual_value(hv1->value, ho->min_val_1, ho->max_val_1);
            double p2 = bind_percentual_value(hv2->value, ho->min_val_2, ho->max_val_2);

            float common = min(p1, p2);
            float current = (h == 1 ? p2 : p1);

            double cx = (ho->max_hist - i);
            double cw = -1.0;

            double cy = (double)o->h - ((double)o->h * common);
            double ch = -(cy - (o->h - ((double)o->h * current)));

            if(ho->flip)
            {
                cy = o->h * common;
                ch = -ch;
            }

            if(ho->reverse_origin)
            {
                cx = i;
                cw = 1.0;
            }

            if(ho->vertical)
            {
                cx = (double)o->w - ((double)o->w * common);
                cy = (ho->max_hist - i);
                ch = -1.0;
                cw = -(cx - (o->w - ((double)o->w * current)));

                if(ho->flip)
                {
                    cx = o->w * common;
                    cw = -cw;
                }

                if(ho->reverse_origin)
                {
                    cy = i;
                    ch = 1.0;
                }
            }

            cairo_rectangle(cr, cx, cy, cw, ch);

            hv1 = element_of(hv1->current.next, hv1, current);
            hv2 = element_of(hv2->current.next, hv2, current);
        }

        if((h == 0 ? px1 : px2) == NULL)
        {
            cairo_set_color(cr, (h == 1 ? ho->hist_2_color : ho->hist_color));
        }
        else
        {
            int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, o->auto_w);
            image_surface = cairo_image_surface_create_for_data((h == 0 ? px1 : px2), CAIRO_FORMAT_ARGB32, o->w, o->h, stride);
            cairo_set_source_surface(cr, image_surface, 0.0, 0.0);
        }

        cairo_fill(cr);

        if(image_surface)
        {
            cairo_surface_destroy(image_surface);
            image_surface = NULL;
        }
    }

    image_cache_unref_image(sd->cd->ich, &ho->h1ia, 0);
    image_cache_unref_image(sd->cd->ich, &ho->h2ia, 0);
    image_cache_unref_image(sd->cd->ich, &ho->hcia, 0);
    return (0);
}
