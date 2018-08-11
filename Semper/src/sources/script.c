/*
 * Script source
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */

/*The reason of scripting support is to have a
 * source-like behavior without digging deep in the code
 * Also, it is safer and faster to write a script as it
 * runs in its own context and can't harm the main core.
 * Well, it may harm the performance a bit*/

#include <mem.h>
#include <string_util.h>
#include <semper_api.h>
#include <surface.h>
#include <semper.h>
#include <objects/object.h>
#include <sources/source.h>
#include <parameter.h>
#include <sources/script/utils.h>
#include <xpander.h>
#include <math_parser.h>

/*lua prototypes*/
extern void *lua_engine_init(unsigned char *buf, void *ip);
extern void lua_engine_call_reset(void *ctx);
extern double lua_engine_call_update(void *ctx);
extern unsigned char *lua_engine_call_string(void *ctx);
extern void lua_engine_cleanup(void **ctx);
extern void lua_engine_call_command(void *ctx, unsigned char *comm);

/*JavaScript prototypes*/
extern void *js_engine_init(void *buf, void *pv);
extern void js_engine_call_reset(void  *ctx);
extern double js_engine_call_update(void *ctx);
extern unsigned char *js_engine_call_string(void *ctx);
extern void js_engine_cleanup(void **ctx);
extern void js_engine_call_command(void *ctx, unsigned char *comm);

typedef enum
{
    unknown,
    lua,
    javascript,
    python
} script_engine;

typedef struct
{
    void *ctx;              //the script engines are using a context (all of them)
    unsigned char *scode;   //the actual script code
    script_engine eng;
    unsigned char *script_name; //we'll just use this as a reference
    unsigned char *sval;
    unsigned char *ret_val;
    /*we're playing with fire as those structures should not be exposed*/
    control_data *cd;
    source *s;
    surface_data *sd;

} script_data;

void script_init(void **spv, void *ip)
{
    script_data *sc = zmalloc(sizeof(script_data));
    sc->s = ip;
    sc->sd = sc->s->sd;
    sc->cd = sc->sd->cd;
    *spv = sc;
}

void script_reset(void *spv, void *ip)
{
    script_data *sc = spv;

    unsigned char * s = param_string("ScriptPath", 0x3, ip, NULL);
    unsigned char script_changed = 1;

    if(s) //if the scripts differs you know what that means (bye bye old script)
    {
        size_t mem = 0;
        unsigned char *tbuf = NULL;

        if(put_file_in_memory(s, (void**)&tbuf, &mem) == 0)
        {

            unsigned char *ref = NULL;

            if(tbuf[0] == 0xff && tbuf[1] == 0xfe) //unicode? no problem
            {
                unsigned char *ttbuf = zmalloc(mem + 2); //we have to ensure that the string is null terminated
                memcpy(ttbuf, tbuf, mem);
                sfree((void**)&tbuf);
                ref = ucs_to_utf8((unsigned short*)(ttbuf + 4), &mem, 0);
                sfree((void**)&ttbuf);
            }
            else
            {
                unsigned char *ttbuf = zmalloc(mem + 1); //we have to ensure that the string is null terminated
                memcpy(ttbuf, tbuf, mem);
                sfree((void**)&tbuf);
                ref = ttbuf;
            }

            if(ref && sc->scode && strcasecmp(ref, sc->scode) == 0)
            {
                sfree((void**)&ref);
                script_changed = 0;
            }
            else
            {
                sfree((void**)&sc->scode);
                sc->scode = ref;
            }
        }

        sfree((void**)&sc->script_name);
        sc->script_name = clone_string(s);
    }

    /*Destroy the old engine context*/
    if(sc->ctx && script_changed)
    {
        switch(sc->eng)
        {
            case lua:
                lua_engine_cleanup(&sc->ctx);
                break;

            case javascript:
                js_engine_cleanup(&sc->ctx);
                break;

            case python:
            case unknown:
                break;
        }

        sc->ctx = NULL;
    }


    /*Here we are taking the most important decision which implies the engine choice*/
    /*The detection is pretty straight forward:
     * Try to create a context until there's one successful. If it is, then that is our engine. If it's not..well...bad luck
     */
    if(sc->ctx == NULL)
    {
        if((sc->ctx = lua_engine_init(sc->scode, sc)) != NULL)
        {
            sc->eng = lua;
        }
        else if((sc->ctx = js_engine_init(sc->scode, sc)) != NULL)
        {
            sc->eng = javascript;
        }
        else
        {
            sc->eng = unknown;
        }
    }

    /*we should call the reset from the script here*/
    switch(sc->eng)
    {
        case lua:
            lua_engine_call_reset(sc->ctx);
            break;

        case javascript:
            js_engine_call_reset(sc->ctx);
            break;

        case python:
        case unknown:
            break;
    }

}

double script_update(void *spv)
{
    script_data *sc = spv;

    switch(sc->eng)
    {
        case lua:
            return(lua_engine_call_update(sc->ctx));

        case javascript:
            return(js_engine_call_update(sc->ctx));

        case python:
        case unknown:
            break;
    }

    return(0.0);
}

unsigned char *script_string(void *spv)
{
    script_data *sc = spv;
    sfree((void**)&sc->sval);

    switch(sc->eng)
    {
        case lua:
            sc->sval = lua_engine_call_string(sc->ctx);
            break;

        case javascript:
            sc->sval = js_engine_call_string(sc->ctx);
            break;

        case python:
        case unknown:
            break;
    }

    return(sc->sval);
}


void script_destroy(void **spv)
{
    script_data *sc = *spv;

    sfree((void**)&sc->scode);
    sfree((void**)&sc->ret_val);
    sfree((void**)&sc->script_name);

    switch(sc->eng)
    {
        case lua:
            if(sc->ctx)
                lua_engine_cleanup(&sc->ctx);

            break;

        case javascript:
            if(sc->ctx)
                js_engine_cleanup(&sc->ctx);

            break;

        case python:
        case unknown:
            break;
    }

    sfree(spv);
}

void script_command(void *spv, unsigned char *comm)
{
    script_data *sc = spv;

    switch(sc->eng)
    {
        case lua:
            if(sc->ctx)
                lua_engine_call_command(sc->ctx, comm);

            break;

        case javascript:
            if(sc->ctx)
                js_engine_call_command(sc->ctx, comm);

            break;

        case python:
        case unknown:
            break;
    }
}

/*
 ******************************
 *       Bridge routines      *
 ******************************
 */

unsigned char *script_param_retrieve(script_item_data *sid, unsigned char *res, unsigned char *def, unsigned char xflags)
{
    object *o = NULL;
    source *s = NULL;
    script_data *sc = sid->pv;

    switch(sid->type)
    {
        case 1:
            o = object_by_name(sc->sd, sid->name, -1);
            break;

        case 2:
            s = source_by_name(sc->sd, sid->name, -1);
            break;

        default:
            return(NULL);
    }

    if(o == NULL && s == NULL && sid->name[0])
    {
        return(NULL);
    }

    sfree((void**)&sc->ret_val);

    if(!strcasecmp(res, "Name"))
    {
        if(sid->name[0])
        {
            sc->ret_val = clone_string(skeleton_get_section_name((o == NULL ? s->cs : o->os)));
        }
        else
        {
            sc->ret_val = clone_string(skeleton_get_section_name(sc->s->cs));
        }
    }
    else
    {
        if(sid->name[0])
        {
            sc->ret_val = parameter_string(((void*)o == NULL ? (void*)s : (void*)o), res, def, xflags);
        }
        else
        {
            sc->ret_val = parameter_string(sc->s, res, def, XPANDER_REQUESTOR_SOURCE);
        }
    }

    return(sc->ret_val);
}

void script_set_param(script_item_data *sid, unsigned char *p, unsigned char *v)
{
    static unsigned char fmt[] = "Parameter(%s,%s,%s)";
    size_t nm = string_length(v) + string_length(p) + sizeof(fmt) + string_length(sid->name);
    unsigned char *buf = zmalloc(nm);


    script_data *sc = sid->pv;
    snprintf(buf, nm, fmt, sid->name, p, v);
    command(sc->sd, &buf);
    sfree((void**)&buf);
}

double script_object_param(script_item_data *sid, unsigned char id) /*this should get the real parameters*/
{
    script_data *sc = sid->pv;
    object *o = object_by_name(sc->sd, sid->name, -1);

    if(o == NULL)
        return(-1.0);

    switch(id)
    {
        case 1:
            return((double)o->x);

        case 2:
            return((double)o->y);

        case 3:
            return((double)o->auto_w);

        case 4:
            return((double)o->auto_h);
    }

    return(-1.0);
}

static inline double script_source_relative_value(double val, double min, double max)
{
    double range = max - min;

    if(range == 0.0)
        return (1.0);

    val = min(val, max);
    val = max(val, min);
    val -= min;
    return (val / range);
}


void *script_source_param(script_item_data *sid, unsigned char id) /*this should get the real parameters*/
{
    script_data *sc = sid->pv;
    source *s = source_by_name(sc->sd, sid->name, -1);

    if(s == NULL)
    {
        return(NULL);
    }

    static double res = 0;

    switch(id)
    {
        case  1:
            return(&s->d_info);

        case 2:
            return(&s->s_info);

        case 3:
            return(&s->max_val);

        case 4:
            return(&s->min_val);

        case 5:
            res = s->max_val - s->min_val;
            return(&res);

        case 6:
            res = (script_source_relative_value(s->d_info, s->min_val, s->max_val));
            return(&res);
    }

    return(NULL);
}

double script_math(unsigned char *frml)
{
    double r = 0.0;

    if(frml && math_parser(frml, &r, NULL, NULL) == 0)
    {
        return(r);
    }

    return(0.0);
}

void script_send_command(void *pv, unsigned char *cmd) /*this allows the script to send commands to the core*/
{
    script_data *sc = pv;
    command(sc->sd, &cmd);
}

unsigned char *script_xpand(void *pv, unsigned char *s)
{
    script_data *sc = pv;
    xpander_request xr =
    {
        .requestor = sc->sd,
        .os = s,
        .req_type = XPANDER_SURFACE,
        .es = NULL
    };

    if(xpander(&xr))
    {
        sfree((void **)&sc->ret_val);
        sc->ret_val = xr.es;
    }
    else
    {
        sc->ret_val = clone_string(xr.os);
    }

    return(sc->ret_val);
}

unsigned char *script_variable(void *pv, unsigned char *s)
{
    script_data *sc = pv;
    size_t len = string_length(s) + 3;
    unsigned char *ts = zmalloc(len);

    snprintf(ts, len, "$%s$", s);
    script_xpand(pv, ts);

    if(ts && sc->ret_val && !strcasecmp(ts, sc->ret_val))
    {
        sfree((void**)&sc->ret_val);
    }

    sfree((void**)&ts);
    return(sc->ret_val);
}
