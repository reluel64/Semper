/*
*Arc object
*Part of Project 'Semper'
*Written by Alexandru-Daniel Mărgărit
*/

#include <surface.h>
#include <objects/object.h>
#include <objects/arc.h>
#include <bind.h>
#include <mem.h>
#include <string_util.h>
#include <parameter.h>
#include <strings.h>

void arc_init(object* o)
{
    if(o && o->pv == NULL)
    {
        o->pv = zmalloc(sizeof(arc_object));
    }
}
void arc_reset(object* o)
{
    arc_object* ao = o->pv;

    ao->start_angle = parameter_double(o, "StartAngle", 0, XPANDER_OBJECT);
    ao->step_angle = parameter_double(o, "StepAngle", 1.0, XPANDER_OBJECT);
    ao->width = parameter_long_long(o, "LineWidth", 1, XPANDER_OBJECT);

    ao->radius = parameter_long_long(o, "Radius", 0, XPANDER_OBJECT);
    ao->color = parameter_color(o, "ArcColor", 0xffffffff, XPANDER_OBJECT);
    ao->fill = parameter_bool(o, "Fill", 0, XPANDER_OBJECT);
    ao->rounded = parameter_bool(o, "Rounded", 0, XPANDER_OBJECT);

    ao->step_angle = ao->step_angle == 0.0f ? 1.0f : ao->step_angle;
    ao->width = ao->width ? ao->width : 1;
    o->w = o->w ? o->w : ao->radius * 2;
    o->h = o->h ? o->h : ao->radius * 2;
}

int arc_update(object* o)
{
    arc_object* ao = o->pv;
    bind_numeric bn = { 0 };

    bind_update_numeric(o, &bn);
    ao->percents = bind_percentual_value(bn.val, bn.min, bn.max);
    return (1);
}

int arc_render(object* o, cairo_t* cr)
{
    arc_object* ao = o->pv;
    cairo_set_line_cap(cr, ao->rounded ? CAIRO_LINE_CAP_ROUND : CAIRO_LINE_CAP_SQUARE);
    cairo_set_color(cr,ao->color);
    cairo_set_line_width(cr, ao->width);

    if(ao->fill)
    {
        cairo_move_to(cr, o->w / 2, o->h / 2);
    }

    cairo_arc(cr, o->w / 2, o->h / 2, ao->radius, ao->start_angle,  DEG2RAD(ao->percents*360.0));

    if(ao->fill)
    {
        cairo_line_to(cr, o->w / 2, o->h / 2);
        cairo_fill(cr);
    }
    else
    {
        cairo_stroke(cr);
    }

    return (0);
}

void arc_destroy(object* o)
{
    sfree((void**)&o->pv);
}
