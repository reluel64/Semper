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

extern int string_attr_color_handler(PangoAttribute *pa,void *pv);

void string_init(object* o)
{
    if(o)
    {
        o->pv = zmalloc(sizeof(string_object));
        string_object* so = o->pv;
        so->font_map = pango_cairo_font_map_new_for_font_type(CAIRO_FONT_TYPE_FT);
        so->context = pango_font_map_create_context(PANGO_FONT_MAP(so->font_map));
        so->layout = pango_layout_new(so->context);
        list_entry_init(&so->attr);
    }
}


void string_reset(object* o)
{

    string_object* so = o->pv;
    unsigned char* temp = NULL;

    if(so->bind_string!=so->string)
    {
        sfree((void**)&(so)->bind_string);
    }

    so->bind_string=NULL;
    sfree((void**)&so->string);

    so->string = parameter_string(o, "Text", "%0", XPANDER_OBJECT);
    so->decimals = parameter_byte(o, "Decimals", 0, XPANDER_OBJECT);
    so->ellipsize=parameter_bool(o, "Ellipsize", 0, XPANDER_OBJECT);
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


    string_destroy_attrs(so);
    string_fill_attrs(o);

    if(linked_list_single(&so->attr))
    {
        string_apply_attr(so);
    }
    /*Set the layout attributes*/
    pango_layout_set_wrap(so->layout,PANGO_WRAP_WORD_CHAR);
    pango_layout_set_ellipsize(so->layout, so->ellipsize?PANGO_ELLIPSIZE_END:PANGO_ELLIPSIZE_NONE);

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

    if(so->bind_string!=so->string)
    {
        sfree((void**)&(so)->bind_string);
    }
    so->bind_string=NULL;

    sb.decimals = so->decimals;
    sb.percentual = so->percentual;
    sb.s_in = so->string;
    sb.scale = so->scale;
    sb.self_scaling = so->scaling;

    /*Obtain the string*/
    bind_update_string(o, &sb);

    if(sb.s_out == NULL)
    {
        return (-1);
    }

    so->bind_string = sb.s_out;

    if(linked_list_single(&so->attr)==0)
    {
        string_apply_attr(so);
    }

    if(o->w < 0)
    {
        pango_layout_set_width(so->layout, -1);
    }
    if(o->h < 0)
    {
        pango_layout_set_height(so->layout, -1);
    }

    pango_layout_set_text(so->layout, so->bind_string, -1);

    /*get the layout size*/
    if(o->w < 0 || o->h < 0)
    {
        pango_layout_get_size(so->layout, (int*)&w, (int*)&h);
        w>>=10;
        h>>=10;
    }

    if(o->w < 0)
    {
        pango_layout_set_width(so->layout, w<<10);
        o->auto_w =(w!=0?w + PADDING_W:0);
    }
    else
    {
        pango_layout_set_width(so->layout, (o->w - PADDING_W) << 10);
    }

    if(o->h < 0)
    {
        pango_layout_set_height(so->layout, h<<10);
        o->auto_h = (h!=0?h + PADDING_H:0);
    }
    else
    {
        pango_layout_set_height(so->layout, (o->h - PADDING_H) << 10);
    }


    return (0);
}

int string_render(object* o, cairo_t* cr)
{
    string_object* so = o->pv;
    double clipw=(double)(o->w<0?o->auto_w:o->w);
    double cliph=(double)(o->h<0?o->auto_h:o->h);

    cairo_rectangle(cr,0.0,0.0,clipw,cliph);
    cairo_clip(cr);

    for(unsigned char i=2; i; i--)
    {
        void **pm[]= {(void*)so,(void*)cr,(void*)i-1};
        pango_attr_list_filter(so->attr_list,string_attr_color_handler,(void*)pm);
    }
    so->was_outlined=0;

    return (0);
}


void string_destroy(object* o)
{
    if(o && o->pv)
    {
        string_object* so = o->pv;
        g_object_unref(so->layout);
        g_object_unref(so->context);
        g_object_unref(so->font_map);
        pango_font_description_free(so->font_desc);
        string_destroy_attrs(so);
        if(so->bind_string!=so->string)
        {
            sfree((void**)&(so)->bind_string);
        }
        so->bind_string=NULL;
        sfree((void**)&(so)->string);
        sfree((void**)&o->pv);
    }
}
