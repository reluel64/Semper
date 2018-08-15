/*
String object attributes
Part of Project "Semper"
Wrriten by Alexandru-Daniel Mărgărit
*/
#define PCRE_STATIC
#include <objects/string/string_attr.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>
#include <pango/pangoft2.h>
#include <fontconfig/fontconfig.h>
#include <enumerator.h>
#include <math_parser.h>
#include <pcre.h>
#include <string_util.h>
#include <xpander.h>
#include <parameter.h>
#include <mem.h>

struct _PangoAttrList
{
    guint ref_count;
    GSList *attributes;
    GSList *attributes_tail;
};
typedef struct
{
    size_t op;
    size_t quotes;
    unsigned char quote_type;
} string_parser_status;

typedef struct
{
    size_t start;
    size_t end;
} string_attr_filter_range;


typedef enum
{
    type_invalid,
    type_strikethrough,
    type_style,
    type_shadow,
    type_underline,
    type_font_name,
    type_font_color,
    type_outline,
    type_size,
    type_weight,
    type_pattern,
    type_stretch,
    type_rise,
    type_case,
    type_spacing,
    type_gradient
} string_format_type;


extern PangoAttribute *string_attr_font(unsigned char *font_name, size_t start, size_t end);
extern PangoAttribute *string_attr_size(double size, size_t start, size_t end);
extern PangoAttribute *string_attr_strike(unsigned char strike, size_t start, size_t end);
extern PangoAttribute *string_attr_style(PangoStyle style, size_t start, size_t end);
extern PangoAttribute *string_attr_underline(PangoUnderline underline, size_t start, size_t end);
extern PangoAttribute *string_attr_weight(PangoWeight weight, size_t start, size_t end);
extern PangoAttribute *string_attr_stretch(PangoStretch stretch, size_t start, size_t end);
extern PangoAttribute *string_attr_rise(int rise, size_t start, size_t end);
extern PangoAttribute *string_attr_case(unsigned char case_type, size_t start, size_t end);
extern PangoAttribute *string_attr_spacing(int spacing, size_t start, size_t end);
extern PangoAttribute *string_attr_color(unsigned char cl_type, string_attributes *sa, size_t start, size_t end);
extern int             string_attr_case_handler(PangoAttribute *pa, void *pv);

static int string_parse_filter(string_tokenizer_status *pi, void* pv)
{
    string_parser_status* sps = pv;

    if(pi->reset)
    {
        memset(sps, 0, sizeof(string_parser_status));
        return(0);
    }

    if(sps->quote_type == 0 && (pi->buf[pi->pos] == '"' || pi->buf[pi->pos] == '\''))
        sps->quote_type = pi->buf[pi->pos];

    if(pi->buf[pi->pos] == sps->quote_type)
        sps->quotes++;

    if(sps->quotes % 2)
        return(0);
    else
        sps->quote_type = 0;

    if(pi->buf[pi->pos] == '(')
        if(++sps->op == 1)
            return(1);



    if(pi->buf[pi->pos] == ';')
        return (sps->op == 0);

    if(sps->op == 1 && pi->buf[pi->pos] == ',')
        return (1);

    if(sps->op && pi->buf[pi->pos] == ')')
           if(--sps->op == 0)
               return(0);

    return (0);
}

static int string_validate_attributes(string_tokenizer_info *sti)
{
    int param = 0;

    for(size_t i = 0; i < sti->oveclen / 2; i++)
    {
        size_t start = sti->ovecoff[2 * i];
        size_t end   = sti->ovecoff[2 * i + 1];

        if(start == end)
        {
            continue;
        }

        /*Clean spaces*/
        if(string_strip_space_offsets(sti->buffer, &start, &end) == 0)
        {
            if(sti->buffer[start] == '(')
            {
                start++;
                param = 1;
            }
            else if(sti->buffer[start] == ';')
            {
                param = 0;
                start++;
            }
        }

        if(string_strip_space_offsets(sti->buffer, &start, &end) == 0)
        {
            if(sti->buffer[start] == '"' || sti->buffer[start] == '\'')
                start++;

            if(sti->buffer[end - 1] == '"' || sti->buffer[end - 1] == '\'')
                end--;
        }

        if(param == 0 && strncasecmp("Pattern", sti->buffer + start, end - start) == 0)
            return(1);
    }

    return(0);
}

static string_attributes *string_attr_alloc_index(list_entry *head, size_t index)
{
    string_attributes *sa = NULL;
    string_attributes *p = NULL;
    string_attributes *n = NULL;
    list_enum_part(sa, head, current)
    {
        if(sa->index < index)
            p = sa;

        if(sa->index > index)
            n = sa;

        if(n && p)
            break;

        if(sa->index == index)
            return (NULL);
    }

    sa = zmalloc(sizeof(string_attributes));
    sa->index = index;
    list_entry_init(&sa->current);

    if(n == NULL)
    {
        linked_list_add_last(&sa->current, head);
    }
    else if(p == NULL)
    {
        linked_list_add(&sa->current, head);
    }
    else
    {
        _linked_list_add(&p->current, &sa->current, &n->current);
    }

    return (sa);
}


static string_format_type string_attr_fill_def(unsigned char *str, string_attributes *sa)
{
    if(!strncasecmp(str, "strikethrough", 13))
    {
        sa->strikethrough = 1;
        return(type_strikethrough);
    }
    else if(!strncasecmp(str, "style", 5))
    {
        sa->style = PANGO_STYLE_NORMAL;
        return(type_style);
    }
    else if(!strncasecmp(str, "shadow", 6))
    {
        sa->font_shadow = 1;
        sa->shadow_color = 0xff000000;
        sa->shadow_x = 1.0;
        sa->shadow_y = -1.0;
        return(type_shadow);
    }
    else if(!strncasecmp(str, "underline", 9))
    {
        sa->underline = 1;
        sa->underline_style = PANGO_UNDERLINE_SINGLE;
        return(type_underline);
    }
    else if(!strncasecmp(str, "FontName", 8))
    {
        sfree((void**)&sa->font_name);
        return(type_font_name);
    }
    else if(!strncasecmp(str, "Color", 5))
    {
        sa->font_color = 0xffffffff;
        return(type_font_color);
    }
    else if(!strncasecmp(str, "Size", 4))
    {
        sa->font_size = 12.0;
        return(type_size);
    }
    else if(!strncasecmp(str, "Weight", 6))
    {
        sa->weight = PANGO_WEIGHT_NORMAL;
        return(type_weight);
    }
    else if(!strncasecmp(str, "Pattern", 7))
    {
        sfree((void**)&sa->pattern);
        return(type_pattern);
    }
    else if(!strncasecmp(str, "Stretch", 7))
    {
        sa->stretch = PANGO_STRETCH_NORMAL;
        return(type_stretch);
    }
    else if(!strncasecmp(str, "Rise", 4))
    {
        sa->rise = 0;
        return(type_rise);
    }
    else if(!strncasecmp(str, "Case", 4))
    {
        sa->str_case = STRING_CASE_NORMAL;
        return(type_case);
    }
    else if(!strncasecmp(str, "Spacing", 7))
    {
        sa->has_spacing = 1;
        return(type_spacing);
    }
    else if(!strncasecmp(str, "Gradient", 8))
    {
        sfree((void**)&sa->gradient);
        sa->gradient_len = 0;
        sa->gradient_ang = 0.0;
        return(type_gradient);
    }
    else if(!strncasecmp(str, "Outline", 7))
    {
        sa->font_outline = 1;
        sa->outline_color = 0xff000000;
        return(type_outline);
    }

    return(type_invalid);
}



static int string_attr_fill_user(object *o, string_format_type *fmt_type, string_attributes *sa, unsigned char *str, size_t start, size_t end, unsigned char param)
{
    size_t len = end - start;
    unsigned char pm[256] = {0};
    strncpy(pm, str + start, min(end - start, 255));

    switch(*fmt_type)
    {
        case type_invalid:
            break;

        case type_strikethrough:
            sa->strikethrough_color = string_to_color(pm);
            *fmt_type = type_invalid;
            break;

        case type_style:
        {
            if(!strcasecmp(pm, "Italic"))
                sa->style = PANGO_STYLE_ITALIC;
            else if(!strcasecmp(pm, "Oblique"))
                sa->style = PANGO_STYLE_OBLIQUE;

            *fmt_type = type_invalid;
            break;
        }

        case type_shadow:
        {
            if(param == 1)
            {
                sa->shadow_color = string_to_color(pm);
            }
            else if(param >= 2 && param <= 3)
            {
                double sz = 0.0;

                if(math_parser(pm, &sz, NULL, NULL) == 0)
                {
                    if(param == 2)
                        sa->shadow_x = (float)sz;
                    else
                        sa->shadow_y = (float)sz;
                }
            }
            else
            {
                *fmt_type = type_invalid;
            }
        }
        break;

        case type_underline:
        {
            if(param == 1)
            {
                if(!strncasecmp("Error", pm, 5))
                    sa->underline_style = PANGO_UNDERLINE_ERROR;
                else if(!strncasecmp("Low", pm, 3))
                    sa->underline_style = PANGO_UNDERLINE_LOW;
                else if(!strncasecmp("Double", pm, 6))
                    sa->underline_style = PANGO_UNDERLINE_DOUBLE;
                else if(!strncasecmp("Single", pm, 6))
                    sa->underline_style = PANGO_UNDERLINE_SINGLE;
            }
            else if(param == 2)
            {
                sa->underline_color = string_to_color(pm);
            }
            else
            {
                *fmt_type = type_invalid;
            }
        }
        break;

        case type_font_name:
            sfree((void**)&sa->font_name);

            if(len)
            {
                sa->font_name = zmalloc(len + 1);
                strncpy(sa->font_name, str, len);
            }

            *fmt_type = type_invalid;
            break;

        case type_font_color:
            sa->font_color = string_to_color(pm);
            *fmt_type = type_invalid;
            break;

        case type_outline:
            sa->outline_color = string_to_color(pm);
            *fmt_type = type_invalid;
            break;

        case type_size:
        {
            double sz = 0.0;

            if(math_parser(pm, &sz, NULL, NULL) == 0)
            {
                sa->font_size = sz;
            }

            *fmt_type = type_invalid;
            break;
        }

        case type_weight:
        {
            double sz = 0.0;

            if(math_parser(pm, &sz, NULL, NULL) == 0)
            {
                sa->weight = (unsigned int)sz;
            }

            *fmt_type = type_invalid;
            break;
        }

        case type_pattern:
        {
            sfree((void**)&sa->pattern);
            sa->pattern = zmalloc(len + 1);
            strncpy(sa->pattern, str + start, len);
            *fmt_type = type_invalid;
            break;
        }

        case type_stretch:
        {
            double sz = 0.0;

            if(math_parser(pm, &sz, NULL, NULL) == 0)
            {
                sa->stretch = (sz > 0.0 && sz < 9.0 ? (unsigned char)sz : PANGO_STRETCH_NORMAL);
            }

            *fmt_type = type_invalid;
            break;
        }

        case type_rise:
        {
            double sz = 0.0;

            if(math_parser(pm, &sz, NULL, NULL) == 0)
            {
                sa->rise = (int)sz;
            }

            *fmt_type = type_invalid;
            break;
        }

        case type_case:
        {
            if(!strncasecmp("Lower", pm, 5))
                sa->str_case = STRING_CASE_LOWER;
            else if(!strncasecmp("Upper", pm, 5))
                sa->str_case = STRING_CASE_UPPER;

            *fmt_type = type_invalid;
            break;
        }

        case type_spacing:
        {
            double sz = 0.0;

            if(math_parser(pm, &sz, NULL, NULL) == 0)
            {
                sa->spacing = (int)sz;
            }

            *fmt_type = type_invalid;
            break;
        }

        case type_gradient:
        {
            if(param == 1)
            {
                double sz = 0.0;

                if(math_parser(pm, &sz, NULL, NULL) == 0)
                {
                    sa->gradient_ang = (int)sz;
                }

                *fmt_type = type_invalid;
            }
            else
            {
                unsigned int *temp = realloc(sa->gradient, sizeof(unsigned int*) * (sa->gradient_len + 1));

                if(temp)
                {
                    sa->gradient = temp;
                    sa->gradient[sa->gradient_len++] = string_to_color(pm);
                }

                *fmt_type = type_invalid;
            }

            break;
        }
    }

    return(0);
}

static int string_fill_text_format(object *o)
{
    void *es = NULL;
    unsigned char *eval = enumerator_first_value(o, ENUMERATOR_OBJECT, &es);
    string_object *so = o->pv;

    do
    {
        string_format_type fmt_type = 0;
        string_attributes *sa = NULL;
        string_parser_status spa = {0};
        unsigned char param = 0;
        size_t index = 0;

        if(eval == NULL)
            break;

        if(strncasecmp("TextFormat", eval, 9))
            continue;

        unsigned char *val = parameter_string(o, eval, NULL, XPANDER_OBJECT);

        if(val == NULL)
            continue;

        string_tokenizer_info    sti =
        {
            .buffer                  = val, //store the string address here
            .filter_data             = &spa,
            .string_tokenizer_filter = string_parse_filter,
            .ovecoff                 = NULL,
            .oveclen                 = 0
        };


        string_tokenizer(&sti);

        if(string_validate_attributes(&sti) == 0)
        {
            sfree((void**)&val);
            sfree((void**)&sti.ovecoff);
            continue;
        }

        sscanf(eval + 10, "%llu", &index);

        if((sa = string_attr_alloc_index(&so->attr, index + 1)) == NULL)
        {
            sfree((void**)&val);
            sfree((void**)&sti.ovecoff);
            continue;
        }

        for(size_t i = 0; i < sti.oveclen / 2; i++)
        {

            size_t start = sti.ovecoff[2 * i];
            size_t end   = sti.ovecoff[2 * i + 1];

            if(start == end)
            {
                continue;
            }

            /*Clean spaces*/

            if(string_strip_space_offsets(sti.buffer, &start, &end) == 0)
            {
                if(sti.buffer[start] == '(')
                {
                    start++;
                    param = 1;
                }
                else if(sti.buffer[start] == ',')
                {
                    start++;
                    param++;
                }
                else if(sti.buffer[start] == ';')
                {
                    fmt_type = 0;
                    param = 0;
                    start++;
                }

                if(sti.buffer[end - 1] == ')')
                {
                    end--;
                }
            }

            if(string_strip_space_offsets(sti.buffer, &start, &end) == 0)
            {
                if(sti.buffer[start] == '"' || sti.buffer[start] == '\'')
                    start++;

                if(sti.buffer[end - 1] == '"' || sti.buffer[end - 1] == '\'')
                    end--;
            }

            if(param == 0)
            {
                fmt_type = string_attr_fill_def(sti.buffer + start, sa);
            }
            else if(start < end)
            {
                string_attr_fill_user(o, &fmt_type, sa, sti.buffer, start, end, param);
            }
        }

        sfree((void**)&val);
        sfree((void**)&sti.ovecoff);
        sti.buffer = NULL;
    }
    while((eval = enumerator_next_value(es)) != NULL);

    enumerator_finish(&es);
    return(0);
}

int string_attr_update(string_object *so)
{
    pango_layout_set_attributes(so->layout, NULL);

    if(so->attr_list)
    {
        pango_attr_list_unref(so->attr_list);
    }

    so->attr_list = pango_attr_list_new();

    if(so->attr_list == NULL)
        return(-1);

    string_attributes *sa = NULL;
    size_t attr_no = 0;
    size_t len = so->bind_string_len;
    const char *errptr = NULL;
    unsigned int ovec[300];
    int erroff = 0;

    list_enum_part(sa, &so->attr, current)
    {
        pcre *ctx = NULL;
        int rc = 0;
        memset(ovec, 0, sizeof(ovec));

        if(attr_no && sa->pattern)
        {
            ctx = pcre_compile(sa->pattern, 0, &errptr, &erroff, NULL);
        }

        if(ctx == NULL && attr_no)
        {
            continue;
        }
        else if(attr_no)
        {
            rc = pcre_exec(ctx, NULL, so->bind_string, len, 0, PCRE_NOTEMPTY, (int*)ovec, 300);
        }

        if(attr_no && rc < 1)
        {
            pcre_free(ctx);
            continue;
        }

        if(attr_no == 0)
        {
            ovec[3] = -1;
            rc = 2;
        }

        for(size_t i = rc > 1 ? 1 : 0; i < rc; i++)
        {
            PangoAttribute *pa = NULL;
            size_t start = ovec[2 * i];
            size_t end = ovec[2 * i + 1];
            unsigned char color_type = 0;

            if(sa->font_name)
            {
                pa = string_attr_font(sa->font_name, start, end);
                pango_attr_list_change(so->attr_list, pa);
            }

            if(sa->strikethrough)
            {
                pa = string_attr_strike(sa->strikethrough, start, end);
                pango_attr_list_change(so->attr_list, pa);
            }

            if(sa->underline)
            {
                pa = string_attr_underline(sa->underline_style, start, end);
                pango_attr_list_change(so->attr_list, pa);
            }

            if(sa->stretch)
            {
                pa = string_attr_stretch(sa->stretch, start, end);
                pango_attr_list_change(so->attr_list, pa);
            }

            if(sa->font_size != 0.0)
            {
                pa = string_attr_size(sa->font_size, start, end);
                pango_attr_list_change(so->attr_list, pa);
            }

            if(sa->weight)
            {
                pa = string_attr_weight(sa->weight, start, end);
                pango_attr_list_change(so->attr_list, pa);
            }

            if(sa->style)
            {
                pa = string_attr_style(sa->style, start, end);
                pango_attr_list_change(so->attr_list, pa);
            }

            if(sa->rise)
            {
                pa = string_attr_rise(sa->rise, start, end);
                pango_attr_list_change(so->attr_list, pa);
            }

            if(sa->str_case)
            {
                pa = string_attr_case(sa->str_case, start, end);
                pango_attr_list_change(so->attr_list, pa);
            }

            if(sa->has_spacing)
            {
                pa = string_attr_spacing(sa->spacing, start, end);
                pango_attr_list_change(so->attr_list, pa);
            }

            if(sa->font_outline || sa->font_shadow || sa->gradient_len >= 2 || sa->font_color)
            {
                pa = string_attr_color(color_type, sa, start, end);
                pango_attr_list_change(so->attr_list, pa);
            }
        }

        if(ctx == NULL)
        {
            pcre_free(ctx);
        }

        attr_no++;
    }
    pango_attr_list_filter(so->attr_list, string_attr_case_handler, so);
    pango_layout_set_attributes(so->layout, so->attr_list);
    return(0);
}

void string_attr_init(object *o)
{
    string_object* so = o->pv;
    unsigned char *temp = NULL;
    string_attributes *sa = zmalloc(sizeof(string_attributes));
    sa->index = 0;
    list_entry_init(&so->attr);
    list_entry_init(&sa->current);
    linked_list_add(&sa->current, &so->attr);

    //collect parameters for the base string
    sa->font_name = parameter_string(o, "FontName", "Arial", XPANDER_OBJECT);
    sa->font_size = parameter_double(o, "FontSize", 12.0, XPANDER_OBJECT);
    sa->font_color = parameter_color(o, "FontColor", 0xffffffff, XPANDER_OBJECT);
    sa->weight = parameter_size_t(o, "FontWeight", PANGO_WEIGHT_NORMAL, XPANDER_OBJECT);
    sa->style = parameter_bool(o, "FontItalic", 0, XPANDER_OBJECT) != 0 ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL;
    sa->weight = sa->weight ? sa->weight : PANGO_WEIGHT_NORMAL;

    temp = parameter_string(o, "StringCase", NULL, XPANDER_OBJECT);

    if(temp)
    {
        strupr(temp);

        if(strstr(temp, "UPPER"))
        {
            sa->str_case = STRING_CASE_UPPER;
        }
        else if(strstr(temp, "LOWER"))
        {
            sa->str_case = STRING_CASE_LOWER;
        }

        sfree((void**)&temp);
    }

    if((sa->font_shadow = parameter_bool(o, "FontShadow", 0, XPANDER_OBJECT)) != 0)
    {
        sa->shadow_color = parameter_color(o, "FontShadowColor", sa->font_color & 0xff000000, XPANDER_OBJECT);
        sa->shadow_x = 1.0;
        sa->shadow_y = -1.0;
    }

    if((sa->font_outline = parameter_bool(o, "FontOutline", 0, XPANDER_OBJECT)) != 0)
    {
        sa->outline_color = parameter_color(o, "FontOutlineColor", sa->font_color & 0xff000000, XPANDER_OBJECT);
    }

    string_fill_text_format(o);

}

void string_attr_destroy(string_object *so)
{
    string_attributes *sa = NULL;
    string_attributes *tsa = NULL;
    pango_layout_set_attributes(so->layout, NULL);
    pango_attr_list_unref(so->attr_list);
    so->attr_list = NULL;
    list_enum_part_safe(sa, tsa, &so->attr, current)
    {
        sfree((void**)&sa->font_name);
        sfree((void**)&sa->pattern);
        sfree((void**)&sa);
    }
}
