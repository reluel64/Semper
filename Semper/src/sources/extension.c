/*
Extension API
Part of Project "Semper"
Written by Alexandru-Daniel Mărgărit
*/

#include <surface.h>
#include <sources/extension.h>
#include <math.h>
#include <mem.h>
#include <string_util.h>
#include <string.h>

#include <xpander.h>
#include <sources/source.h>
#include <skeleton.h>
#include <parameter.h>

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
typedef struct _extension_command
{
    surface_data* sd;
    control_data* cd;
    unsigned char* comm;
} extension_command;

SEMPER_API double extension_double(unsigned char* pn, void* ip, double def)
{
    if(!ip || !pn)
    {
        return (def);
    }

    source* s = ip;

    return (parameter_double(s, pn, def, XPANDER_SOURCE));
}

SEMPER_API size_t extension_size_t(unsigned char* pn, void* ip, size_t def)
{
    if(!ip || !pn)
    {
        return (def);
    }

    source* s = ip;
    return (parameter_size_t(s, pn, def, XPANDER_SOURCE));
}

SEMPER_API int extension_set_max(double val, void* ip, unsigned char force, unsigned char hold)
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
SEMPER_API int extension_set_min(double val, void* ip, unsigned char force, unsigned char hold)
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

SEMPER_API unsigned char extension_bool(unsigned char* pn, void* ip, unsigned char def)
{
    return ((unsigned char)extension_double(pn, ip, (double)def) != 0.0);
}

SEMPER_API unsigned char* extension_string(unsigned char* pn, unsigned char flags, void* ip, unsigned char* def)
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

SEMPER_API void* extension_surface(void* ip)
{
    source* s = ip;
    return(s?s->sd:NULL);
}

SEMPER_API unsigned char* extension_name(void* ip)
{
    source* s = ip;
    return(s?skeleton_get_section_name(s->cs):NULL);
}

SEMPER_API void* extension_by_name(unsigned char* name, void* ip)
{
    source* s = ip;

    if(s && s->sd)
    {
        return (source_by_name(s->sd, name,-1));
    }

    return (NULL);
}

SEMPER_API void* extension_private(void* ip)
{
    source* s = ip;

    if(s)
    {
        return (s->pv);
    }

    return (NULL);
}

SEMPER_API int extension_parent_candidate(void* pc, void* ip)
{
    source* s = ip;
    source* sp = pc;

    if(sp && s)
    {
        if(s->type==sp->type)
        {
            if(sp->type==1&&s->extension && sp->extension && !strcasecmp(s->extension, sp->extension))
            {
                return(1);
            }
            else if(sp->type==1)
            {
                return(0);
            }

            return(1);
        }
    }

    return (0);
}

static int extension_command_handler(extension_command* ec)
{
    if(!ec)
    {
        return (-1);
    }

    command(ec->sd, &ec->comm);
    sfree((void**)&ec->comm);
    sfree((void**)&ec);

    return (0);
}

SEMPER_API void extension_send_command(void* ir, unsigned char* command)
{
    if(ir && command)
    {
        source* s = ir;
        surface_data* sd = s->sd;
        control_data* cd = sd->cd;
        extension_command* ec = zmalloc(sizeof(extension_command));
        ec->sd = sd;
        ec->comm = clone_string(command);
        event_push(cd->eq, (event_handler)extension_command_handler, (void*)ec, 0, EVENT_PUSH_TAIL); //we will queue this event to be processed later
    }
}

SEMPER_API int extension_has_parent(unsigned char* str)
{
    size_t strl = string_length(str);

    if(str[0] == '[' && str[strl - 1] == ']')
    {
        return (1);
    }

    return (0);
}

SEMPER_API void* extension_get_parent(unsigned char* str, void* ip)
{
    if(!str || !ip)
    {
        return (NULL);
    }

    size_t strl = string_length(str);

    if(str[0] == '[' && str[strl - 1] == ']')
    {
        source* s = ip;
        source* p = source_by_name(s->sd, str + 1,strl-2);

        if(extension_parent_candidate(p, s))
        {
            return (p);
        }
    }

    return (NULL);
}


SEMPER_API int extension_tokenize_string(extension_string_tokenizer_info *esti)
{
    if(esti == NULL)
    {
        return (-1);
    }

    return (string_tokenizer((string_tokenizer_info*)esti));
}

SEMPER_API void extension_tokenize_string_free(extension_string_tokenizer_info *esti)
{
    sfree((void**)&esti->ovecoff);
}

SEMPER_API unsigned char *extension_get_path(void *ip,unsigned char pth)
{
    if(ip==NULL)
    {
        return(NULL);
    }

    source *s=ip;
    surface_data *sd=s->sd;
    control_data *cd=sd->cd;

    switch(pth)
    {
        default:
            return(NULL);

        case EXTENSION_PATH_SEMPER:
            return(cd->root_dir);

        case EXTENSION_PATH_EXTENSIONS:
            return(cd->ext_dir);

        case EXTENSION_PATH_SURFACE:
            return(sd->sp.surface_dir);

        case EXTENSION_PATH_SURFACES:
            return(cd->surface_dir);
    }
}


SEMPER_API unsigned char *extension_absolute_path(void *ip,unsigned char *rp,unsigned char pth)
{
    if(ip==NULL||rp==NULL)
    {
        return(NULL);
    }

    source *s=ip;
    surface_data *sd=s->sd;
    control_data *cd=sd->cd;
    unsigned char *root=NULL;
    size_t rootl=0;

    switch(pth)
    {
        default:
            return(NULL);

        case EXTENSION_PATH_SEMPER:
            root=cd->root_dir;
            rootl=cd->root_dir_length;
            break;

        case EXTENSION_PATH_EXTENSIONS:
            root=cd->ext_dir;
            rootl=cd->ext_dir_length;
            break;

        case EXTENSION_PATH_SURFACE:
            root=sd->sp.surface_dir;
            rootl=string_length(sd->sp.surface_dir);
            break;

        case EXTENSION_PATH_SURFACES:
            root=cd->surface_dir;
            rootl=cd->surface_dir_length;
            break;
    }

    if(root)
    {
        size_t rpl=string_length(rp);
        sfree((void**)&s->ext_str);
        s->ext_str=zmalloc(rootl+rpl+2); //null + /
        snprintf(s->ext_str,rootl+rpl+2,"%s/%s",root,rp);
#ifdef WIN32
        windows_slahses(s->ext_str);
#endif
        uniform_slashes(s->ext_str);
        return(s->ext_str);
    }

    return(NULL);
}
