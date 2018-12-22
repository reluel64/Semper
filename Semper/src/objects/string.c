/*
String object
Part of Project "Semper"
Wrriten by Alexandru-Daniel Mărgărit
 */
#define PCRE_STATIC
#include <objects/string.h>
#include <mem.h>
#include <surface.h>
#include <bind.h>
#include <string_util.h>
#include <cairo/cairo.h>
#include <xpander.h>
#include <parameter.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>
#include <pango/pangoft2.h>
#include <fontconfig/fontconfig.h>
#include <objects/string/string_attr.h>

extern int string_attr_color_handler(PangoAttribute *pa, void *pv);

void string_init(object* o)
{
    if(o)
    {
        o->pv = zmalloc(sizeof(string_object));
        string_object* so = o->pv;
        so->context = pango_context_new();
        so->layout = pango_layout_new(so->context);

        list_entry_init(&so->attr);
    }
}


void string_reset(object* o)
{

    string_object* so = o->pv;
    unsigned char* temp = NULL;

    if(so->bind_string != so->string)
    {
        sfree((void**) & (so)->bind_string);
    }

    pango_context_set_font_map(so->context, pango_cairo_font_map_get_default());

    so->bind_string = NULL;
    sfree((void**)&so->string);

    so->string = parameter_string(o, "Text", "%0", XPANDER_OBJECT);
    so->decimals = parameter_byte(o, "Decimals", 0, XPANDER_OBJECT);
    so->ellipsize = parameter_bool(o, "Ellipsize", 0, XPANDER_OBJECT);
    so->percentual = parameter_bool(o, "Percentual", 0, XPANDER_OBJECT);
    so->scale = parameter_double(o, "Scale", 0, XPANDER_OBJECT);
    so->scaling = parameter_self_scaling(o, "SelfScaling", 0, XPANDER_OBJECT);
    temp = parameter_string(o, "StringAlign", "Left", XPANDER_OBJECT);

    strupr(temp);

    if(strstr(temp, "RIGHT"))
        so->align = 1;
    else if(strstr(temp, "CENTER"))
        so->align = 2;
    else
        so->align = 0;

    sfree((void**)&temp);


    string_attr_destroy(so);
    string_attr_init(o);
    so->bind_string = so->string;
    so->bind_string_len = string_length(so->string);

    if(linked_list_single(&so->attr))
    {
        string_attr_update(so);
    }

    /*Set the layout attributes*/
    pango_layout_set_wrap(so->layout, PANGO_WRAP_WORD_CHAR);
    pango_layout_set_ellipsize(so->layout, so->ellipsize ? PANGO_ELLIPSIZE_END : PANGO_ELLIPSIZE_NONE);

    switch(so->align)
    {
        case 1:
            pango_layout_set_alignment(so->layout, PANGO_ALIGN_RIGHT);
            break;

        case 2:
            pango_layout_set_alignment(so->layout, PANGO_ALIGN_CENTER);
            break;

        default:
            pango_layout_set_alignment(so->layout, PANGO_ALIGN_LEFT);
            break;
    }
}

int string_update(object* o)
{
    string_bind sb = { 0 };
    string_object* so = o->pv;
    PangoRectangle pr ={0};
    long w = 0;
    long h = 0;

    if(o->hidden)
    {
        return (0);
    }

    if(so == NULL)
    {
        return (-1);
    }

    if(so->bind_string != so->string)
    {
        sfree((void**) & (so)->bind_string);
    }

    so->bind_string = NULL;

    sb.decimals = so->decimals;
    sb.percentual = so->percentual;
    sb.s_in = so->string;
    sb.scale = so->scale;
    sb.self_scaling = so->scaling;
    pango_context_set_font_map(so->context, pango_cairo_font_map_get_default());
    /*Obtain the string*/
    so->bind_string_len = bind_update_string(o, &sb);

    if(sb.s_out == NULL)
    {
        return (-1);
    }

    so->bind_string = sb.s_out;

    if(linked_list_single(&so->attr) == 0)
    {
        string_attr_update(so);
    }


    pango_layout_set_width(so->layout, -1);
    pango_layout_set_height(so->layout, -1);


    pango_layout_set_text(so->layout, so->bind_string, -1);

    if(o->w >= 0)
    {
        pango_layout_set_width(so->layout, (o->w) << 10);
    }

    if(o->h >= 0)
    {
        pango_layout_set_height(so->layout, (o->h) << 10);
    }

    if(so->iter)
    {
        pango_layout_iter_free(so->iter);
    }

    so->iter = pango_layout_get_iter(so->layout);

    /*get the layout size*/

    pango_layout_iter_get_layout_extents(so->iter, NULL,&pr);

    w =  (pr.width  >> 10);
    h =  (pr.height >> 10);
    so->layout_x = (long)(pr.x >> 10);
    so->layout_y = (long)(pr.y >> 10);

    if(o->w < 0)
    {
        o->auto_w = (w != 0 ? w + so->layout_x : 0);
    }

    if(o->h < 0)
    {
        o->auto_h = (h != 0 ? h + so->layout_y  : 0);
    }

    return (0);
}

int string_render(object* o, cairo_t* cr)
{
    string_object* so = o->pv;
    double clipw = (double)(o->w < 0 ? o->auto_w : o->w);
    double cliph = (double)(o->h < 0 ? o->auto_h : o->h);

    cairo_rectangle(cr, 0.0, 0.0, clipw, cliph);
    cairo_clip(cr);

    for(unsigned char i = ATTR_COLOR_SHADOW; i <= ATTR_COLOR_OUTLINE ; i++)
    {
        void **pm[] =
        {
                (void*)so,
                (void*)cr,
                (void*)(size_t)i
        };
        pango_attr_list_filter(so->attr_list, string_attr_color_handler, (void*)pm);
    }

    so->was_outlined = 0;

    return (0);
}


void string_destroy(object* o)
{
    if(o && o->pv)
    {
        string_object* so = o->pv;
        string_attr_destroy(so);
        g_object_unref(so->layout);
        g_object_unref(so->context);
        pango_font_description_free(so->font_desc);

        if(so->iter)
        {
            pango_layout_iter_free(so->iter);
        }

        if(so->bind_string != so->string)
        {
            sfree((void**) & (so)->bind_string);
        }

        so->bind_string = NULL;
        sfree((void**) & (so)->string);
        sfree((void**)&o->pv);
    }
}
