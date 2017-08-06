/*
String object
Part of Project "Semper"
Wrriten by Alexandru-Daniel Mărgărit
*/
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

#define PADDING_H 4
#define PADDING_W 4

void string_init(object* o)
{
    if(o)
    {
        o->pv = zmalloc(sizeof(string_object));
        string_object* so = o->pv;
        so->font_map = pango_cairo_font_map_new_for_font_type(CAIRO_FONT_TYPE_FT);
        so->context = pango_font_map_create_context(PANGO_FONT_MAP(so->font_map));
        so->layout = pango_layout_new(so->context);
        so->font_desc = pango_font_description_new();
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
    sfree((void**)&so->font_name);

    so->string = parameter_string(o, "Text", "%0", XPANDER_OBJECT);
    so->font_name = parameter_string(o, "FontName", "Arial", XPANDER_OBJECT);
    so->font_size = parameter_long_long(o, "FontSize", 12, XPANDER_OBJECT);
    so->decimals = parameter_byte(o, "Decimals", 0, XPANDER_OBJECT);
    so->font_color = parameter_color(o, "FontColor", 0xffffffff, XPANDER_OBJECT);
    so->font_shadow = parameter_bool(o, "FontShadow", 0, XPANDER_OBJECT);
    so->ellipsize=parameter_bool(o, "Ellipsize", 0, XPANDER_OBJECT);
    so->percentual = parameter_bool(o, "Percentual", 0, XPANDER_OBJECT);
    so->scale = parameter_double(o, "Scale", 0, XPANDER_OBJECT);
    so->scaling = parameter_self_scaling(o, "SelfScaling", 0, XPANDER_OBJECT);
    so->font_outline = parameter_bool(o, "FontOutline", 0, XPANDER_OBJECT);
    so->weight = parameter_size_t(o, "FontWeight", 400, XPANDER_OBJECT);
    so->italic = parameter_bool(o, "FontItalic", 0, XPANDER_OBJECT);
    so->weight = so->weight ? so->weight : 400;
    if(so->font_shadow)
    {
        so->shadow_color = parameter_color(o, "FontShadowColor", so->font_color & 0xff000000, XPANDER_OBJECT);
    }
    if(so->font_outline)
    {
        so->outline_color = parameter_color(o, "FontOutlineColor", so->font_color & 0xff000000, XPANDER_OBJECT);
    }
    temp = parameter_string(o, "StringAlign", "Left", XPANDER_OBJECT);

    strupr(temp);

    if(strstr(temp, "RIGHT"))
    {
        so->align = 1;
    }
    else if(strstr(temp, "CENTER"))
    {
        so->align = 2;
    }
    else
    {
        so->align = 0;
    }

    sfree((void**)&temp);

    temp = parameter_string(o, "StringCase", NULL, XPANDER_OBJECT);

    if(temp)
    {
        strupr(temp);

        if(strstr(temp, "UPPER"))
        {
            string_upper(so->string);
        }
        else if(strstr(temp, "LOWER"))
        {
            string_lower(so->string);
        }
    }
    
    
    pango_layout_set_wrap(so->layout,PANGO_WRAP_WORD_CHAR);
    /*Set the font description*/
    pango_font_description_set_family(so->font_desc, so->font_name);
    pango_font_description_set_size(so->font_desc, so->font_size * PANGO_SCALE);
    pango_font_description_set_style(so->font_desc, so->italic?PANGO_STYLE_ITALIC:PANGO_STYLE_NORMAL);
    pango_font_description_set_weight(so->font_desc, so->weight);
    /*Set the layout attributes*/
    pango_layout_set_font_description(so->layout, so->font_desc);
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
    sfree((void**)&temp);
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
    }


    if(o->w < 0)/*Adjust the size*/
    {
        pango_layout_set_width(so->layout, w);
        o->auto_w =(w!=0?(w >> 10) + PADDING_W:0);
    }
    else /*Size specified by the user so we'll use it*/
    {
        pango_layout_set_width(so->layout, (o->w - PADDING_W) << 10);
    }

    if(o->h < 0) /*Adjust the size*/
    {
        pango_layout_set_height(so->layout, h);
        o->auto_h = (h!=0?(h >> 10) + PADDING_H:0);
    }
    else /*Size specified by the user so we'll use it*/
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

    if(so->font_shadow && (so->shadow_color & 0xff000000))
    {
        cairo_move_to(cr, PADDING_W / 2.0 + 1.0, PADDING_H / 2.0 - 1.0);
        cairo_set_color(cr, so->shadow_color);
        pango_cairo_show_layout(cr, so->layout);
    }

    cairo_move_to(cr, PADDING_W / 2.0, PADDING_H / 2.0);
    cairo_set_color(cr, so->font_color);
    pango_cairo_show_layout(cr, so->layout);

    if(so->font_outline && (so->outline_color & 0xff000000))
    {
        cairo_set_line_width(cr, 1.0);
        cairo_set_color(cr,so->outline_color);
        pango_cairo_layout_path(cr, so->layout);
        cairo_stroke(cr);
    }
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
        sfree((void**)&(so)->font_name);
        if(so->bind_string!=so->string)
        {
            sfree((void**)&(so)->bind_string);
        }
        so->bind_string=NULL;
        sfree((void**)&(so)->string);
        sfree((void**)&o->pv);
    }
}
