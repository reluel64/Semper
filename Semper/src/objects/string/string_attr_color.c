/*
String object color attribute
Part of Project "Semper"
Wrriten by Alexandru-Daniel Mărgărit
 */
#include <pango/pango.h>
#include <pango/pangocairo.h>
#include <mem.h>
#include <semper_api.h>
#include <objects/string/string_attr.h>
#include <objects/string.h>
#include <string_util.h>
#include <cairo/cairo.h>

typedef struct
{
        PangoAttribute attr;
        string_attributes *sa;
} StringAttrColor;

static PangoAttribute *string_attr_color_copy(const PangoAttribute *pa)
{
    StringAttrColor *attr = zmalloc(sizeof(StringAttrColor));
    StringAttrColor *src = (StringAttrColor*)pa;
    memcpy(attr, src, sizeof(StringAttrColor));
    return(&attr->attr);
}

static void string_attr_color_destroy(PangoAttribute *pa1)
{

    sfree((void**)&pa1);
}

static int string_attr_color_compare(const PangoAttribute *pa1, const PangoAttribute *pa2)
{
    StringAttrColor *attr1 = pa1;
    StringAttrColor *attr2 = pa2;
    unused_parameter(pa1);
    unused_parameter(pa2);
    /*
    if(attr1->sa->font_color == 0x800094ff)
    {
        printf("0x%x\n",attr1->sa->font_color);
    }

    if(attr2->sa->font_color == 0x800094ff)
       {
           printf("0x%x\n",attr2->sa->font_color);
       }*/

    return(attr1->sa == attr2->sa);
}

PangoAttribute *string_attr_color(string_attributes *sa)
{
    static PangoAttrClass cls =
    {
            .type = STRING_ATTR_COLOR,
            .copy = string_attr_color_copy,
            .destroy = string_attr_color_destroy,
            .equal = string_attr_color_compare
    };

    StringAttrColor *attr = zmalloc(sizeof(StringAttrColor));
    attr->attr.klass = &cls;
    attr->sa = sa;
    return(&attr->attr);
}

PangoAttribute *string_attr_shadow( string_attributes *sa)
{
    static PangoAttrClass cls =
    {
            .type = STRING_ATTR_SHADOW,
            .copy = string_attr_color_copy,
            .destroy = string_attr_color_destroy,
            .equal = string_attr_color_compare
    };

    StringAttrColor *attr = zmalloc(sizeof(StringAttrColor));
    attr->attr.klass = &cls;
    attr->sa = sa;
    return(&attr->attr);
}


PangoAttribute *string_attr_outline(string_attributes *sa)
{
    static PangoAttrClass cls =
    {
            .type = STRING_ATTR_OUTLINE,
            .copy = string_attr_color_copy,
            .destroy = string_attr_color_destroy,
            .equal = string_attr_color_compare
    };

    StringAttrColor *attr = zmalloc(sizeof(StringAttrColor));
    attr->attr.klass = &cls;
    attr->sa = sa;
    return(&attr->attr);
}


int string_attr_color_handler(PangoAttribute *pa, void *pv)
{
    void **pm = (void**)pv;

    if((pa->klass->type < STRING_ATTR_GRADIENT || pa->klass->type >STRING_ATTR_SHADOW)  || ((StringAttrColor*)pa)->sa == NULL)
    {
        return(0);
    }

    string_object *so = (string_object*)pm[0];
    cairo_t *cr = (cairo_t*)pm[1];
    string_attributes *sa = ((StringAttrColor*)pa)->sa;
    PangoLayoutIter *iter = pango_layout_iter_copy(so->iter);

    double offx = 0.0;
    double offy = 0.0;

    if((pa->klass->type == STRING_ATTR_SHADOW  && pm[2] != (void*)ATTR_COLOR_SHADOW)||
       (pa->klass->type == STRING_ATTR_OUTLINE && pm[2] != (void*)ATTR_COLOR_OUTLINE))
    {
        return(0);
    }
    cairo_save(cr);

    if(pa->klass->type == STRING_ATTR_SHADOW)
    {
        offx = sa->shadow_x;
        offy = sa->shadow_y;
    }


    do
    {
        cairo_save(cr);

        cairo_pattern_t *pattern = NULL;
        PangoLayoutLine *line = NULL;
        PangoRectangle lpr={0};
        cairo_rectangle_t gradient_dim={0};
        int baseline = 0;
        char full_line = 0;
        line = pango_layout_iter_get_line_readonly(iter);

        if(line == NULL)
            continue;

        if(line->length + line->start_index < pa->start_index)
            continue;

        if(line->start_index > pa->end_index)
        {
            break;
        }
        pango_layout_iter_get_line_extents(iter,NULL,&lpr);
        baseline = pango_layout_iter_get_baseline(iter);


        /* Go unrestricted*/
        if(pm[2] ==(void*)ATTR_COLOR_SHADOW)
        {
            cairo_set_color(cr,sa->shadow_color);
        }
        else if(pm[2]== (void*)ATTR_COLOR_OUTLINE)
        {
            cairo_set_color(cr,sa->outline_color);
        }
        else
        {
            cairo_set_color(cr,sa->font_color);
        }

        full_line = (line->start_index >= pa->start_index && ((pa->end_index >= line->start_index + line->length)));

        if(sa->gradient_len>=2 && pm[2] != (void*)ATTR_COLOR_SHADOW)
        {
            gradient_dim.height = (double)(lpr.height >> 10);
            gradient_dim.width = (double)(lpr.width >> 10);
            gradient_dim.x = (double)(lpr.x >> 10);
            gradient_dim.y = (double)((lpr.y) >> 10);
        }

        if(!full_line)
        {
            PangoLayoutIter *ch_iter = pango_layout_iter_copy(iter);

            do
            {
                PangoRectangle rpr = {0};
                size_t index = pango_layout_iter_get_index(ch_iter);

                if(index < pa->start_index||index < line->start_index)
                    continue;

                if(index >= pa->end_index || index > line->start_index + line->length)
                {
                    break;
                }

                pango_layout_iter_get_char_extents (ch_iter,&rpr);
                cairo_rectangle(cr,(double)(rpr.x>>10)+offx,(double)(rpr.y>>10)+offy,(double)(rpr.width>>10),(double)(rpr.height>>10));

            }while(pango_layout_iter_next_char (ch_iter));

            cairo_clip(cr);

            if(sa->gradient_len>=2 && pm[2] != (void*)ATTR_COLOR_SHADOW)
            {
                cairo_clip_extents (cr,&gradient_dim.x,&gradient_dim.y,&gradient_dim.width,&gradient_dim.height);
            }

            pango_layout_iter_free(ch_iter);
        }

        if(sa->gradient_len >= 2 && pm[2] !=(void*)ATTR_COLOR_SHADOW)
        {
            pattern = cairo_pattern_create_linear(gradient_dim.x,
                    gradient_dim.y + gradient_dim.height / 2.0,
                    gradient_dim.x + gradient_dim.width,
                    gradient_dim.y + gradient_dim.height / 2.0);
            cairo_matrix_t m = { 0 };

            cairo_matrix_init_translate(&m,gradient_dim.x, gradient_dim.y);

            cairo_matrix_translate(&m, gradient_dim.width/ 2.0, gradient_dim.height / 2.0);
            cairo_matrix_rotate(&m, DEG2RAD(sa->gradient_ang));
            cairo_matrix_translate(&m, - gradient_dim.width / 2.0, - gradient_dim.height / 2.0);
            cairo_matrix_translate(&m, -gradient_dim.x, -gradient_dim.y);

            cairo_pattern_set_matrix(pattern, &m);
            cairo_set_source(cr, pattern);

            for(size_t i = 0; i < sa->gradient_len; i++)
            {
                unsigned char *color = (unsigned char*)&sa->gradient[i];
                double red = (double)color[2] / 255.0;
                double green = (double)color[1] / 255.0;
                double blue = (double)color[0] / 255.0;
                double alpha = (double)color[3] / 255.0;
                double pos = (double)(i == 0 ? i : i + 1) / (double)sa->gradient_len;
                cairo_pattern_add_color_stop_rgba(pattern, (double)pos, red, green, blue, alpha);
            }
        }

        cairo_move_to (cr,(double)(lpr.x>>10)+offx,(double)(baseline>>10)+offy);
        pango_cairo_layout_line_path (cr,line);

        if(pa->klass->type==STRING_ATTR_OUTLINE)
            cairo_stroke(cr);
        else
            cairo_fill(cr);

        if(pattern)
        {
            cairo_pattern_destroy(pattern);
        }
        cairo_restore(cr);

    }while(pango_layout_iter_next_line(iter));

    pango_layout_iter_free(iter);
    cairo_restore(cr);
    return(0);
}
