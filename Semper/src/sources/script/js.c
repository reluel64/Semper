/* JavaScript support using Duktape
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */

#include <string_util.h>
#include <sources/script/utils.h>
#include <xpander.h>
#include <3rdparty/duktape.h>
#include <math_parser.h>

static script_item_data *js_engine_item_data(duk_context *ctx,unsigned char *p)
{
    size_t buf_sz=0;
    script_item_data *sid=NULL;
    duk_push_this(ctx);
    if(duk_get_prop_string(ctx, -1, p))
    {
        sid=duk_get_buffer(ctx,-1,&buf_sz);

        if(buf_sz!=sizeof(script_item_data))
        {
            sid=NULL;
        }
    }
    duk_pop_n(ctx,2);
    return(sid);
}

/*
 *****************************
 * Object API for JavaScript *
 *****************************
 */

static duk_ret_t js_engine_object_get_name(duk_context *ctx)
{
    script_item_data *sid=js_engine_item_data(ctx,"SemperObjectData");

    if(sid)
    {
        duk_push_string(ctx,sid->name);
        return(1);
    }
    return(0);
}

static duk_ret_t js_engine_object_get_x(duk_context *ctx)
{
    script_item_data *sid=js_engine_item_data(ctx,"SemperObjectData");

    if(sid)
    {
        double d= script_object_param(sid,1);
        duk_push_number(ctx,d);
        return(1);
    }

    return(0);
}

static duk_ret_t js_engine_object_get_y(duk_context *ctx)
{
    script_item_data *sid=js_engine_item_data(ctx,"SemperObjectData");
    
    if(sid)
    {
        double d= script_object_param(sid,2);
        duk_push_number(ctx,d);
        return(1);
    }
    
    return(0);
}

static duk_ret_t js_engine_object_get_w(duk_context *ctx)
{
    script_item_data *sid=js_engine_item_data(ctx,"SemperObjectData");

    if(sid)
    {
        double d= script_object_param(sid,3);
        duk_push_number(ctx,d);
        return(1);
    }

    return(0);
}

static duk_ret_t js_engine_object_get_h(duk_context *ctx)
{
    script_item_data *sid=js_engine_item_data(ctx,"SemperObjectData");

    if(sid)
    {
        double d= script_object_param(sid,4);
        duk_push_number(ctx,d);
        return(1);
    }

    return(0);
}

static duk_ret_t js_engine_object_param(duk_context *ctx)
{
    unsigned char *s=(unsigned char*)duk_to_string(ctx,-1);
    script_item_data *sid=js_engine_item_data(ctx,"SemperObjectData");

    if(sid)
    {
        unsigned char *r=script_param_retrieve(sid,(unsigned char*)s,NULL,XPANDER_REQUESTOR_OBJECT);
        if(r)
        {
            duk_push_string(ctx,r);
            return(1);
        }
    }

    return(0);
}

static duk_ret_t js_engine_object_set_x(duk_context *ctx)
{
    double d=duk_to_number(ctx,-1);
    script_item_data *sid=js_engine_item_data(ctx,"SemperObjectData");

    if(sid==NULL)
    {
        return(0);
    }
    unsigned char buf[64]= {0};
    snprintf(buf,64,"%lf",d);
    script_set_param(sid,"X",buf);
    return(0);
}

static duk_ret_t js_engine_object_set_y(duk_context *ctx)
{
    double d=duk_to_number(ctx,-1);
    script_item_data *sid=js_engine_item_data(ctx,"SemperObjectData");

    if(sid==NULL)
    {
        return(0);
    }
    unsigned char buf[64]= {0};
    snprintf(buf,64,"%lf",d);
    script_set_param(sid,"Y",buf);
    return(0);
}

static duk_ret_t js_engine_object_set_w(duk_context *ctx)
{
    double d=duk_to_number(ctx,-1);
    script_item_data *sid=js_engine_item_data(ctx,"SemperObjectData");

    if(sid==NULL)
    {
        return(0);
    }
    unsigned char buf[64]= {0};
    snprintf(buf,64,"%lf",d);
    script_set_param(sid,"W",buf);
    return(0);
}

static duk_ret_t js_engine_object_set_h(duk_context *ctx)
{
    double d=duk_to_number(ctx,-1);
    script_item_data *sid=js_engine_item_data(ctx,"SemperObjectData");

    if(sid==NULL)
    {
        return(0);
    }
    unsigned char buf[64]= {0};
    snprintf(buf,64,"%lf",d);
    script_set_param(sid,"H",buf);
    return(0);
}

static duk_ret_t js_engine_object_show(duk_context *ctx)
{
    script_item_data *sid=js_engine_item_data(ctx,"SemperObjectData");

    if(sid==NULL)
    {
        return(0);
    }
    script_set_param(sid,"Hidden","0");
    return(0);
}

static duk_ret_t js_engine_object_hide(duk_context *ctx)
{
    script_item_data *sid=js_engine_item_data(ctx,"SemperObjectData");

    if(sid==NULL)
    {
        return(0);
    }
    script_set_param(sid,"Hidden","1");
    return(0);
}

/*
 ********************************
 * Source API for JavaScript    *
 ********************************
 */

static int js_engine_source_get_name(duk_context *ctx)
{
    script_item_data *sid=js_engine_item_data(ctx,"SemperSourceData");

    if(sid)
    {
        void *o=script_param_retrieve(sid,"Name",NULL,0);

        if(o==NULL)
        {
            return(0);
        }
        duk_push_string(ctx,o);
        return(1);
    }
    return(0);
}

static duk_ret_t js_engine_source_param(duk_context *ctx)
{
    unsigned char *s=(unsigned char*)duk_to_string(ctx,-1);
    script_item_data *sid=js_engine_item_data(ctx,"SemperSourceData");

    if(sid==NULL)
    {
        return(0);
    }
    unsigned char *r=script_param_retrieve(sid,(unsigned char*)s,NULL,XPANDER_REQUESTOR_SOURCE);

    if(r)
    {
        duk_push_string(ctx,r);
        return(1);
    }
    return(0);
}

static duk_ret_t js_engine_source_enable(duk_context *ctx)
{
    script_item_data *sid=js_engine_item_data(ctx,"SemperSourceData");

    if(sid==NULL)
    {
        return(0);
    }
    script_set_param(sid,"Disabled","0");
    return(0);
}

static duk_ret_t js_engine_source_disable(duk_context *ctx)
{
    script_item_data *sid=js_engine_item_data(ctx,"SemperSourceData");

    if(sid==NULL)
    {
        return(0);
    }
    script_set_param(sid,"Disabled","1");
    return(0);
}

static duk_ret_t js_engine_source_get_dbl(duk_context *ctx)
{
    script_item_data *sid=js_engine_item_data(ctx,"SemperSourceData");

    if(sid)
    {
        double *d=(double*)script_source_param(sid,1);
        if(d)
        {
            duk_push_number(ctx,*d);
            return(1);
        }
    }
    return(0);
}


static duk_ret_t js_engine_source_get_rel(duk_context *ctx)
{
    script_item_data *sid=js_engine_item_data(ctx,"SemperSourceData");

    if(sid)
    {
        double *d=(double*)script_source_param(sid,6);
        if(d)
        {
            duk_push_number(ctx,*d);
            return(1);
        }
    }
    return(0);
}

static duk_ret_t js_engine_source_get_min(duk_context *ctx)
{
    script_item_data *sid=js_engine_item_data(ctx,"SemperSourceData");

    if(sid)
    {
        double *d=(double*)script_source_param(sid,4);
        if(d)
        {
            duk_push_number(ctx,*d);
            return(1);
        }
    }
    return(0);
}


static duk_ret_t js_engine_source_get_max(duk_context *ctx)
{
    script_item_data *sid=js_engine_item_data(ctx,"SemperSourceData");

    if(sid)
    {
        double *d=(double*)script_source_param(sid,3);
        if(d)
        {
            duk_push_number(ctx,*d);
            return(1);
        }
    }
    return(0);
}

static duk_ret_t js_engine_source_get_range(duk_context *ctx)
{
    script_item_data *sid=js_engine_item_data(ctx,"SemperSourceData");

    if(sid)
    {
        double *d=(double*)script_source_param(sid,5);
        if(d)
        {
            duk_push_number(ctx,*d);
            return(1);
        }
    }
    return(0);
}

static duk_ret_t js_engine_source_get_str(duk_context *ctx)
{
    script_item_data *sid=js_engine_item_data(ctx,"SemperSourceData");

    if(sid)
    {
        unsigned char **s=(unsigned char**)script_source_param(sid,2);
        if(s&&*s)
        {
            duk_push_string(ctx,*s);
            return(1);
        }
    }
    return(0);
}

/*
 *******************
 * Root routines   *
 *******************
 */

static duk_ret_t js_engine_xpand(duk_context *ctx)
{
    unsigned char *s=(unsigned char*)duk_to_string(ctx,-1);

    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "SemperInternalPointer");
    void *pv=duk_get_pointer(ctx,-1);
    duk_pop_n(ctx,2);

    if(pv==NULL||s==NULL)
    {
        return(0);
    }
    unsigned char *rs=script_xpand(pv,(unsigned char*)s);

    if(rs)
    {
        duk_push_string(ctx,rs);
        return(1);
    }
    return(0);
}

static duk_ret_t js_engine_variable(duk_context *ctx)
{
    unsigned char *s=(unsigned char*)duk_to_string(ctx,-1);

    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "SemperInternalPointer");
    void *pv=duk_get_pointer(ctx,-1);
    duk_pop_n(ctx,2);

    if(pv==NULL||s==NULL)
    {
        return(0);
    }
    unsigned char *rs=script_variable(pv,(unsigned char*)s);

    if(rs)
    {
        duk_push_string(ctx,rs);
        return(1);
    }
    return(0);
}

static duk_ret_t js_engine_parse_formula(duk_context *ctx)
{
    unsigned char *s=(unsigned char*)duk_to_string(ctx,-1);

    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "SemperInternalPointer");
    void *pv=duk_get_pointer(ctx,-1);
    duk_pop_n(ctx,2);

    if(pv==NULL||s==NULL)
    {
        return(0);
    }
    
    double d=0.0;
   
    if(math_parser(s,&d,NULL,NULL)==0)
    {
        duk_push_number(ctx,d);
        return(1);
    }
    return(0);
}

static duk_ret_t js_engine_command(duk_context *ctx)
{
    unsigned char *s=(unsigned char*)duk_to_string(ctx,-1);
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "SemperInternalPointer");
    void *pv=duk_get_pointer(ctx,-1);
    duk_pop_n(ctx,2);

    if(pv==NULL||s==NULL)
    {
        return(0);
    }
    if(s)
    {
        script_send_command(pv,(unsigned char*)s);
        return(1);
    }
    return(0);
}

/*
 **************************
 * Instantiation routines *
 **************************
 */

static duk_ret_t js_engine_create_object(duk_context *ctx)
{
    static const duk_function_list_entry obj_funcs[] =
    {
        { "get_name",      js_engine_object_get_name,        0 },
        { "get_x",         js_engine_object_get_x,           0 },
        { "get_y",         js_engine_object_get_y,           0 },
        { "get_w",         js_engine_object_get_w,           0 },
        { "get_h",         js_engine_object_get_h,           0 },
        { "set_x",         js_engine_object_set_x,           1 },
        { "set_y",         js_engine_object_set_y,           1 },
        { "set_w",         js_engine_object_set_w,           1 },
        { "set_h",         js_engine_object_set_h,           1 },
        { "param",         js_engine_object_param,           1 },
        { "show",          js_engine_object_show,            0 },
        { "hide",          js_engine_object_hide,            0 },
        { NULL,            NULL,                             0 }
    };

    unsigned char *s=(unsigned char*)duk_to_string(ctx,-1);
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "SemperInternalPointer");
    void *pv=duk_get_pointer(ctx,-1);
    duk_pop_n(ctx,2);

    duk_idx_t idx=duk_push_object(ctx);
    duk_put_function_list(ctx, idx, obj_funcs);
    script_item_data *sid=duk_push_fixed_buffer(ctx,sizeof(script_item_data));
    duk_put_prop_string(ctx,idx,"SemperObjectData");
    strncpy(sid->name,s,1023);
    sid->type=1;
    sid->pv=pv;

    return 1;
}

static duk_ret_t js_engine_create_source(duk_context *ctx)
{
    static const duk_function_list_entry obj_funcs[] =
    {
        { "get_str",                js_engine_source_get_str,    0 },
        { "get_num",                js_engine_source_get_dbl,    0 },
        { "get_max",                js_engine_source_get_max,    0 },
        { "get_min",                js_engine_source_get_min,    0 },
        { "get_rel",                js_engine_source_get_rel,    0 },
        { "get_range",              js_engine_source_get_range,  0 },
        { "enable",                 js_engine_source_enable,     0 },
        { "disable",                js_engine_source_disable,    0 },
        { "name",                   js_engine_source_get_name,   0 },
        { "param",                  js_engine_source_param,      1 },
        { NULL,                     NULL,                        0 }
    };

    unsigned char *s=(unsigned char*)duk_to_string(ctx,-1);
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "SemperInternalPointer");
    void *pv=duk_get_pointer(ctx,-1);
    duk_pop_n(ctx,2);

    duk_idx_t idx=duk_push_object(ctx);  /* -> [ ... global obj ] */
    duk_put_function_list(ctx, idx, obj_funcs);
    script_item_data *sid=duk_push_fixed_buffer(ctx,sizeof(script_item_data));
    duk_put_prop_string(ctx,idx,"SemperSourceData");
    strncpy(sid->name,s,1023);
    sid->type=2;
    sid->pv=pv;

    return 1;
}

static duk_ret_t js_engine_create_self_source(duk_context *ctx)
{
    static const duk_function_list_entry obj_funcs[] =
    {
        { "get_str",                js_engine_source_get_str,    0 },
        { "get_num",                js_engine_source_get_dbl,    0 },
        { "get_max",                js_engine_source_get_max,    0 },
        { "get_min",                js_engine_source_get_min,    0 },
        { "get_rel",                js_engine_source_get_rel,    0 },
        { "get_range",              js_engine_source_get_range,  0 },
        { "enable",                 js_engine_source_enable,     0 },
        { "disable",                js_engine_source_disable,    0 },
        { "name",                   js_engine_source_get_name,   0 },
        { "param",                  js_engine_source_param,      1 },
        { NULL,                     NULL,                        0 }
    };

    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "SemperInternalPointer");
    void *pv=duk_get_pointer(ctx,-1);
    duk_pop_n(ctx,1);

    duk_idx_t idx=duk_push_object(ctx);  /* -> [ ... global obj ] */
    duk_put_function_list(ctx, idx, obj_funcs);
    script_item_data *sid=duk_push_fixed_buffer(ctx,sizeof(script_item_data));
    duk_put_prop_string(ctx,idx,"SemperSourceData");
    sid->type=2;
    sid->pv=pv;

    return 1;
}

/*
 * Like in the case of lua, this brings the magic.
 **/

static void js_engine_register(void *ctx,void *pv)
{
    const duk_function_list_entry scr_funcs[] =
    {
        { "object",         js_engine_create_object,           1 },
        { "source",         js_engine_create_source,           1 },
        { "cmd",            js_engine_command,                 1 },
        { "xpand",          js_engine_xpand,                   1 },
        { "variable",       js_engine_variable,                1 },
        { "script",         js_engine_create_self_source,      0 },
        { "formula",        js_engine_parse_formula,           1 }, 
        { NULL,             NULL,                              0 }
    };

    duk_push_global_object(ctx);
    duk_idx_t idx=duk_push_object(ctx);
    duk_push_pointer(ctx, pv);
    duk_put_prop_string(ctx, idx, "SemperInternalPointer");
    duk_put_function_list(ctx, -1, scr_funcs);
    duk_put_prop_string(ctx, -2, "semper");
    duk_pop(ctx);
}

void *js_engine_init(unsigned char *buf,void *pv)
{
    duk_context *ctx = duk_create_heap_default();
    js_engine_register(ctx,pv);

    if(duk_peval_string(ctx, buf))
    {
        duk_destroy_heap(ctx);
        return(NULL);
    }
    if(duk_get_global_string(ctx,"script_init"))
    {
        duk_pcall(ctx, 0);
    }
    duk_pop(ctx);
    return(ctx);
}


void js_engine_call_reset(duk_context *ctx)
{
    if(duk_get_global_string(ctx,"script_reset"))
    {
        duk_pcall(ctx, 0);
    }
    duk_pop(ctx);
}


double js_engine_call_update(duk_context *ctx)
{
    double ret=0.0;

    if(duk_get_global_string(ctx,"script_update"))
    {
        duk_pcall(ctx, 0);
        ret=duk_get_number(ctx,-1);
    }
    duk_pop(ctx);
    return(ret);
}

unsigned char *js_engine_call_string(duk_context *ctx)
{
    unsigned char *ret=NULL;
    if(duk_get_global_string(ctx,"script_string"))
    {
        duk_pcall(ctx, 0);
        ret=clone_string((unsigned char*)duk_safe_to_string(ctx,-1));
    }
    duk_pop(ctx);
    return(ret);
}


void js_engine_call_command(duk_context *ctx,unsigned char *comm)
{
	if(duk_get_global_string(ctx,"script_command"))
    {
        if(comm)
        {
            duk_push_string(ctx,comm);
        }
        duk_pcall(ctx, 1);
    }
    duk_pop(ctx);
}

void js_engine_cleanup(duk_context **ctx)
{
    if(ctx&&*ctx)
    {
        if(duk_get_global_string(*ctx,"script_destroy"))
        {
            duk_pcall(*ctx, 0);
        }
        duk_pop(*ctx);
        duk_destroy_heap(*ctx);
        *ctx=NULL;
    }
}
