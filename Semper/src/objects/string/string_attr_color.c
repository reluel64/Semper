
#include <pango/pango.h>
#include <pango/pangocairo.h>
#include <mem.h>
#include <sources/extension.h>
#include <objects/string/string_attr.h>
#include <objects/string.h>
#include <string_util.h>
#include <cairo/cairo.h>

typedef struct
{
    PangoAttribute attr;
    string_attributes *sa;
    unsigned char type;
} StringAttrColor;

static PangoAttribute *string_attr_color_copy(const PangoAttribute *pa)
{
    StringAttrColor *attr=zmalloc(sizeof(StringAttrColor));
    StringAttrColor *src=(StringAttrColor*)pa;
    memcpy(attr,src,sizeof(StringAttrColor));
    return(&attr->attr);
}

static void string_attr_color_destroy(PangoAttribute *pa1)
{
    sfree((void**)&pa1);
}

static int string_attr_color_compare(const PangoAttribute *pa1,const PangoAttribute *pa2)
{
    unused_parameter(pa1);
    unused_parameter(pa2);
    return(0);
}

PangoAttribute *string_attr_color(unsigned char cl_type,string_attributes *sa,size_t start,size_t end)
{
    static PangoAttrClass cls=
    {
        .type=STRING_ATTR_COLOR,
        .copy=string_attr_color_copy,
        .destroy=string_attr_color_destroy,
        .equal=string_attr_color_compare
    };

    StringAttrColor *attr=zmalloc(sizeof(StringAttrColor));
    attr->attr.klass=&cls;
    attr->attr.start_index=start;
    attr->attr.end_index=end;
    attr->sa=sa;
    attr->type=cl_type;
    return(&attr->attr);
}

static int string_attr_color_draw(string_object *so,string_attributes *sa,PangoRectangle *pr,cairo_t *cr,unsigned char shadow)
{
    if(sa->font_shadow)
    {
        cairo_save(cr);
        cairo_translate(cr, sa->shadow_x+PADDING_W / 2.0, PADDING_H / 2.0 +sa->shadow_y);
        cairo_rectangle(cr,pr->x,pr->y,pr->width,pr->height);
        cairo_clip(cr);
        cairo_set_color(cr,sa->shadow_color);
        pango_cairo_show_layout(cr,so->layout);
        cairo_reset_clip(cr);
        cairo_restore(cr);
    }

    if(shadow)
    {
        return(0);
    }

    cairo_save(cr);
    cairo_rectangle(cr,pr->x,pr->y,pr->width,pr->height);
    cairo_clip(cr);

    if(sa->gradient_len<2)
    {
        cairo_set_color(cr, sa->font_color);
        pango_cairo_show_layout(cr,so->layout);
    }
    else
    {
        cairo_pattern_t* background = cairo_pattern_create_linear((double)pr->x,
                                      (double)pr->y+(double)pr->height/2.0,
                                      (double)(pr->x+pr->width),
                                      (double)pr->y+(double)pr->height/2.0);
        cairo_matrix_t m = { 0 };

        cairo_matrix_init_translate(&m,(double)pr->x, (double)pr->y);

        cairo_matrix_translate(&m,(double)pr->width/2.0, (double)pr->height/2.0);
        cairo_matrix_rotate(&m, DEG2RAD(sa->gradient_ang));
        cairo_matrix_translate(&m, -((double)pr->width)/2.0, -((double)pr->height)/2.0);
        cairo_matrix_translate(&m,-(double)pr->x, -(double)pr->y);

        cairo_pattern_set_matrix(background, &m);
        cairo_set_source(cr,background);

        for(size_t i=0; i<sa->gradient_len; i++)
        {
            unsigned char *color = (unsigned char*)&sa->gradient[i];
            double red = (double)color[2] / 255.0;
            double green = (double)color[1] / 255.0;
            double blue = (double)color[0] / 255.0;
            double alpha = (double)color[3] / 255.0;
            double pos=(double)(i==0?i:i+1)/(double)sa->gradient_len;
            cairo_pattern_add_color_stop_rgba(background, (double)pos, red, green, blue,alpha);
        }
        pango_cairo_show_layout(cr,so->layout);
        cairo_pattern_destroy(background);
    }

    if(sa->font_outline)
    {
        cairo_reset_clip(cr);

        if(so->was_outlined==0)
        {
            cairo_rectangle(cr,pr->x-1,pr->y,pr->width-1,pr->height);
        }
        else
        {
            cairo_rectangle(cr,pr->x-1,pr->y,pr->width+2,pr->height);
        }

        so->was_outlined=1;
        cairo_clip(cr);
        cairo_set_line_width(cr, 1.0);
        cairo_set_color(cr,sa->outline_color);
        pango_cairo_layout_path(cr, so->layout);
        cairo_stroke(cr);
    }
    else
    {
        so->was_outlined=0;
    }

    cairo_restore(cr);
    return(0);
}


int string_attr_color_handler(PangoAttribute *pa,void *pv)
{
    void **pm=(void**)pv;
    if(pa->klass->type!=STRING_ATTR_COLOR||((StringAttrColor*)pa)->sa==NULL)
    {
        return(0);
    }
    string_object *so=pm[0];
    cairo_t *cr=pm[1];
    string_attributes *sa=((StringAttrColor*)pa)->sa;

    long px=0;
    long py=0;
    long pw=0;
    long ph=0;
    long oy=0;
    long ox=0;
    unsigned char yh=0;

    if(sa->font_shadow==0&&pm[2])
    {
        return(0);
    }

    if(pa->start_index==0&&so->bind_string[0]!=' '&&sa->font_outline)
    {
        cairo_translate(cr,1,0);
    }


    size_t start=pa->start_index;

    for(size_t i=pa->start_index; i<pa->end_index; i++)
    {
        PangoRectangle pr= {0};

        if(((i+1==pa->end_index)||(i==pa->start_index))&&so->bind_string[i]==' ')
        {
            start++;
            continue;
        }

        pango_layout_index_to_pos(so->layout,i,&pr);

        yh=0;
        pr.x>>=10;
        pr.y>>=10;
        pr.width>>=10;
        pr.height>>=10;

        if(i==start)
        {
            px=pr.x;
            py=pr.y;
            oy=pr.y;
        }

        else
        {
            if(ox==pr.x)
            {
                continue;
            }

            ox=pr.x;

            if(oy!=pr.y)
            {
                PangoRectangle prv=
                {
                    .x=px,
                    .y=py,
                    .width=pw,
                    .height=ph
                };
                string_attr_color_draw(so,sa,&prv,cr,(unsigned char)(size_t)(pm[2]));
                oy=pr.y;
                yh=1;
                pw=0;
                ph=0;
                px=pr.x;
                py=pr.y;
            }
        }
        pw=max((pr.width+pr.x)-px,pw);
        ph=max(pr.height,ph);
    }

    if(yh==0)
    {
        PangoRectangle pr=
        {
            .x=px,
            .y=py,
            .width=pw,
            .height=ph
        };

        string_attr_color_draw(so,sa,&pr,cr,(unsigned char)(size_t)(pm[2]));
    }
    return(0);
}
