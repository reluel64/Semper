/* Line Object
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 * */
#include <objects/line.h>
#include <bind.h>
#include <semper.h>
#include <mem.h>
#include <string_util.h>
#include <parameter.h>
#include <xpander.h>
#include <enumerator.h>

#warning "Code needs testing"

static void line_data_destroy(line_data** ld);

void line_destroy(object* o)
{
    line_object* lo = o->pv;
    line_data* ld = NULL;
    line_data* tld = NULL;
    list_enum_part_safe(ld, tld, &lo->lines, current)
    {
        line_data_destroy(&ld);
    }
    sfree((void**)&o->pv);
}

void line_init(object* o)
{
    if(o && o->pv == NULL)
    {
        o->pv = zmalloc(sizeof(line_object));
        line_object* lo = o->pv;
        list_entry_init(&lo->lines);
    }
}

static void line_data_destroy(line_data** ld)
{
    line_data* tld = *ld;
    line_value* lv = NULL;
    line_value* tlv = NULL;
    linked_list_remove(&tld->current);

    list_enum_part_safe(lv, tlv, &tld->values, current)
    {
        linked_list_remove(&lv->current);
        sfree((void**)&lv);
    }
    sfree((void**)ld);
}

static line_data* line_data_alloc(line_object* lo, size_t index)
{
    line_data* n = NULL;
    line_data* p = NULL;
    line_data* ld = NULL;

    list_enum_part(ld, &lo->lines, current)
    {
        if(ld->index < index)
            p = ld;

        else if(ld->index > index)
            n = ld;

        else
            return (ld);

        if(n && p)
            break;
    }

    ld = zmalloc(sizeof(line_data));
    list_entry_init(&ld->current);
    list_entry_init(&ld->values);
    ld->index = index;

    if(n == NULL)
    {
        linked_list_add_last(&ld->current, &lo->lines);
    }
    else if(p == NULL)
    {
        linked_list_add(&ld->current, &lo->lines);
    }
    else
    {
        _linked_list_add(&p->current, &ld->current, &n->current);
    }

    return (ld);
}

static int line_cleanup_invalid_entries(object* o)
{
    line_object* lo = o->pv;

    line_data* ld = NULL;
    line_data* tld = NULL;

    list_enum_part_safe(ld, tld, &lo->lines, current)
    {
        unsigned char buf[256] = { 0 };

        snprintf(buf, sizeof(buf), (unsigned char*)(ld->index > 0 ? "LineColor%llu" : "LineColor"), ld->index);
        unsigned char* p = parameter_string(o, buf, NULL, XPANDER_OBJECT);

        if(p == NULL)
        {
            line_data_destroy(&ld);
        }

        sfree((void**)&p);
    }
    return (0);
}

void line_reset(object* o)
{
    line_cleanup_invalid_entries(o);

    line_object* lo = o->pv;

    lo->flip = parameter_bool(o, "Flip", 0, XPANDER_OBJECT);
    lo->vertical = parameter_bool(o, "Vertical", 0, XPANDER_OBJECT);
    lo->reverse = parameter_bool(o, "ReverseOrigin", 0, XPANDER_OBJECT);

    size_t dim = 0;

    if(lo->vertical)
    {
        dim = o->h;
    }
    else
    {
        dim = o->w;
    }

    void* es = NULL;
    unsigned char* v = enumerator_first_value(o, ENUMERATOR_OBJECT, &es);

    do
    {
        if(strncasecmp(v, "LineColor", 9))
        {
            continue;
        }

        size_t index = 0;

        sscanf(v + 9, "%llu", &index);

        line_data* ld = line_data_alloc(lo, index);
        ld->line_color = parameter_color(o, v, 0xff00ff00 + index, XPANDER_OBJECT);

    }
    while((v = enumerator_next_value(es)));

    enumerator_finish(&es);
    lo->max_pts = dim;


    if(lo->max_pts < lo->v_count)
    {
        line_data* ld = NULL;
        list_enum_part(ld, &lo->lines, current)
        {
            line_value* lv = NULL;
            line_value* tlv = NULL;
            size_t i = lo->v_count - lo->max_pts;
            list_enum_part_backward_safe(lv, tlv, &ld->values, current)
            {
                if(i == 0)
                {
                    break;
                }

                linked_list_remove(&lv->current);
                sfree((void**)&lv);
                i--;
            }
        }
    }

}

int line_update(object* o)
{
    line_object* lo = o->pv;
    line_data* ld = NULL;

    list_enum_part(ld, &lo->lines, current)
    {
        line_value* lv = NULL;
        bind_numeric bn = { 0 };
        bn.index = ld->index;
        bind_update_numeric(o, &bn);

        if(ld->max < bn.max)
        {
            ld->max = bn.max;
        }

        if(ld->min > bn.min)
        {
            ld->min = bn.min;
        }

        if(lo->max_pts > lo->v_count)
        {
            lv = zmalloc(sizeof(line_value));
            list_entry_init(&lv->current);
            lv->value = bn.val;
            list_entry_init(&lv->current);
            linked_list_add(&lv->current, &ld->values);
        }
        else
        {
            lv = element_of(ld->values.prev, lv, current);
            linked_list_remove(&lv->current);
            list_entry_init(&lv->current);
            linked_list_add(&lv->current, &ld->values);
            lv->value = bn.val;
        }
    }

    if(lo->v_count < lo->max_pts)
    {
        lo->v_count++;
    }
    else
    {
        lo->v_count = lo->max_pts;
    }

    return (1);
}

int line_render(object* o, cairo_t* cr)
{
    line_object* lo = o->pv;
    line_data* ld = NULL;
    long height = o->h;
    long width = o->w;
    cairo_set_line_width(cr, 1.0);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);

    list_enum_part(ld, &lo->lines, current)
    {

        cairo_set_color(cr, ld->line_color);

        line_value* lv = NULL;
        size_t i = 0;
        list_enum_part(lv, &ld->values, current)
        {
            double x = 0.0;
            double y = 0.0;
            double p = bind_percentual_value(lv->value, ld->min, ld->max);

            x = width - i;
            y = height - height * p;

            if(lo->reverse)
            {
                x = i;
            }

            if(lo->flip)
            {
                y = height * p;
            }

            if(lo->vertical)
            {
                y = height - i;
                x = width - width * p;

                if(lo->reverse)
                {
                    y = i;
                }

                if(lo->flip)
                {
                    x = width * p;
                }
            }

            i++;
            cairo_line_to(cr, x, y);
        }
        cairo_stroke(cr);
    }
    return (0);
}
