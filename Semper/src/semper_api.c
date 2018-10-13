/*
* Semper API
* Part of Project "Semper"
* Written by Alexandru-Daniel Mărgărit
*/

#include <surface.h>
#include <semper_api.h>
#include <math.h>
#include <mem.h>
#include <string_util.h>
#include <string.h>

#include <xpander.h>
#include <sources/source.h>
#include <skeleton.h>
#include <parameter.h>
#include <diag.h>
#ifdef WIN32
#ifdef SEMPER_API
#undef SEMPER_API
#define SEMPER_API __attribute__((dllexport))
#else
#define SEMPER_API __attribute__((dllexport))
#endif
#else
#define SEMPER_API
#endif
typedef struct
{
    surface_data* sd;
    control_data *cd;
    unsigned char* comm;
} extension_command;
extern int diag_log(unsigned char lvl, char *fmt, ...);

SEMPER_API double param_double(unsigned char* pn, void* ip, double def)
{
    if(!ip || !pn)
    {
        return (def);
    }

    source* s = ip;

    return (parameter_double(s, pn, def, XPANDER_SOURCE));
}

SEMPER_API size_t param_size_t(unsigned char* pn, void* ip, size_t def)
{
    if(!ip || !pn)
    {
        return (def);
    }

    source* s = ip;
    return (parameter_size_t(s, pn, def, XPANDER_SOURCE));
}

SEMPER_API int source_set_max(double val, void* ip, unsigned char force, unsigned char hold)
{
    source* s = ip;

    if(s)
    {
        if(force || val > s->max_val)
        {
            s->max_val = val;
            s->hold_max = hold;
            return (1);
        }
    }

    return (0);
}
SEMPER_API int source_set_min(double val, void* ip, unsigned char force, unsigned char hold)
{
    source* s = ip;

    if(s)
    {
        if(force || val < s->min_val)
        {
            s->min_val = val;
            s->hold_min = hold;
            return (1);
        }
    }

    return (0);
}

SEMPER_API unsigned char param_bool(unsigned char* pn, void* ip, unsigned char def)
{
    return ((unsigned char)param_double(pn, ip, (double)def) != 0.0);
}

SEMPER_API unsigned char* param_string(unsigned char* pn, unsigned char flags, void* ip, unsigned char* def)
{
    if(!ip || !pn)
    {
        return (NULL);
    }

    source* s = ip;
    sfree((void**)&s->ext_str);
    s->ext_str = parameter_string(s, pn, def, (flags | XPANDER_REQUESTOR_SOURCE) & 0x13);
    return (s->ext_str);
}

SEMPER_API void* get_surface(void* ip)
{
    source* s = ip;
    return(s ? s->sd : NULL);
}

SEMPER_API unsigned char* get_extension_name(void* ip)
{
    source* s = ip;
    return(s ? skeleton_get_section_name(s->cs) : NULL);
}

SEMPER_API void* get_extension_by_name(unsigned char* name, void* ip)
{
    source* s = ip;

    if(s && s->sd)
    {
        return (source_by_name(s->sd, name, -1));
    }

    return (NULL);
}

SEMPER_API void* get_private_data(void* ip)
{
    return(ip ? (((source*)ip)->pv) : NULL);
}

SEMPER_API int is_parent_candidate(void* pc, void* ip)
{
    source* s = ip;
    source* sp = pc;

    if(sp && s)
    {
        if(s->type == sp->type)
        {
            if(sp->type == 1 && s->extension && sp->extension && !strcasecmp(s->extension, sp->extension))
            {
                return(1);
            }
            else if(sp->type == 1)
            {
                return(0);
            }

            return(1);
        }
    }

    return (0);
}
#warning "Need to review this routine"
static int extension_command_handler(extension_command* ec)
{
    if(!ec||!ec->sd)
    {
        return (-1);
    }

    surface_data *sd = NULL;
    control_data *cd = ec->cd;
    int command_exec = 0;
    list_enum_part(sd, &cd->surfaces, current)
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
        if(cd->srf_reg == ec->sd)
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

SEMPER_API void send_command_ex(void* ir, unsigned char* cmd, size_t timeout, char unique)
{
    if(ir && cmd)
    {
        unsigned char flags = unique ? EVENT_REMOVE_BY_DATA_HANDLER : 0;
        flags |= timeout > 0 ? EVENT_PUSH_TIMER : 0;
        source* s = ir;
        surface_data* sd = s->sd;
        extension_command* ec = zmalloc(sizeof(extension_command));
        ec->sd = sd;
        ec->cd=sd->cd;
        ec->comm = clone_string(cmd);
        event_push(ec->cd->eq, (event_handler)extension_command_handler, (void*)ec, timeout, flags); //we will queue this event to be processed later
    }
}

SEMPER_API void send_command(void* ir, unsigned char* cmd)
{
    send_command_ex(ir, cmd, 0, 0);
}



SEMPER_API int has_parent(unsigned char* str)
{
    size_t strl = string_length(str);

    if(str[0] == '[' && str[strl - 1] == ']')
    {
        return (1);
    }

    return (0);
}

SEMPER_API void* get_parent(unsigned char* str, void* ip)
{
    if(!str || !ip)
    {
        return (NULL);
    }

    size_t strl = string_length(str);

    if(str[0] == '[' && str[strl - 1] == ']')
    {
        source* s = ip;
        source* p = source_by_name(s->sd, str + 1, strl - 2);

        if(is_parent_candidate(p, s))
        {
            return (p);
        }
    }

    return (NULL);
}


SEMPER_API int tokenize_string(tokenize_string_info *tsi)
{
    if(tsi == NULL)
    {
        return (-1);
    }

    return (string_tokenizer((string_tokenizer_info*)tsi));
}

SEMPER_API void tokenize_string_free(tokenize_string_info *tsi)
{
    sfree((void**)&tsi->ovecoff);
}

SEMPER_API unsigned char *get_path(void *ip, unsigned char pth)
{
    if(ip == NULL)
    {
        return(NULL);
    }

    source *s = ip;
    surface_data *sd = s->sd;
    control_data *cd = sd->cd;
    sfree((void**)&s->ext_str);

    switch(pth)
    {
        default:
            return(NULL);

        case EXTENSION_PATH_SEMPER:
            s->ext_str = clone_string(cd->root_dir);
            break;
        case EXTENSION_PATH_EXTENSIONS:
            s->ext_str = clone_string(cd->ext_dir);
            break;
        case EXTENSION_PATH_SURFACE:
            s->ext_str = clone_string(sd->sp.surface_dir);
            break;
        case EXTENSION_PATH_SURFACES:
            s->ext_str = clone_string(cd->surface_dir);
            break;
    }

    return(s->ext_str);

}


SEMPER_API unsigned char *absolute_path(void *ip, unsigned char *rp, unsigned char pth)
{
    if(ip == NULL || rp == NULL)
    {
        return(NULL);
    }

    source *s = ip;
    surface_data *sd = s->sd;
    control_data *cd = sd->cd;
    unsigned char *root = NULL;
    size_t rootl = 0;

    switch(pth)
    {
        default:
            return(NULL);

        case EXTENSION_PATH_SEMPER:
            root = cd->root_dir;
            rootl = cd->root_dir_length;
            break;

        case EXTENSION_PATH_EXTENSIONS:
            root = cd->ext_dir;
            rootl = cd->ext_dir_length;
            break;

        case EXTENSION_PATH_SURFACE:
            root = sd->sp.surface_dir;
            rootl = string_length(sd->sp.surface_dir);
            break;

        case EXTENSION_PATH_SURFACES:
            root = cd->surface_dir;
            rootl = cd->surface_dir_length;
            break;
    }

    if(root)
    {
        size_t rpl = string_length(rp);
        sfree((void**)&s->ext_str);
        s->ext_str = zmalloc(rootl + rpl + 2); //null + /
        snprintf(s->ext_str, rootl + rpl + 2, "%s/%s", root, rp);
        uniform_slashes(s->ext_str);
        return(s->ext_str);
    }

    return(NULL);
}

SEMPER_API int semper_event_remove(void *ip, event_handler eh, void* pv, unsigned char flags)
{
    source *s = ip;
    control_data *cd = NULL;

    if(s && s->sd)
    {
        cd = ((surface_data*)s->sd)->cd;
    }

    if(cd == NULL)
    {
        return(-1);
    }

    event_remove(cd->eq, eh, pv, flags);
    return(0);
}

SEMPER_API int semper_event_push(void *ip, event_handler handler, void* pv, size_t timeout, unsigned char flags)
{
    source *s = ip;
    control_data *cd = NULL;

    if(s && s->sd)
    {
        cd = ((surface_data*)s->sd)->cd;
    }

    if(cd == NULL)
    {
        return(-1);
    }

    return(event_push(cd->eq, handler, pv, timeout, flags));
}

SEMPER_API void semper_safe_flag_destroy(void **psf)
{
    safe_flag_destroy(psf);
}
SEMPER_API size_t semper_safe_flag_get(void *sf)
{
    return(safe_flag_get(sf));
}
SEMPER_API void semper_safe_flag_set(void *sf, size_t flag)
{
    safe_flag_set(sf, flag);
}

SEMPER_API void *semper_safe_flag_init(void)
{
    return(safe_flag_init());
}

SEMPER_API unsigned char *semper_ucs_to_utf8(unsigned short *s_in, size_t *len, unsigned char be)
{
    return(ucs_to_utf8(s_in, len, be));
}

SEMPER_API unsigned short *semper_utf8_to_ucs(unsigned char *s_in)
{
    return(utf8_to_ucs(s_in));
}

SEMPER_API void semper_free(void **p)
{
    sfree(p);
}
