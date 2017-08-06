/* lua script support
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */


#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <string_util.h>
#include <sources/script/utils.h>
#include <xpander.h>
#include <math_parser.h>

static int lua_engine_object_get_name(lua_State *ctx)
{
    script_item_data *sid=luaL_testudata(ctx,1,"SemperObject");

    if(sid)
    {
        void *o=script_param_retrieve(sid,"Name",NULL,0);

        if(o==NULL)
        {
            return(0);
        }

        lua_pushstring(ctx,sid->name);
        return(1);
    }
    return(0);
}

static int lua_engine_object_get_x(lua_State *ctx)
{
    script_item_data *sid=luaL_testudata(ctx,1,"SemperObject");

    if(sid)
    {
        double d= script_object_param(sid,1);
        lua_pushnumber(ctx,d);
        return(1);
    }
    return(0);
}

static int lua_engine_object_get_y(lua_State *ctx)
{
    script_item_data *sid=luaL_testudata(ctx,1,"SemperObject");

    if(sid)
    {
        double d= script_object_param(sid,2);
        lua_pushnumber(ctx,d);
        return(1);
    }
    return(0);
}

static int lua_engine_object_get_w(lua_State *ctx)
{
    script_item_data *sid=luaL_testudata(ctx,1,"SemperObject");

    if(sid)
    {
        double d= script_object_param(sid,3);
        lua_pushnumber(ctx,d);
        return(1);
    }
    return(0);
}

static int lua_engine_object_get_h(lua_State *ctx)
{
    script_item_data *sid=luaL_testudata(ctx,1,"SemperObject");

    if(sid)
    {
        double d= script_object_param(sid,4);
        lua_pushnumber(ctx,d);
        return(1);
    }
    return(0);
}

static int lua_engine_object_set_x(lua_State *ctx)
{

    double d=luaL_checknumber(ctx,-1);
    script_item_data *sid=luaL_testudata(ctx,1,"SemperObject");

    if(sid==NULL)
    {
        return(0);
    }
    unsigned char buf[64]= {0};
    snprintf(buf,64,"%lf",d);
    script_set_param(sid,"X",buf);

    return(0);
}

static int lua_engine_object_set_y(lua_State *ctx)
{

    double d=luaL_checknumber(ctx,-1);
    script_item_data *sid=luaL_testudata(ctx,1,"SemperObject");

    if(sid==NULL)
        return(0);
    unsigned char buf[64]= {0};
    snprintf(buf,64,"%lf",d);
    script_set_param(sid,"Y",buf);
    return(0);
}

static int lua_engine_object_set_h(lua_State *ctx)
{
    double d=luaL_checknumber(ctx,-1);
    script_item_data *sid=luaL_testudata(ctx,1,"SemperObject");

    if(sid==NULL)
    {
        return(0);
    }

    unsigned char buf[64]= {0};
    snprintf(buf,64,"%lf",d);
    script_set_param(sid,"H",buf);
    return(0);
}

static int lua_engine_object_show(lua_State *ctx)
{
    script_item_data *sid=luaL_testudata(ctx,1,"SemperObject");

    if(sid==NULL)
    {
        return(0);
    }
    script_set_param(sid,"Hidden","0");
    return(0);
}

static int lua_engine_object_hide(lua_State *ctx)
{
    script_item_data *sid=luaL_testudata(ctx,1,"SemperObject");

    if(sid==NULL)
    {
        return(0);
    }
    script_set_param(sid,"Hidden","1");
    return(0);

}

static int lua_engine_object_set_w(lua_State *ctx)
{
    double d=luaL_checknumber(ctx,-1);
    script_item_data *sid=luaL_testudata(ctx,1,"SemperObject");

    if(sid==NULL)
    {
        return(0);
    }
    unsigned char buf[64]= {0};
    snprintf(buf,64,"%lf",d);
    script_set_param(sid,"W",buf);
    return(0);

}

static int lua_engine_object_param(lua_State *ctx)
{
    const unsigned char *s=luaL_checkstring(ctx,-1);
    script_item_data *sid=luaL_testudata(ctx,1,"SemperObject");

    if(sid==NULL)
    {
        return(0);
    }
    unsigned char *r=script_param_retrieve(sid,(unsigned char*)s,NULL,XPANDER_REQUESTOR_OBJECT);

    if(r)
    {
        lua_pushstring(ctx,r);
        return(1);
    }
    return(0);
}

/*
 ********************************
 * Source API for LUA           *
 ********************************
 */

static int lua_engine_source_get_name(lua_State *ctx)
{
    script_item_data *sid=luaL_testudata(ctx,1,"SemperSource");

    if(sid)
    {
        void *o=script_param_retrieve(sid,"Name",NULL,0);

        if(o==NULL)
        {
            return(0);
        }

        lua_pushstring(ctx,o);
        return(1);
    }
    return(0);
}

static int lua_engine_source_param(lua_State *ctx)
{
    const unsigned char *s=luaL_checkstring(ctx,-1);
    script_item_data *sid=luaL_testudata(ctx,1,"SemperSource");

    if(sid==NULL)
    {
        return(0);
    }
    unsigned char *r=script_param_retrieve(sid,(unsigned char*)s,NULL,XPANDER_REQUESTOR_SOURCE);

    if(r)
    {
        lua_pushstring(ctx,r);
        return(1);
    }
    return(0);
}

static int lua_engine_source_enable(lua_State *ctx)
{
    script_item_data *sid=luaL_testudata(ctx,1,"SemperSource");

    if(sid==NULL)
    {
        return(0);
    }
    script_set_param(sid,"Disabled","0");
    return(0);
}

static int lua_engine_source_disable(lua_State *ctx)
{
    script_item_data *sid=luaL_testudata(ctx,1,"SemperSource");

    if(sid==NULL)
    {
        return(0);
    }
    script_set_param(sid,"Disabled","1");
    return(0);
}

static int lua_engine_source_get_dbl(lua_State *ctx)
{
    script_item_data *sid=luaL_testudata(ctx,1,"SemperSource");

    if(sid)
    {
        double *d=(double*)script_source_param(sid,1);

        if(d)
        {
            lua_pushnumber(ctx,*d);
            return(1);
        }
    }
    return(0);
}

static int lua_engine_source_get_rel(lua_State *ctx)
{
    script_item_data *sid=luaL_testudata(ctx,1,"SemperSource");

    if(sid)
    {
        double *d=(double*)script_source_param(sid,6);
        if(d)
        {
            lua_pushnumber(ctx,*d);
            return(1);
        }
    }
    return(0);
}

static int lua_engine_source_get_min(lua_State *ctx)
{
    script_item_data *sid=luaL_testudata(ctx,1,"SemperSource");

    if(sid)
    {
        double *d=(double*)script_source_param(sid,4);
        if(d)
        {
            lua_pushnumber(ctx,*d);
            return(1);
        }
    }
    return(0);
}

static int lua_engine_source_get_max(lua_State *ctx)
{
    script_item_data *sid=luaL_testudata(ctx,1,"SemperSource");

    if(sid)
    {
        double *d=(double*)script_source_param(sid,3);
        if(d)
        {
            lua_pushnumber(ctx,*d);
            return(1);
        }
    }
    return(0);
}

static int lua_engine_source_get_range(lua_State *ctx)
{
    script_item_data *sid=luaL_testudata(ctx,1,"SemperSource");

    if(sid)
    {
        double *d=(double*)script_source_param(sid,5);
        if(d)
        {
            lua_pushnumber(ctx,*d);
            return(1);
        }
    }
    return(0);
}

static int lua_engine_source_get_str(lua_State *ctx)
{
    script_item_data *sid=luaL_testudata(ctx,1,"SemperSource");

    if(sid)
    {
        unsigned char **s=(unsigned char**)script_source_param(sid,2);
        if(s&&*s)
        {
            lua_pushstring(ctx,*s);
            return(1);
        }
    }
    return(0);
}

static int lua_engine_command(lua_State *ctx)
{
    const unsigned char *s=luaL_checkstring(ctx,-1);
    void *ip=luaL_testudata (ctx,1,"SemperScript");

    if(ip==NULL||s==NULL)
    {
        return(0);
    }
    script_send_command(ip,(unsigned char*)s);
    return(0);
}

static int lua_engine_xpand(lua_State *ctx)
{
    const unsigned char *s=luaL_checkstring(ctx,-1);
    void *ip=luaL_testudata (ctx,1,"SemperScript");

    if(ip==NULL||s==NULL)
    {
        return(0);
    }
    unsigned char *rs=script_xpand(ip,(unsigned char*)s);

    if(rs)
    {
        lua_pushstring(ctx,rs);
        return(1);
    }
    return(0);
}

static int lua_engine_parse_formula(lua_State *ctx)
{
    const unsigned char *s=luaL_checkstring(ctx,-1);
    void *ip=luaL_testudata (ctx,1,"SemperScript");

    if(ip==NULL||s==NULL)
    {
        return(0);
    }
   
    double d=0.0;

    if(math_parser((unsigned char*)s,&d,NULL,NULL)==0)
    {
        lua_pushnumber(ctx,d);
        return(1);
    }
    return(0);
}


static int lua_engine_variable(lua_State *ctx)
{
    const unsigned char *s=luaL_checkstring(ctx,-1);
    void *ip=luaL_testudata (ctx,1,"SemperScript");

    if(ip==NULL||s==NULL)
    {
        return(0);
    }
    unsigned char *rs=script_variable(ip,(unsigned char*)s);

    if(rs)
    {
        lua_pushstring(ctx,rs);
        return(1);
    }
    return(0);
}

/*
 ********************************************
 * Instantiation routines                   *
 ********************************************
 */
 
static int lua_engine_object(lua_State *ctx)
{
    const unsigned char *s=luaL_checkstring(ctx,-1);
    void *ip=luaL_testudata (ctx,1,"SemperScript");

    if(s==NULL||ip==NULL)
    {
        return(0);
    }

    script_item_data *sid=lua_newuserdata(ctx,sizeof(script_item_data));

    if(sid==NULL)
    {
        return(0);
    }
    memset(sid,0,sizeof(script_item_data)); //we do not want garbge in our structure
    strncpy(sid->name,s,1023);
    sid->pv=ip;
    sid->type=1;
    luaL_setmetatable(ctx, "SemperObject");
    return(1);
}

static int lua_engine_source(lua_State *ctx)
{
    const unsigned char *s=luaL_checkstring(ctx,-1);
    void *ip=luaL_testudata (ctx,1,"SemperScript");

    if(s==NULL||ip==NULL)
    {
        return(0);
    }

    script_item_data *sid=lua_newuserdata(ctx,sizeof(script_item_data));

    if(sid==NULL)
    {
        return(0);
    }
    memset(sid,0,sizeof(script_item_data)); //we do not want garbge in our structure
    strncpy(sid->name,s,1023);
    sid->pv=ip;
    sid->type=2;
    luaL_setmetatable(ctx, "SemperSource");
    return(1);
}

static int lua_engine_self_source(lua_State *ctx)
{
    const unsigned char *s=luaL_checkstring(ctx,-1);
    void *ip=luaL_testudata (ctx,1,"SemperScript");

    if(s==NULL||ip==NULL)
    {
        return(0);
    }

    script_item_data *sid=lua_newuserdata(ctx,sizeof(script_item_data));

    if(sid==NULL)
    {
        return(0);
    }
    memset(sid,0,sizeof(script_item_data)); //we do not want garbge in our structure
    sid->pv=ip;
    sid->type=2;
    luaL_setmetatable(ctx, "SemperSource");
    return(1);
}

static int lua_engine_surface(lua_State *ctx)
{
    const unsigned char *s=luaL_checkstring(ctx,-1);
    void *ip=luaL_testudata (ctx,1,"SemperScript");

    if(s==NULL||ip==NULL)
    {
        return(0);
    }

    script_item_data *sid=lua_newuserdata(ctx,sizeof(script_item_data));

    if(sid==NULL)
    {
        return(0);
    }
    memset(sid,0,sizeof(script_item_data)); //we do not want garbge in our structure
    strncpy(sid->name,s,1023);
    sid->pv=ip;
    sid->type=2;
    luaL_setmetatable(ctx, "SemperSource");
    return(1);
}

/*These routines do...what am I saying?...This is pure magic*/
static void lua_engine_register(void *ctx,void *pv)
{
    const luaL_Reg gen_rtn[]=
    {
        {"object",                        lua_engine_object },
        {"source",                        lua_engine_source },
        {"surface",                      lua_engine_surface },
        {"cmd",                          lua_engine_command },
        {"xpand",                          lua_engine_xpand },
        {"variable",                    lua_engine_variable },
        {"script",                   lua_engine_self_source },
        {"formula",                lua_engine_parse_formula },
        { NULL,                                        NULL }
    };

    const luaL_Reg obj[]=
    {
        { "get_x",                  lua_engine_object_get_x },
        { "get_y",                  lua_engine_object_get_y },
        { "get_w",                  lua_engine_object_get_w },
        { "get_h",                  lua_engine_object_get_h },
        { "get_name",            lua_engine_object_get_name },
        { "set_x",                  lua_engine_object_set_x },
        { "set_y",                  lua_engine_object_set_y },
        { "set_w",                  lua_engine_object_set_w },
        { "set_h",                  lua_engine_object_set_h },
        { "param",                  lua_engine_object_param },
        { "hide",                    lua_engine_object_hide },
        { "show",                    lua_engine_object_show },
        { NULL,                                        NULL }
    };


    const luaL_Reg srf[]=
    {
        {NULL,                                         NULL }
    };

    const luaL_Reg src[]=
    {
        { "get_str",                  lua_engine_source_get_str },
        { "get_num",                  lua_engine_source_get_dbl },
        { "get_max",                  lua_engine_source_get_max },
        { "get_min",                  lua_engine_source_get_min },
        { "get_rel",                  lua_engine_source_get_rel },
        { "get_range",              lua_engine_source_get_range },
        { "enable",                    lua_engine_source_enable },
        { "disable",                  lua_engine_source_disable },
        { "name",                    lua_engine_source_get_name },
        { "param",                      lua_engine_source_param },
        { NULL,                                            NULL }
    };

    /*register tables*/
    luaL_newmetatable(ctx,"SemperObject");
    luaL_setfuncs(ctx,obj,0);
    lua_pushvalue(ctx, -1);
    lua_setfield(ctx, -1, "__index");


    luaL_newmetatable(ctx,"SemperSource");
    luaL_setfuncs(ctx,src,0);
    lua_pushvalue(ctx, -1);
    lua_setfield(ctx, -1, "__index");


    luaL_newmetatable(ctx,"SemperSurface");
    luaL_setfuncs(ctx,srf,0);
    lua_pushvalue(ctx, -1);
    lua_setfield(ctx, -1, "__index");


    luaL_newmetatable(ctx,"SemperScript");
    luaL_setfuncs(ctx,gen_rtn,0);
    lua_pushvalue(ctx, -1);
    lua_setfield(ctx, -1, "__index");
    lua_pushlightuserdata(ctx,pv); //this will point to our internal structures
    luaL_setmetatable(ctx, "SemperScript");

}

void *lua_engine_init(unsigned char *scbuf,void *ip)
{
    if(scbuf==NULL)
    {
        return(NULL);
    }
    void *ctx=luaL_newstate();

    if(ctx)
    {
        luaL_openlibs(ctx);
        lua_engine_register(ctx,ip);
        if(luaL_dostring(ctx,scbuf))
        {
            lua_close(ctx);
            ctx=NULL;
        }
        else
        {
            lua_setglobal(ctx,"semper");

            if(lua_getglobal(ctx,"script_init")==LUA_TFUNCTION)
            {
                lua_pcall(ctx,0,0,0);
            }
        }
    }
    return(ctx);
}

void lua_engine_call_reset(void *ctx)
{
    if(ctx!=NULL&&lua_getglobal(ctx,"script_reset")==LUA_TFUNCTION)
    {
        lua_pcall(ctx,0,0,0);
    }
}

double lua_engine_call_update(void *ctx)
{
    double r=0.0;

    if(ctx)
    {
        if(lua_getglobal(ctx,"script_update")==LUA_TFUNCTION    &&
                !lua_pcall(ctx,0,1,0)   &&
                lua_isnumber(ctx, -1))
        {
            r=lua_tonumber(ctx,-1);
        }

        lua_pop(ctx, 1);
    }
    return(r);
}

unsigned char *lua_engine_call_string(void *ctx)
{
    unsigned char *sr=NULL;

    if(ctx)
    {
        if(lua_getglobal(ctx,"script_string")==LUA_TFUNCTION&&!lua_pcall(ctx,0,1,0)&&lua_isstring(ctx, -1))
        {
            sr=clone_string((unsigned char*)lua_tostring(ctx,-1));
        }
        lua_pop(ctx, 1);
    }
    return(sr);
}

void lua_engine_call_command(void *ctx,unsigned char *comm)
{
    if(ctx)
    {
        if(lua_getglobal(ctx,"script_command")==LUA_TFUNCTION)
        {
            lua_pushstring(ctx,comm);
            lua_pcall(ctx,1,0,0);
        }
    }
}

void lua_engine_cleanup(void **ctx)
{
    lua_close(*ctx);
    *ctx=NULL;
}
