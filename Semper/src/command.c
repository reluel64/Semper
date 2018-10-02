/*
* Command handling routines
* Part of Project 'Semper'
* Written by Alexandru-Daniel Mărgărit
*/

#include <surface.h>
#include <semper.h>
#include <math.h>
#include <objects/object.h>
#include <string_util.h>
#include <mem.h>
#include <sources/source.h>
#include <team.h>
#include <surface_builtin.h>
#include <math_parser.h>
#include <xpander.h>
#include <parameter.h>


#define DEFER_COMMAND 0x2712
#define COMMAND_PARAMETER_STACK 10

typedef struct
{
    surface_data* sd;
    control_data* cd;
    unsigned char* comm;
} command_defer_data;

typedef struct
{
    unsigned char* pms[COMMAND_PARAMETER_STACK];
    size_t plength;
    unsigned char *rem_cmd;
} command_parameters;

typedef struct
{
    unsigned char* act_name;
    int (*handler)(surface_data* sd, command_parameters* cp);
    size_t min_parameters;
} command_info;

typedef struct
{
    unsigned char* comm_name; // command name
    command_parameters cpm;
    surface_data* sd;

} command_handler_status;

typedef struct
{
    size_t op;
    size_t quotes;
    unsigned char quote_type;
} command_tokenizer_status;

#define COMMAND_HANDLER( _func_name ) static int(_func_name)(surface_data * sd, command_parameters * cp)

static int command_defer_handler(command_defer_data* ec)
{
    if(!ec)
    {
        return (-1);
    }

    surface_data *sd = NULL;
    int command_exec = 0;
    list_enum_part(sd, &ec->cd->surfaces, current)
    {
        if(ec->sd == sd)
        {
            command(ec->sd, &ec->comm);
            command_exec = 1;
            break;
        }
    }

    if(command_exec == 0)
    {
        if(ec->cd->srf_reg == ec->sd)
        {
            command(ec->sd, &ec->comm);
            command_exec = 1;
        }
    }

    if(command_exec == 0)
    {
        diag_warn("%s %d Surface %p was not found", __FUNCTION__, __LINE__, ec->sd);
    }

    sfree((void**)&ec->comm);
    sfree((void**)&ec);

    return (0);
}



COMMAND_HANDLER(handler_draggable_command)
{
    unsigned char draggable = 0;

    if(cp->plength > 1)
    {
        sd = surface_by_name(sd->cd, cp->pms[1]);
    }

    if(sd == NULL)
    {
        return (-1);
    }

    crosswin_get_draggable(sd->sw, &draggable);

    if(!strcasecmp("-1", cp->pms[0]))
    {
        if(draggable)
            skeleton_add_key(sd->scd, "Draggable", "0");
        else
            skeleton_add_key(sd->scd, "Draggable", "1");

        draggable = !draggable;
    }
    else
    {
        skeleton_add_key(sd->scd, "Draggable", cp->pms[0]);
        draggable = atoi(cp->pms[0]);
    }

    crosswin_set_draggable(sd->sw, draggable);
    return (0);
}

COMMAND_HANDLER(handler_set_opacity)
{
    if(cp->plength > 1)
    {
        sd = surface_by_name(sd->cd, cp->pms[1]);
    }

    if(sd == NULL)
    {
        return (-1);
    }

    sd->ro = atoi(cp->pms[0]);
    skeleton_add_key(sd->scd, "Opacity", cp->pms[0]);
    surface_fade(sd);
    return (0);
}

COMMAND_HANDLER(handler_load_registry)
{
    unused_parameter(cp);
    control_data* cd = sd->cd;
    return (surface_builtin_init(cd, catalog));
}

COMMAND_HANDLER(handler_unload_registry)
{
    unused_parameter(cp);
    control_data* cd = sd->cd;
    return (surface_builtin_destroy(&cd->srf_reg));
}

COMMAND_HANDLER(handler_keep_position)
{
    if(cp->plength > 1)
    {
        sd = surface_by_name(sd->cd, cp->pms[1]);
    }

    if(sd == NULL)
    {
        return (-1);
    }

    if(!strcasecmp("-1", cp->pms[0]))
    {
        if(sd->snp)
        {
            skeleton_add_key(sd->scd, "KeepPosition", "0");
        }
        else
        {
            skeleton_add_key(sd->scd, "KeepPosition", "1");
        }

        sd->snp = !sd->snp;
    }
    else
    {
        skeleton_add_key(sd->scd, "KeepPosition", cp->pms[0]);
        sd->snp = atoi(cp->pms[0]);
    }

    return (0);
}

COMMAND_HANDLER(handler_reload_if_modified)
{
    if(cp->plength > 1)
    {
        sd = surface_by_name(sd->cd, cp->pms[1]);
    }

    if(sd == NULL)
    {
        return (-1);
    }

    if(!strcasecmp("-1", cp->pms[0]))
    {
        if(sd->rim)
        {
            skeleton_add_key(sd->scd, "ReloadIfModified", "0");
        }
        else
        {
            skeleton_add_key(sd->scd, "ReloadIfModified", "1");
        }

        sd->rim = !sd->rim;
    }
    else
    {
        skeleton_add_key(sd->scd, "ReloadIfModified", cp->pms[0]);
        sd->rim = atoi(cp->pms[0]);
    }

    return (0);
}

COMMAND_HANDLER(handler_keep_on_screen)
{
    unsigned char keep_on_screen = 0;

    if(cp->plength > 1)
    {
        sd = surface_by_name(sd->cd, cp->pms[1]);
    }

    if(sd == NULL)
    {
        return (-1);
    }

    crosswin_get_keep_on_screen(sd->sw, &keep_on_screen);

    if(!strcasecmp("-1", cp->pms[0]))
    {
        if(keep_on_screen)
        {
            skeleton_add_key(sd->scd, "KeepOnScreen", "0");
        }
        else
        {
            skeleton_add_key(sd->scd, "KeepOnScreen", "1");
        }

        keep_on_screen = !keep_on_screen;
    }
    else
    {
        skeleton_add_key(sd->scd, "KeepOnScreen", cp->pms[0]);
        keep_on_screen = atoi(cp->pms[0]);
    }

    crosswin_set_keep_on_screen(sd->sw, keep_on_screen);
    return (0);
}

COMMAND_HANDLER(handler_click_through)
{
    unsigned char click_through = 0;

    if(cp->plength > 1)
    {
        sd = surface_by_name(sd->cd, cp->pms[1]);
    }

    if(sd == NULL)
    {
        return (-1);
    }

    crosswin_get_click_through(sd->sw, &click_through);

    if(!strcasecmp("-1", cp->pms[0]))
    {
        if(click_through)
        {
            skeleton_add_key(sd->scd, "Clickthrough", "0");
        }
        else
        {
            skeleton_add_key(sd->scd, "Clickthrough", "1");
        }

        click_through = !click_through;

    }
    else
    {
        skeleton_add_key(sd->scd, "Clickthrough", cp->pms[0]);
        click_through = atoi(cp->pms[0]);
    }

    crosswin_set_click_through(sd->sw, click_through);
    return (0);
}

COMMAND_HANDLER(handler_show_command)
{
    if(cp->plength > 0)
    {
        sd = surface_by_name(sd->cd, cp->pms[0]);
    }

    if(sd == NULL)
    {
        return (-1);
    }

    skeleton_add_key(sd->scd, "hidden", "0");
    crosswin_set_opacity(sd->sw, sd->ro);
    crosswin_set_visible(sd->sw, 1);
    return (0);
}

COMMAND_HANDLER(handler_hide_command)
{
    if(cp->plength > 0)
    {
        sd = surface_by_name(sd->cd, cp->pms[0]);
    }

    if(sd == NULL)
    {
        return (-1);
    }

    skeleton_add_key(sd->scd, "Hidden", "1");
    crosswin_set_opacity(sd->sw, 0);
    crosswin_set_visible(sd->sw, 0);
    return (0);
}

COMMAND_HANDLER(handler_hide_fade_command)
{
    if(cp->plength > 0)
    {
        sd = surface_by_name(sd->cd, cp->pms[0]);
    }

    if(sd == NULL)
    {
        return (-1);
    }

    skeleton_add_key(sd->scd, "Hidden", "1");
    sd->ro = 0;
    sd->fade_direction = -1;
    surface_fade(sd);
    return (0);
}

COMMAND_HANDLER(handler_show_fade_command)
{
    if(cp->plength > 0)
    {
        sd = surface_by_name(sd->cd, cp->pms[0]);
    }

    if(sd == NULL)
    {
        return (-1);
    }

    skeleton_add_key(sd->scd, "Hidden", "0");
    sd->ro = parameter_byte(sd, "Opacity", 255, XPANDER_SURFACE_CONFIG);
    sd->fade_direction = 1;
    surface_fade(sd);
    return (0);
}

COMMAND_HANDLER(handler_update_surface)
{
    if(cp->plength != 0)
    {
        sd = surface_by_name(sd->cd, cp->pms[0]);
    }

    if(sd)
    {
        event_push(sd->cd->eq, (event_handler)surface_update, (void*)sd, 0, EVENT_REMOVE_BY_DATA_HANDLER | EVENT_PUSH_TAIL);
    }

    return (0);
}

COMMAND_HANDLER(handler_add_object)
{
    if(cp->plength > 2)
    {
        sd = surface_by_name(sd->cd, cp->pms[2]);
    }

    if(sd == NULL)
    {
        return (0);
    }

    section s = skeleton_get_section(&sd->skhead, cp->pms[0]); // check is there is something in the skeleton

    if(s == NULL)
    {
        s = skeleton_add_section(&sd->skhead, cp->pms[0]);
        skeleton_add_key(s, "Object", cp->pms[1]);
        object_init(s, sd);
    }

    return (1);
}


COMMAND_HANDLER(handler_add_source)
{
    if(cp->plength > 2)
    {
        sd = surface_by_name(sd->cd, cp->pms[2]);
    }

    if(sd == NULL)
    {
        return (0);
    }

    section s = skeleton_get_section(&sd->skhead, cp->pms[0]); // check is there is something in the skeleton

    if(s == NULL)
    {
        s = skeleton_add_section(&sd->skhead, cp->pms[0]);
        skeleton_add_key(s, "Source", cp->pms[1]);
        source_init(s, sd);
    }

    return (1);
}

COMMAND_HANDLER(handler_change_param)
{
    unsigned char* in = cp->pms[0];
    unsigned char* param = cp->pms[1];
    unsigned char* nv = NULL;

    if(cp->plength >= 4)
    {
        sd = surface_by_name(sd->cd, cp->pms[3]);
    }

    if(cp->plength >= 3)
    {
        nv = cp->pms[2];
    }

    if(sd == NULL)
    {
        return (-1);
    }

    object* o = object_by_name(sd, in, -1);
    source* s = source_by_name(sd, in, -1);

    if(o)
    {
        if(nv)
        {
            skeleton_add_key(o->os, param, nv);
        }
        else
        {
            key k = skeleton_get_key(o->os, param);
            skeleton_key_remove(&k);
        }

        o->vol_var = 1;
    }
    else if(s)
    {
        if(nv)
        {
            skeleton_add_key(s->cs, param, nv);
        }
        else
        {
            key k = skeleton_get_key(s->cs, param);
            skeleton_key_remove(&k);
        }

        s->vol_var = 1;
    }

    return (1);
}

COMMAND_HANDLER(handler_surface_pos)
{
    unsigned char* x = cp->pms[0];
    unsigned char* y = cp->pms[1];

    if(cp->plength >= 3)
    {
        sd = surface_by_name(sd->cd, cp->pms[2]);
    }

    if(sd == NULL || cp->plength < 2)
    {
        return (-1);
    }

    double xd = 0.0;
    double yd = 0.0;

    if(!math_parser(x, &xd, NULL, NULL) && !math_parser(y, &yd, NULL, NULL))
    {
        crosswin_set_position(sd->sw, (long)xd, (long)yd);

        if(sd->snp)
        {
        	long x = 0;
        	long y =0;
        	size_t monitor = 0;
        	unsigned char buf[260] = { 0 };
        	crosswin_get_position(sd->sw, &x, &y, &monitor);

            snprintf(buf, sizeof(buf), "%ld", x);
            skeleton_add_key(sd->scd, "X", buf);

            snprintf(buf, sizeof(buf), "%ld", y);
            skeleton_add_key(sd->scd, "Y", buf);

            snprintf(buf, sizeof(buf), "%llu", monitor);
            skeleton_add_key(sd->scd, "Monitor", buf);
            /*Defer the parameter update*/
            event_push(sd->cd->eq, (event_handler)semper_save_configuration, (void*)sd->cd, 100, EVENT_PUSH_TIMER | EVENT_REMOVE_BY_DATA_HANDLER);
        }
    }

    return (1);
}

COMMAND_HANDLER(handler_execute)
{
    unused_parameter(sd);
    int ret = 0;
#ifdef WIN32
    wchar_t* s = utf8_to_ucs(cp->pms[0]);

    if(s)
    {
        CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        ret = (size_t)ShellExecuteW(NULL, NULL, s, NULL, NULL, SW_SHOW);
        CoUninitialize();
        sfree((void**)&s);
    }

#endif
    return (!ret);
}

COMMAND_HANDLER(handler_change_variable)
{
    unsigned char* vn = cp->pms[0];
    unsigned char* vv = NULL;

    if(cp->plength > 1)
    {
        vv = cp->pms[1];
    }

    if(cp->plength > 2)
    {
        sd = surface_by_name(sd->cd, cp->pms[2]);
    }

    if(sd == NULL)
    {
        return(-1);
    }

    double val = 0.0;
    int is_math = 0;

    if(vv != NULL)
    {
        is_math = math_parser(vv, &val, NULL, NULL) == 0;
    }

    if(is_math)
    {
        vv = zmalloc(64);
        snprintf(vv, 64, "%lf", val);
        remove_trailing_zeros(vv);
    }

    if(sd->sv == NULL)
    {
        sd->sv = skeleton_add_section(&sd->skhead, "Surface-Variables");
    }

    if(vv)
    {
        skeleton_add_key(sd->sv, vn, vv);
    }
    else
    {
        key k = skeleton_get_key(sd->sv, vn);
        skeleton_key_remove(&k);
    }

    if(is_math)
    {
        sfree((void**)&vv);
    }

    return (1);
}

COMMAND_HANDLER(handler_switch_source)
{
    unsigned char ns = 0;
    source* s = NULL;

    if(cp->plength >= 1)
    {
        s = source_by_name(sd, cp->pms[0], -1);
    }

    if(!s)
    {
        return (-1);
    }

    if(cp->plength >= 2)
    {
        ns = (unsigned char)compute_formula(cp->pms[1]);
    }
    else
    {
        ns = s->disabled;
    }

    skeleton_add_key(s->cs, "Disabled", ns ? "0" : "1");
    s->vol_var = 1;
    return (1);
}

COMMAND_HANDLER(handler_source_command)
{
    surface_data* lsd = sd;

    if(cp->plength >= 3)
    {
        lsd = surface_by_name(sd->cd, cp->pms[2]);
    }

    if(lsd)
    {
        source* s = source_by_name(lsd, cp->pms[0], -1);

        if(s && s->disabled == 0 && s && s->source_command_rtn)
        {
            s->source_command_rtn(s->pv, cp->pms[1]);
        }
    }

    return (1);
}

COMMAND_HANDLER(handler_unload_surface)
{
    if(cp->plength)
    {
        sd = surface_by_name(sd->cd, cp->pms[0]);
    }

    if(sd)
    {
        section* s = NULL;

        if(cp->plength)
        {
            s = skeleton_get_section(&sd->cd->shead, cp->pms[0]);
        }
        else
        {
            s = sd->scd;
        }

        skeleton_add_key(s, "Variant", "0");

        semper_save_configuration(sd->cd);
        surface_destroy(sd);
    }

    return (1);
}

COMMAND_HANDLER(handler_load_surface)
{

    section* s = skeleton_add_section(&sd->cd->shead, cp->pms[0]);
    control_data* cd = sd->cd;

    unsigned char* sfp = zmalloc(cd->surface_dir_length + string_length(cp->pms[0]) + 3);
    strcpy(sfp, cd->surface_dir);
    sfp[cd->surface_dir_length] = '/';
    strcpy(sfp + cd->surface_dir_length + 1, cp->pms[0]);

    size_t var = 0;

    if(cp->plength > 1)
    {
        var = surface_file_variant(sfp, cp->pms[1]);
    }

    sfree((void**)&sfp);
    surface_data* ld_srf = NULL;

    if(surface_load(sd->cd, cp->pms[0], var ? var : 1))
    {
        ld_srf = surface_by_name(sd->cd, cp->pms[0]);

        if(ld_srf->sp.variant != (var ? var : 1))
        {
            surface_change_variant(ld_srf, cp->pms[1]);
        }
    }
    else
    {
        ld_srf = surface_by_name(sd->cd, cp->pms[0]);

        if(ld_srf)
        {
            surface_init_update(ld_srf);
        }
    }

    if(ld_srf)
    {
        unsigned char v_buf[260];
        memset(v_buf, 0, sizeof(v_buf));
        snprintf(v_buf, sizeof(v_buf), "%llu", ld_srf->sp.variant);
        skeleton_add_key(s, "Variant", v_buf);
        semper_save_configuration(sd->cd);
    }

    return (1);
}

COMMAND_HANDLER(handler_remove)
{

    source* s = NULL;
    object* o = NULL;

    if(cp->plength == 1)
    {
        s = source_by_name(sd, cp->pms[0], -1);
        o = object_by_name(sd, cp->pms[0], -1);
    }
    else if(cp->plength > 1)
    {
        sd = surface_by_name(sd->cd, cp->pms[1]);
        s = source_by_name(sd, cp->pms[0], -1);
        o = object_by_name(sd, cp->pms[0], -1);
    }

    if(sd && o)
    {
        o->die = 1;
    }
    else if(sd && s)
    {
        s->die = 1; // this source is dead
        list_enum_part(s, &sd->sources, current)
        {
            s->vol_var = 1; // trigger the sources to self-reset
        }
    }

    return (1);
}

COMMAND_HANDLER(handler_parameter_team)
{
    surface_data* lsd = sd;
    object* o = NULL;
    source* s = NULL;
    unsigned char* nv = NULL;
    unsigned char* param = NULL;

    if(cp->plength >= 4)
    {
        lsd = surface_by_name(sd->cd, cp->pms[3]);

        if(lsd == NULL)
            return (0);
    }

    if(cp->plength >= 3)
    {
        nv = cp->pms[2];
    }

    if(cp->plength >= 2)
    {
        param = cp->pms[1];
    }

    list_enum_part(o, &lsd->objects, current)
    {
        if(o->die == 0 && team_member(o->team, cp->pms[0]))
        {
            if(nv)
            {
                skeleton_add_key(o->os, param, nv);
            }
            else
            {
                key k = skeleton_get_key(o->os, param);
                skeleton_key_remove(&k);
            }

            o->vol_var = 1;
        }
    }
    list_enum_part(s, &lsd->sources, current)
    {
        if(s->die == 0 && team_member(s->team, cp->pms[0]))
        {
            if(nv)
            {
                skeleton_add_key(s->cs, param, nv);
            }
            else
            {
                key k = skeleton_get_key(s->cs, param);
                skeleton_key_remove(&k);
            }

            s->vol_var = 1;
        }
    }
    return (1);
}

COMMAND_HANDLER(handler_update_team)
{
    object* o = NULL;
    source* s = NULL;

    if(cp->plength > 1)
    {
        sd = surface_by_name(sd->cd, cp->pms[1]);

        if(!sd)
        {
            return (0);
        }
    }

    list_enum_part(o, &sd->objects, current)
    {
        if(team_member(o->team, cp->pms[0]))
        {
            object_update(o);
        }
    }

    list_enum_part(s, &sd->sources, current)
    {
        if(team_member(s->team, cp->pms[0]))
        {
            source_update(s);
        }
    }

    return (1);
}

COMMAND_HANDLER(handler_update)
{
    surface_data* lsd = sd;

    if(cp->plength > 1)
    {
        lsd = surface_by_name(sd->cd, cp->pms[1]);
        lsd = lsd ? lsd : sd;
    }

    object* o = object_by_name(lsd, cp->pms[0], -1);
    source* s = source_by_name(lsd, cp->pms[0], -1);

    if(!strcasecmp(cp->pms[0], "S*"))
    {
        list_enum_part(s, &lsd->sources, current)
        {
            if(s->die == 0)
            {
                source_update(s);
            }
        }
    }
    else if(!strcasecmp(cp->pms[0], "O*"))
    {
        list_enum_part(o, &lsd->objects, current)
        {
            if(o->die == 0)
            {
                object_update(o);
            }
        }
    }
    else if(o && o->die == 0)
    {
        object_update(o);
    }
    else if(s && s->die == 0)
    {
        source_update(s);
    }

    return (1);
}

COMMAND_HANDLER(handler_reset)
{
    surface_data* lsd = sd;

    if(cp->plength > 1)
    {
        lsd = surface_by_name(sd->cd, cp->pms[1]);
        lsd = lsd ? lsd : sd;
    }

    object* o = object_by_name(lsd, cp->pms[0], -1);
    source* s = source_by_name(lsd, cp->pms[0], -1);

    if(o && o->die == 0)
    {
        o->vol_var = 1;
    }
    else if(s && s->die == 0)
    {
        s->vol_var = 1;
    }

    return (1);
}

COMMAND_HANDLER(handler_force_draw)
{
    if(cp && cp->plength > 1)
    {
        sd = surface_by_name(sd->cd, cp->pms[1]);

        if(sd == NULL)
        {
            return (-1);
        }

    }

    surface_adjust_size(sd);
    crosswin_draw(sd->sw);
    return (1);
}

COMMAND_HANDLER(handler_defer)
{
    if(cp && cp->plength >= 1)
    {
        control_data* cd = sd->cd;
        command_defer_data* cdd = zmalloc(sizeof(command_defer_data));
        cdd->sd = sd;
        cdd->cd = cd;
        cdd->comm = clone_string(cp->rem_cmd);
        event_push(cd->eq, (event_handler)command_defer_handler, (void*)cdd, atoi(cp->pms[0]), EVENT_PUSH_TIMER);
        return(DEFER_COMMAND);
    }

    return(0);
}

COMMAND_HANDLER(handler_quit_app)
{
    control_data* cd = sd->cd;
    safe_flag_set(cd->quit_flag, 1);
    event_wake(cd->eq);
    return(0);
}


/******************************************************************************/

static int command_execute(command_handler_status* chs)
{
    int ret = 0;
    int found = 0;

    static command_info ci[] =
    {
        //command name          command handler        minimum parameters
        { "UnloadSurface",      handler_unload_surface,     0 },
        { "UpdateSurface",      handler_update_surface,     0 },
        { "ForceDraw",          handler_force_draw,         0 },
        { "Show",               handler_show_command,       0 },
        { "Hide",               handler_hide_command,       0 },
        { "ShowFade",           handler_show_fade_command,  0 },
        { "HideFade",           handler_hide_fade_command,  0 },
        { "LoadRegistry",       handler_load_registry,      0 },
        { "UnLoadRegistry",     handler_unload_registry,    0 },
        { "Quit",               handler_quit_app,           0 },

        { "Execute",            handler_execute,            1 },
        { "Variable",           handler_change_variable,    1 },
        { "Update",             handler_update,             1 },
        { "Reset",              handler_reset,              1 },
        { "SwitchSource",       handler_switch_source,      1 },
        { "LoadSurface",        handler_load_surface,       1 },
        { "Remove",             handler_remove,             1 },
        { "UpdateTeam",         handler_update_team,        1 },
        { "Draggable",          handler_draggable_command,  1 },
        { "KeepOnScreen",       handler_keep_on_screen,     1 },
        { "KeepPosition",       handler_keep_position,      1 },
        { "ClickThrough",       handler_click_through,      1 },
        { "ReloadIfModified",   handler_reload_if_modified, 1 },
        { "SetOpacity",         handler_set_opacity,        1 },
        { "Defer",              handler_defer,              1 },

        { "Parameter",          handler_change_param,       2 },
        { "SurfacePos",         handler_surface_pos,        2 },
        { "AddObject",          handler_add_object,         2 },
        { "ParameterTeam",      handler_parameter_team,     2 },
        { "SourceCommand",      handler_source_command,     2 },
        { "AddSource",          handler_add_source,         2 }
    };

    if(chs->comm_name)
    {
        for(size_t i = 0; i < sizeof(ci) / sizeof(command_info); i++)
        {

            if(ci[i].handler && chs->comm_name && (strcasecmp(chs->comm_name, ci[i].act_name) == 0))
            {
                found = 1;

                if((ci[i].min_parameters == 0) || (ci[i].min_parameters <= chs->cpm.plength))
                {
                    ret = ci[i].handler(chs->sd, &chs->cpm);

                }
                else
                {
                	diag_error("Not enough parameters to execute %s",chs->comm_name);
                }
                break;

            }
        }
    }
    else
    {
        diag_warn("%s: NULL command", __FUNCTION__);
    }

    if(found == 0)
    {
        diag_warn("%s: Command \"%s\" not found", __FUNCTION__, chs->comm_name);
    }

    return (ret);
}

static int command_parse_string_filter(string_tokenizer_status *pi, void* pv)
{
    command_tokenizer_status* cts = pv;

    if(pi->reset)
    {
        memset(cts, 0, sizeof(command_tokenizer_status));
        return(0);
    }

    if(cts->quote_type == 0 && (pi->buf[pi->pos] == '"' || pi->buf[pi->pos] == '\''))
        cts->quote_type = pi->buf[pi->pos];

    if(pi->buf[pi->pos] == cts->quote_type)
        cts->quotes++;

    if(cts->quotes % 2)
        return(0);
    else
        cts->quote_type = 0;


    if(pi->buf[pi->pos] == '(')
        if(++cts->op == 1)
            return(1);

    if(pi->buf[pi->pos] == ';')
        return (cts->op == 0);

    if(cts->op == 1 && pi->buf[pi->pos] == ',')
        return (1);

    if(cts->op && pi->buf[pi->pos] == ')')
           if(--cts->op == 0)
               return(0);

    return (0);
}


int command(surface_data* sd, unsigned char **pa)
{
    unsigned char push_params = 0;
    unsigned char execute = 0;
    unsigned char stack_pos = 0;

    if(pa == NULL || *pa == NULL || sd == NULL || sd->cd == NULL)
    {
        diag_warn("%s %d control_data %p surface_data %p action %p", __FUNCTION__, __LINE__, (sd?sd->cd:NULL), sd, pa);
        return (-1);
    }

    command_tokenizer_status cts = { 0 };
    command_handler_status   chs =  { 0 };
    string_tokenizer_info    sti =
    {
        .buffer                  = *pa, //store the string address here
        .filter_data             = &cts,
        .string_tokenizer_filter = command_parse_string_filter,
        .ovecoff                 = NULL,
        .oveclen                 = 0
    };

    *pa = NULL;

    string_tokenizer(&sti);

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
                push_params = 1;
            }
            else if(sti.buffer[start] == ',')
            {
                start++;
            }
            else if(sti.buffer[start] == ';')
            {
                push_params = 0;
                execute = 0;
                start++;
            }

            if(sti.buffer[end - 1] == ')')
            {
                end--;
                execute = 1;
            }
        }

        if(string_strip_space_offsets(sti.buffer, &start, &end) == 0)
        {
            if(sti.buffer[start] == '"' || sti.buffer[start] == '\'')
                start++;

            if(sti.buffer[end - 1] == '"' || sti.buffer[end - 1] == '\'')
                end--;
        }

        if(push_params && stack_pos < COMMAND_PARAMETER_STACK)
        {
            if((execute && end != start) || execute == 0)
            {
                chs.cpm.pms[stack_pos] = zmalloc((end - start) + 1);
                strncpy(chs.cpm.pms[stack_pos], sti.buffer + start, end - start);
                stack_pos++;
                chs.cpm.plength++;
            }
        }
        else if(chs.comm_name == NULL)
        {
            chs.comm_name = zmalloc((end - start) + 1);
            strncpy(chs.comm_name, sti.buffer + start, end - start);
        }

        if(execute)
        {
            int exec_ret = 0;
            chs.sd = sd;
            chs.cpm.rem_cmd = sti.buffer + sti.ovecoff[2 * i + 1];
            exec_ret = command_execute(&chs);
            execute = 0;

            for(size_t i = 0; i < chs.cpm.plength; i++)
            {
                sfree((void**)&chs.cpm.pms[i]);
            }

            stack_pos = 0;
            push_params = 0;
            chs.cpm.plength = 0;

            sfree((void**)&chs.comm_name);

            if(exec_ret == DEFER_COMMAND)
                break;
        }
    }

    sfree((void**)&sti.ovecoff);

    /*If the string has been set during command processing
     * then *pa will not be NULL and we could free the stored command.
     * Otherwise we just  restore the value
     * In this way we do not have to allocate additional
     * memory to store a copy of the string
     * */

    if(*pa == NULL)
        *pa = sti.buffer;
    else
        sfree((void**)&sti.buffer);

    return (0);
}
