/*
 * Parameter setters
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 * */
#include <skeleton.h>
#include <xpander.h>
#include <string_util.h>
#include <objects/object.h>
#include <surface.h>
#include <sources/source.h>
#include <mem.h>
#include <image/image_cache.h>
#include <ancestor.h>

static inline section parameter_dispatch_section(void* r, section* shead, unsigned char flag)
{
    if(r == NULL || ((flag & 0x4) && (flag & 0x8) && (flag & 0x10)))
    {
        return (NULL);
    }

    switch(flag & 0x1C)
    {
        case XPANDER_REQUESTOR_OBJECT:
        {
            object* ro = r;
            surface_data* sd = ro->sd;
            *shead = &sd->skhead;
            return (ro->os);
        }

        case XPANDER_REQUESTOR_SOURCE:
        {
            source* rs = r;
            surface_data* sd = rs->sd;
            *shead = &sd->skhead;
            return (rs->cs);
        }

        case XPANDER_REQUESTOR_SURFACE:
        {

            surface_data* sd = r;
            *shead = &sd->skhead;

            if(flag & 0x20)
            {
                *shead = &sd->cd->shead;
                return (sd->scd);
            }

            return (sd->spm);
        }

        default:
            return (NULL);
    }
}

double parameter_double(void* req, unsigned char* npm, double def, unsigned char xpander_flags)
{
    double ret = def;
    section shead = NULL;
    section s = parameter_dispatch_section(req, &shead, xpander_flags);

    if(!npm || !s)
    {
        return (ret);
    }

    xpander_request xr;
    memset(&xr, 0, sizeof(xpander_request));
    xr.req_type = xpander_flags;
    xr.requestor = req;

    key k = skeleton_get_key(s, npm);
    xr.os = skeleton_key_value(k);

    if(xr.os == NULL)
    {
        xr.os = ancestor(req, npm, xpander_flags);
    }

    if(xpander(&xr))
    {
        ret = compute_formula(xr.es);
        sfree((void**)&xr.es);
    }
    else if(xr.os)
    {
        ret = compute_formula(xr.os);
    }

    xr.os = NULL;
    return (ret);
}


size_t parameter_size_t(void* req, unsigned char* npm, size_t def, unsigned char xpander_flags)
{
    size_t ret = def;
    section shead = NULL;
    section s = parameter_dispatch_section(req, &shead, xpander_flags);

    if(!npm || !s)
    {
        return (ret);
    }

    xpander_request xr;
    memset(&xr, 0, sizeof(xpander_request));
    xr.req_type = xpander_flags;
    xr.requestor = req;

    key k = skeleton_get_key(s, npm);
    xr.os = skeleton_key_value(k);

    if(xr.os == NULL)
    {
        xr.os = ancestor(req, npm, xpander_flags);
    }

    if(xpander(&xr))
    {
        ret = (size_t)compute_formula(xr.es);
        sfree((void**)&xr.es);
    }
    else if(xr.os)
    {
        ret =(size_t) compute_formula(xr.os);
    }

    xr.os = NULL;
    return (ret);
}


long long parameter_long_long(void* req, unsigned char* npm, long long def, unsigned char xpander_flags)
{
    return ((long long)parameter_size_t(req, npm, (size_t)def, xpander_flags));
}

unsigned char parameter_byte(void* req, unsigned char* npm, unsigned char def, unsigned char xpander_flags)
{
    return ((unsigned char)parameter_size_t(req, npm, (size_t)def, xpander_flags));
}

unsigned char parameter_bool(void* req, unsigned char* npm, unsigned char def, unsigned char xpander_flags)
{
    return (parameter_size_t(req, npm, (size_t)def, xpander_flags)>0);
}

unsigned char* parameter_string(void* req, unsigned char* npm, unsigned char* def, unsigned char xpander_flags)
{
    unsigned char* ret = NULL;
    section shead = NULL;
    section s = parameter_dispatch_section(req, &shead, xpander_flags);

    if(!npm || !s)
    {
        return (clone_string(def));
    }

    xpander_request xr;
    memset(&xr, 0, sizeof(xpander_request));
    xr.req_type = xpander_flags;
    xr.requestor = req;

    key k = skeleton_get_key(s, npm);
    xr.os = skeleton_key_value(k);

    if(xr.os == NULL)
    {
        xr.os = ancestor(req, npm, xpander_flags);
    }

    if(xr.os == NULL)
    {
        return (clone_string(def));
    }

    if(xpander(&xr))
    {
        ret = xr.es;
    }
    else if(xr.os)
    {
        ret = clone_string(xr.os);
    }

    xr.os = NULL;
    return (ret);
}

unsigned int parameter_color(void* req, unsigned char* npm, unsigned int def, unsigned char xpander_flags)
{
    unsigned int ret = def;

    section shead = NULL;
    section s = parameter_dispatch_section(req, &shead, xpander_flags);

    if(!npm || !s)
    {
        return (def);
    }

    xpander_request xr;
    memset(&xr, 0, sizeof(xpander_request));
    xr.req_type = xpander_flags;
    xr.requestor = req;

    key k = skeleton_get_key(s, npm);
    xr.os = skeleton_key_value(k);

    if(xr.os == NULL)
    {
        xr.os = ancestor(req, npm, xpander_flags);
    }

    if(xpander(&xr))
    {
        ret = string_to_color(xr.es);
        sfree((void**)&xr.es);
    }
    else if(xr.os)
    {
        ret = string_to_color(xr.os);
    }

    xr.os = NULL;
    return (ret);
}

int parameter_image_tile(void* req, unsigned char* npm, image_tile* param, unsigned char xpander_flags)
{
    int ret = 1;
    section shead = NULL;
    section s = parameter_dispatch_section(req, &shead, xpander_flags);

    if(!npm || !param || !s)
    {
        return (0);
    }

    xpander_request xr;
    memset(&xr, 0, sizeof(xpander_request));
    xr.req_type = xpander_flags;
    xr.requestor = req;

    memset(param, 0, sizeof(image_tile));
    key k = skeleton_get_key(s, npm);
    xr.os = skeleton_key_value(k);

    if(xr.os == NULL)
    {
        xr.os = ancestor(req, npm, xpander_flags);
    }

    if(xpander(&xr))
    {
        string_to_image_tile(xr.es, param);
        sfree((void**)&xr.es);
    }
    else if(xr.os)
    {
        string_to_image_tile(xr.os, param);
    }

    xr.os = NULL;
    return (ret);
}

int parameter_object_padding(void* req, unsigned char* npm, object_padding* param, unsigned char xpander_flags)
{
    int ret = -1;
    section shead = NULL;
    section s = parameter_dispatch_section(req, &shead, xpander_flags);

    if(!npm || !param || !s)
    {
        return (0);
    }

    xpander_request xr;
    memset(&xr, 0, sizeof(xpander_request));
    xr.req_type = xpander_flags;
    xr.requestor = req;

    memset(param, 0, sizeof(object_padding));
    key k = skeleton_get_key(s, npm);
    xr.os = skeleton_key_value(k);

    if(xr.os == NULL)
    {
        xr.os = ancestor(req, npm, xpander_flags);
    }

    if(xpander(&xr))
    {
        string_to_padding(xr.es, param);
        sfree((void**)&xr.es);
        ret = 0;
    }
    else if(xr.os)
    {
        string_to_padding(xr.os, param);
        ret = 0;
    }

    xr.os = NULL;
    return (ret);
}

int parameter_image_crop(void* req, unsigned char* npm, image_crop* param, unsigned char xpander_flags)
{
    int ret = -1;
    section shead = NULL;
    section s = parameter_dispatch_section(req, &shead, xpander_flags);

    if(!npm || !param || !s)
    {
        return (0);
    }

    xpander_request xr;
    memset(&xr, 0, sizeof(xpander_request));
    xr.req_type = xpander_flags;
    xr.requestor = req;

    memset(param, 0, sizeof(image_crop));
    key k = skeleton_get_key(s, npm);
    xr.os = skeleton_key_value(k);

    if(xr.os == NULL)
    {
        xr.os = ancestor(req, npm, xpander_flags);
    }

    if(xpander(&xr))
    {
        string_to_image_crop(xr.es, param);
        sfree((void**)&xr.es);
        ret = 0;
    }
    else if(xr.os)
    {
        string_to_image_crop(xr.os, param);
        ret = 0;
    }

    xr.os = NULL;
    return (ret);
}

int parameter_color_matrix(void* req, unsigned char* npm, double* cm, unsigned char xpander_flags)
{
    int ret = -1;
    section shead = NULL;
    section s = parameter_dispatch_section(req, &shead, xpander_flags);

    if(!npm || !cm || !s)
    {
        return (0);
    }

    xpander_request xr;
    memset(&xr, 0, sizeof(xpander_request));
    xr.req_type = xpander_flags;
    xr.requestor = req;

    memset(cm, 0, sizeof(double) * 5);

    key k = skeleton_get_key(s, npm);
    xr.os = skeleton_key_value(k);

    if(xr.os == NULL)
    {
        xr.os = ancestor(req, npm, xpander_flags);
    }

    if(xpander(&xr))
    {
        string_to_color_matrix(xr.es, cm);
        sfree((void**)&xr.es);
        ret = 0;
    }
    else if(xr.os)
    {
        string_to_color_matrix(xr.os, cm);
        ret = 0;
    }

    xr.os = NULL;
    return (ret);
}

unsigned int parameter_self_scaling(void* req, unsigned char* npm, unsigned int def, unsigned char xpander_flags)
{
    unsigned int ret = def;
    section shead = NULL;
    section s = parameter_dispatch_section(req, &shead, xpander_flags);

    if(!npm || !s)
    {
        return (def);
    }

    xpander_request xr;
    memset(&xr, 0, sizeof(xpander_request));
    xr.req_type = xpander_flags;
    xr.requestor = req;

    key k = skeleton_get_key(s, npm);
    xr.os = skeleton_key_value(k);

    if(xr.os == NULL)
    {
        xr.os = ancestor(req, npm, xpander_flags);
    }

    if(xpander(&xr))
    {

        if(xr.es)
        {
            if(strcasecmp("1", xr.es) == 0)
            {
                ret = 1;
            }
            else if(strcasecmp("2", xr.es) == 0)
            {
                ret = 2;
            }
            else if(strcasecmp("1k", xr.es) == 0)
            {
                ret = 3;
            }
            else if(strcasecmp("2k", xr.es) == 0)
            {
                ret = 4;
            }
        }

        sfree((void**)&xr.es);
    }
    else if(xr.os)
    {
        if(strcasecmp("1", xr.os) == 0)
        {
            ret = 1;
        }
        else if(strcasecmp("2", xr.os) == 0)
        {
            ret = 2;
        }
        else if(strcasecmp("1k", xr.os) == 0)
        {
            ret = 3;
        }
        else if(strcasecmp("2k", xr.os) == 0)
        {
            ret = 4;
        }
    }

    xr.os = NULL;
    return (ret);
}
