/*
Generic object routines
Part of Project 'Semper'
Written by Alexandru-Daniel Mărgărit
*/

#include <crosswin/crosswin.h>
#include <objects/bar.h>
#include <objects/histogram.h>
#include <objects/image.h>
#include <objects/line.h>
#include <objects/string.h>
#include <objects/arc.h>
#include <objects/spinner.h>
#include <objects/button.h>
#include <objects/string.h>
#include <objects/vector.h>
#include <string_util.h>
#include <mem.h>
#include <bind.h>
#include <xpander.h>
#include <string.h>
#include <skeleton.h>
#include <mouse.h>
#include <parameter.h>
#include <surface_builtin.h>
#include <surface.h>

typedef struct
{
    unsigned char *obj_name;
    void (*object_init_rtn)(object* o);
    void (*object_reset_rtn)(object* o);
    int (*object_update_rtn)(object* o);
    int (*object_render_rtn)(object* o, cairo_t* cr);
    void (*object_destroy_rtn)(object* o);

} object_routine_entry;

static int object_routines_table(object_routine_entry **ore, unsigned char *on)
{
    static object_routine_entry tbl[] =
    {
        /*Object name   Init routine    Reset routine       Update routine      Render routine             Destroy routine*/
        {"Custom",      NULL,           NULL,               NULL,               NULL,                                NULL},
        {"Image",       image_init,     image_reset,        image_update,       image_render,               image_destroy},
        {"String",      string_init,    string_reset,       string_update,      string_render,             string_destroy},
        {"Bar",         bar_init,       bar_reset,          bar_update,         bar_render,                   bar_destroy},
        {"Histogram",   histogram_init, histogram_reset,    histogram_update,   histogram_render,       histogram_destroy},
        {"Line",        line_init,      line_reset,         line_update,        line_render,                 line_destroy},
        {"Arc",         arc_init,       arc_reset,          arc_update,         arc_render,                   arc_destroy},
        {"Spinner",     spinner_init,   spinner_reset,      spinner_update,     spinner_render,           spinner_destroy},
        {"Button",      button_init,    button_reset,       button_update,      button_render,             button_destroy},
        {"Vector",      vector_init,    vector_reset,       NULL,               vector_render,             vector_destroy}
    };

    if(on == NULL || ore == NULL)
    {
        return (0);
    }

    for(size_t i = 0; i < sizeof(tbl) / sizeof(object_routine_entry); i++)
    {
        if(!strcasecmp(tbl[i].obj_name, on))
        {
            *ore = tbl + i;

            if(i == 0)
            {
                diag_error("%s %d Object Type: %s is not implemented", __FUNCTION__, __LINE__, tbl[i].obj_name);
                return (0);
            }

            return (i + 1);
        }
    }

    return (0);
}

static int object_render_background(object* o, cairo_t* cr)
{

    long w = o->w < 0 ? o->auto_w : o->w;
    long h = o->h < 0 ? o->auto_h : o->h;
    unsigned char* color = NULL;
    double red = 0.0;
    double green = 0.0;
    double blue = 0.0;
    double alpha = 0.0;
    cairo_pattern_t* background = NULL;
    color = (unsigned char*) &o->back_color;
    red = (double) color[2] / 255.0;
    green = (double) color[1] / 255.0;
    blue = (double) color[0] / 255.0;
    alpha = (double) color[3] / 255.0;
    background = cairo_pattern_create_linear(w / 2, 0.0, w / 2, h);
    cairo_pattern_add_color_stop_rgba(background, 0, red, green, blue, alpha);

    if(o->back_color_2)
    {
        cairo_matrix_t m = { 0 };
        color = (unsigned char*) &o->back_color_2;
        red = (double) color[2] / 255.0;
        green = (double) color[1] / 255.0;
        blue = (double) color[0] / 255.0;
        alpha = (double) color[3] / 255.0;
        cairo_pattern_add_color_stop_rgba(background, 1, red, green, blue, alpha);
        cairo_matrix_init_translate(&m, w / 2, h / 2);
        cairo_matrix_rotate(&m, DEG2RAD(o->back_color_angle));
        cairo_matrix_translate(&m, -w / 2, -h / 2);
        cairo_pattern_set_matrix(background, &m);
    }

    cairo_rectangle(cr, -o->op.left, -o->op.top, o->op.left + o->op.right + w, o->op.bottom + o->op.top + h);
    cairo_set_source(cr, background);
    cairo_fill(cr);
    cairo_pattern_destroy(background);
    return(0);
}

int object_calculate_coordinates(object* o)
{
    if(o == NULL)
    {
        return (-1);
    }

    surface_data* sd = o->sd;

    long poh = 0;
    long pow = 0;
    long pox = 0;
    long poy = 0;
    object* po = o;
    long nx = o->x;
    long ny = o->y;
    unsigned char reps[260] = { 0 };
    unsigned char* out = NULL;
    /*we are going backwards to obtain the coordinates of the previous object
     * If the object is disabled, we will go backwards until we reach the head of the list or a enabled object*/
    while(1)
    {
        if(po->current.prev != &sd->objects)
        {
            po = element_of(po->current.prev, po, current);

            if(po->enabled)
            {
                break;
            }
        }
        else
        {
            po = NULL;
            break;
        }
    }

    if(po)
    {
        pow = po->w < 0 ? po->auto_w : po->w;
        poh = po->h < 0 ? po->auto_h : po->h;
        pox = po->x;
        poy = po->y;
    }

    snprintf(reps, sizeof(reps), "(r,+%ld);(R,+%ld)", pox, pow + pox);
    out = replace(o->sx, reps, 0);
    nx = (long) compute_formula(out);
    sfree((void**) &out);

    snprintf(reps, sizeof(reps), "(r,+%ld);(R,+%ld)", poy, poh + poy);
    out = replace(o->sy, reps, 0);
    ny = (long) compute_formula(out);
    sfree((void**) &out);

    if(nx != o->x || ny != o->y)
    {
        o->x = nx;
        o->y = ny;
    }

    return (1);
}

static void object_tweak_matrix(object *o, cairo_matrix_t *m)
{
    long w = o->w < 0 ? o->auto_w : o->w;
    long h = o->h < 0 ? o->auto_h : o->h;

    cairo_matrix_init_translate(m, o->x + o->op.left, o->y + o->op.top);

    if(o->angle != 0.0)
    {
        cairo_matrix_translate(m, w / 2, h / 2);
        cairo_matrix_rotate(m, DEG2RAD(o->angle));
        cairo_matrix_translate(m, -w / 2, -h / 2);
    }
}


static unsigned char object_may_hit(object *o, cairo_t *cr, double x, double y, cairo_surface_t *s, long stride)
{

    int ret = 0;
    long w = o->w < 0 ? o->auto_w : o->w;
    long h = o->h < 0 ? o->auto_h : o->h;
    cairo_matrix_t m = {0};
    object_tweak_matrix(o, &m);
    cairo_save(cr);

    cairo_set_matrix(cr, &m);
    cairo_new_path(cr);
    cairo_rectangle(cr, 0.0, 0.0, (double) w, (double) h);
    cairo_clip(cr);
    cairo_set_color(cr, 0xff000000);
    cairo_paint(cr);
    cairo_surface_flush(s);
    unsigned char* imgb = cairo_image_surface_get_data(s);

    size_t pos_in_buf = (size_t) x + (size_t) y * (size_t) stride;

    if(imgb && imgb[pos_in_buf])
    {
        ret = 1;
    }

    cairo_restore(cr);
    return (ret);
}


static void object_render_internal(object* o, cairo_t* cr)
{
    cairo_matrix_t m = { 0 };
    long ow = o->w < 0 ? o->auto_w : o->w;
    long oh = o->h < 0 ? o->auto_h : o->h;
    object_tweak_matrix(o, &m);

    cairo_save(cr);

    cairo_new_path(cr);
    cairo_rectangle(cr, (double) o->x, (double) o->y, (double) ow, (double) oh);
    cairo_clip(cr);
    cairo_set_matrix(cr, &m);

    object_render_background(o, cr);

    if(o->object_render_rtn)
    {
        o->object_render_rtn(o, cr);
    }

    cairo_restore(cr);
}

object* object_by_name(surface_data* sd, unsigned char* on, size_t len)
{
    object* o = NULL;

    if(!on || !sd)
    {
        return (NULL);
    }

    list_enum_part(o, &sd->objects, current)
    {
        unsigned char* con = skeleton_get_section_name(o->os);

        if(con && (len == (size_t) - 1 ? !strcasecmp(con, on) : !strncasecmp(con, on, len)))
        {
            if(o->die == 0)
            {
                return (o);
            }
            else
            {
                return (NULL);
            }
        }
    }
    return (NULL);
}


static int object_tooltip_update(object *o)
{
    static unsigned char *empty_txt = "Parameter(Text,Disabled,1)";
    static unsigned char *empty_title = "Parameter(Title,Disabled,1)";
    unsigned char tmp_cmd[64]={0};
    unsigned char *tmp_cmd_buf=tmp_cmd;
    static int i= 0;
    if(o->ttip == NULL)
    {
        return(-1);
    }

    if(o->ot.title)
    {
        string_bind titsb = {0};
        titsb.s_in = o->ot.title;
        titsb.percentual = o->ot.percentual;
        titsb.scale = o->ot.scale;
        titsb.self_scaling = o->ot.scaling;
        titsb.decimals = o->ot.decimals;
        bind_update_string(o, &titsb);

        if(titsb.s_out)
        {
            unsigned char titfmt[] = {"Parameter(Title,Text,\"%s\")"};
            size_t fbuf = sizeof(titfmt) + string_length(titsb.s_out);
            unsigned char *buf = zmalloc(fbuf + 1);

            if(titsb.s_out[0] != ' ')
            {
                snprintf(buf, fbuf, titfmt, titsb.s_out);
                command(o->ttip, &buf);
            }

            sfree((void**) &buf);
        }
    }
    else
    {
        command(o->ttip, &empty_title);
    }

    if(o->ot.text)
    {
        string_bind txtsb = {0};
        txtsb.s_in = o->ot.text;
        txtsb.percentual = o->ot.percentual;
        txtsb.scale = o->ot.scale;
        txtsb.self_scaling = o->ot.scaling;
        txtsb.decimals = o->ot.decimals;
        bind_update_string(o, &txtsb);

        if(txtsb.s_out)
        {
            unsigned char txtfmt[] = {"Parameter(Text,Text,\"%s\")"};
            size_t fbuf = sizeof(txtfmt) + string_length(txtsb.s_out);
            unsigned char *buf = zmalloc(fbuf + 1);

            if(txtsb.s_out[0] != ' ')
            {
                snprintf(buf, fbuf, txtfmt, txtsb.s_out);
                command(o->ttip, &buf);
            }

            sfree((void**) &buf);
        }
    }
    else
    {
        command(o->ttip, &empty_txt);
    }


    /*The tooltip relies on the parent object to update its contents.
     * Therefore, we need 3 fast update cycles to have everything nice
     * Cycles:
     * 1) Update the Title and the Text with the parameters set above
     * 2) Update the background size
     * 3) Update the reflect the new content*/

    snprintf(tmp_cmd,sizeof(tmp_cmd),"Parameter(Background,BackColor,0x%08x)",o->ot.color);
    command(o->ttip,&tmp_cmd_buf);
    for(unsigned char cycle = 0; cycle < 3; cycle++)
    {
        surface_update(o->ttip);
    }

    return(0);
}

int object_hit_testing(surface_data* sd, mouse_status* ms)
{
    object* o = NULL;
    object *lo = NULL;

    unsigned char found = 0;
    unsigned char mouse_ret = 0;
    long sw = 0;
    long sh = 0;
    crosswin_get_size(sd->sw,&sw,&sh);

    if(ms->x >= 0 && ms->y >= 0 && ms->x < sw && ms->y < sh)
    {

        long stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8, sw);
        cairo_surface_t* cs = cairo_image_surface_create(CAIRO_FORMAT_A8, sw, sh);

        if(cairo_surface_status(cs))
        {
            cairo_surface_destroy(cs);
            return (0);
        }

        cairo_t* ctx = cairo_create(cs);

        if(cairo_status(ctx))
        {
            cairo_destroy(ctx);
            cairo_surface_destroy(cs);
            return (0);
        }

        list_enum_part_backward(o, &sd->objects, current)
        {
            cairo_set_operator(ctx, CAIRO_OPERATOR_SOURCE);

            if(o->enabled == 0 || o->hidden|| !object_may_hit(o, ctx, (double) ms->x, (double) ms->y, cs, stride) )
            {
                continue;
            }

            object_render_internal(o, ctx);
            cairo_surface_flush(cs);

            unsigned char* imgb = cairo_image_surface_get_data(cs);

            size_t pos_in_buf = (size_t) ms->x + (size_t) ms->y * (size_t) stride;

            if(imgb && imgb[pos_in_buf])
            {
                if(o->object_type == 9)    //Button
                {
                    button_mouse(o, ms);
                }
                if(ms->hover==mouse_hover)
                {
                    if(((object*)o)->ttip == NULL)
                    {
                        surface_builtin_init((void*)o, 1);

                    }

                    if(((object*)o)->ttip)
                    {
                        surface_data *ttip_sd = ((object*)o)->ttip;
                        long sdx = 0;
                        long sdy = 0;
                        size_t mon = 0;
                        object_tooltip_update(o);
                        crosswin_get_position(sd->sw,&sdx,&sdy,&mon);

                        crosswin_set_monitor(ttip_sd->sw,mon);
                        crosswin_set_position(ttip_sd->sw,sdx + ms->x + 10, sdy + ms->y + 10);
                        crosswin_set_zorder(ttip_sd->sw, crosswin_topmost);
                        crosswin_set_keep_on_screen(ttip_sd->sw,1);
                        crosswin_set_visible(ttip_sd->sw,1);
                        event_push(sd->cd->eq,(event_handler)surface_fade,(void*)o->ttip,o->ot.timeout,EVENT_PUSH_TIMER);
                    }

                }
                else
                {
                    surface_builtin_destroy(&((object*)o)->ttip);
                }
                mouse_ret = mouse_handle_button(o, MOUSE_OBJECT, ms);

                found = 1;
                break;
            }
        }

        if(found == 0)
        {
            o = NULL;
        }

        cairo_destroy(ctx);
        cairo_surface_destroy(cs);
    }

    /*trigger leave event for other objects*/

    list_enum_part(lo, &sd->objects, current)
    {
        if(lo != o)
        {
            /* dummy mouse status to signal that other objects
             * are not in focus so they should trigger
             * MouseLeaveAction
             */
            mouse_status dms = { 0 };
            dms.state = mouse_button_state_none;
            dms.hover = mouse_unhover;
            surface_builtin_destroy(&((object*)lo)->ttip);
            if(lo->object_type == 9)    //Button
            {
                button_mouse(lo, &dms);
            }

            mouse_handle_button(lo, MOUSE_OBJECT, &dms);
        }
    }

    return (found && mouse_ret);
}

int object_init(section s, surface_data* sd)
{
    if(sd == NULL || s == NULL)
    {
        return (-1);
    }

    key k = skeleton_get_key(s, "Object");
    object* o = NULL;
    unsigned char* type = skeleton_key_value(k);
    object_routine_entry *ore = NULL;
    unsigned char obj_type = object_routines_table(&ore, type);

    if(!obj_type)
    {
        return (-1);
    }

    o = zmalloc(sizeof(object));

    if(o == NULL)
    {
        return (-1);
    }

    o->object_type = obj_type;
    o->os = s;
    o->sd = sd;

    /*For the moment we will initialize the function pointers here but this may change in the future*/
    o->object_init_rtn    = ore->object_init_rtn;
    o->object_reset_rtn   = ore->object_reset_rtn;
    o->object_update_rtn  = ore->object_update_rtn;
    o->object_render_rtn  = ore->object_render_rtn;
    o->object_destroy_rtn = ore->object_destroy_rtn;

    list_entry_init(&o->current);
    list_entry_init(&o->bindings);

    linked_list_add_last(&o->current, &sd->objects);

    if(o->object_init_rtn)
    {
        o->object_init_rtn(o);
    }

    return (0);
}

void object_reset(object* o)
{
    surface_data* sd = o->sd;
    bind_reset(o);    /*Handle any change that may occur with the binded sources*/
    mouse_set_actions(o, MOUSE_OBJECT);
    sfree((void**) &o->update_act);
    sfree((void**) &o->team);
    sfree((void**) &o->sx);
    sfree((void**) &o->sy);

    sfree((void**) &o->ot.title);
    sfree((void**) &o->ot.text);


    o->sx = parameter_string(o, "X", NULL, XPANDER_OBJECT);
    o->sy = parameter_string(o, "Y", NULL, XPANDER_OBJECT);

    o->w = parameter_long_long(o, "W", -1, XPANDER_OBJECT);
    o->h = parameter_long_long(o, "H", -1, XPANDER_OBJECT);

    o->vol_var = parameter_bool(o, "Volatile", 0, XPANDER_OBJECT);
    o->hidden = parameter_bool(o, "Hidden", 0, XPANDER_OBJECT);
    o->back_color = parameter_color(o, "BackColor", 0, XPANDER_OBJECT);
    o->back_color_2 = parameter_color(o, "BackColor2", 0, XPANDER_OBJECT);
    o->enabled = !parameter_bool(o, "Disabled", 0, XPANDER_OBJECT);
    o->divider = parameter_size_t (o, "Divider", sd->def_divider, XPANDER_OBJECT);
    o->angle = parameter_double(o, "Angle", 0.0, XPANDER_OBJECT);
    o->back_color_angle = parameter_double(o, "BackColorAngle", 0.0, XPANDER_OBJECT);
    o->update_act = parameter_string(o, "UpdateAction", NULL, XPANDER_OBJECT);
    o->team = parameter_string(o, "Team", NULL, XPANDER_OBJECT);
    parameter_object_padding(o, "Padding", &o->op, XPANDER_OBJECT);

    o->ot.text = parameter_string(o, "TooltipText", NULL, XPANDER_OBJECT);
    o->ot.title = parameter_string(o, "TooltipTitle", NULL, XPANDER_OBJECT);
    o->ot.percentual = parameter_bool(o, "ToolTipPercentual", 0, XPANDER_OBJECT);
    o->ot.scale = parameter_double(o, "ToolTipScale", 0, XPANDER_OBJECT);
    o->ot.scaling = parameter_self_scaling(o, "ToolTipSelfScaling", 0, XPANDER_OBJECT);
    o->ot.decimals = parameter_byte(o, "ToolTipDecimals", 0, XPANDER_OBJECT);
    o->ot.color = parameter_color(o, "TooltipColor", 0xff000000, XPANDER_OBJECT);
    o->ot.timeout = parameter_size_t (o, "TooltipTimeout", 500, XPANDER_OBJECT);
    /*Do the private reset of the object */

    if(o->ot.timeout == 0)
    {
        o->ot.timeout = 500;
    }

    if(o->object_reset_rtn)
    {
        o->object_reset_rtn(o);
    }

    /*Just in case the user tries to force the divider to be 0*/
    if(o->divider == 0)
    {
        o->divider = sd->def_divider;
    }
}

int object_update(object *o)
{
    surface_data* sd = o->sd;

    if(o->vol_var)
    {
        object_reset(o);
    }

    if(o->enabled == 0)
    {
        sfree((void**) &o->sx);
        sfree((void**) &o->sy);
        o->w = 0;
        o->h = 0;
        o->x = 0;
        o->y = 0;
        o->auto_h = 0;
        o->auto_w = 0;
        return (0);
    }


    if(o->object_update_rtn)
    {
        o->object_update_rtn(o);

        if(o->ttip && (o->ot.text || o->ot.title))
        {
            object_tooltip_update(o);
        }

        if(o->update_act_lock == 0)
        {
            o->update_act_lock = 1;
            o->update_act ? command(sd, &o->update_act) : 0;
            o->update_act_lock = 0;
        }
    }

    return (0);
}


void object_render(surface_data* sd, cairo_t* cr)
{
    object* o = NULL;
    list_enum_part(o, &sd->objects, current)
    {
        if(o->die == 1)
        {
            continue;
        }

        object_calculate_coordinates(o);

        if(o->enabled && o->hidden == 0)
        {
            object_render_internal(o, cr);
        }
    }
}

void object_destroy(object** o)
{
    if(o && *o)
    {

        object* to = *o;
        linked_list_remove(&to->current);
        sfree((void**) &to->update_act);
        sfree((void**) &to->team);
        sfree((void**) &to->sx);
        sfree((void**) &to->sy);
        sfree((void**) &to->ot.title);
        sfree((void**) &to->ot.text);

        surface_builtin_destroy(&to->ttip);

        if(to->object_destroy_rtn)
        {
            to->object_destroy_rtn(to);
        }

        bind_destroy(to);

        mouse_destroy_actions(&to->ma);
        skeleton_remove_section(&to->os);

        sfree((void**) o);
    }
}
