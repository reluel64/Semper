/*
 * Variable (e)xpander
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 * */

#include <surface.h>
#include <semper.h>
#include <string_util.h>
#include <mem.h>
#include <sources/source.h>
#include <xpander.h>
#include <objects/object.h>
#include <sources/source.h>
#include <bind.h>
#ifdef __linux__
#include <ctype.h>
#endif

#define XPANDER_MODIFIER_SCALE    (1<<0)
#define XPANDER_MODIFIER_PERCENT  (1<<1)
#define XPANDER_MODIFIER_DECIMALS (1<<2)
#define XPANDER_MODIFIER_MAX_MIN  (1<<3)
#define XPANDER_SOURCE_RTN_PARAMETER_STACK 1024


typedef struct
{
    unsigned char *vn;
    void *vv;
    size_t (*conv_func)(void *d, unsigned char **out);
} xpander_table;

typedef struct
{
    unsigned char *buffer;
    unsigned char *out_buf;
    surface_data *sd;
    size_t start;
    size_t end;
    size_t marker;
    size_t out_len;
} xpander_section_variable_info;

typedef struct
{
    section sect;
    surface_data *sd;
    control_data *cd;
    unsigned char *buf_out;
    unsigned char escapements;
    string_tokenizer_info sti;
} xpander_data;


typedef struct
{
    size_t op;
    size_t quotes;
    unsigned char quote_type;
} xpander_source_tokenizer_status;


static size_t xpander_convert_long(void *in, unsigned char **out)
{
    long val = *((long*)in);
    long cval = labs(val);
    size_t len = 0;

    for(; cval >= (long)pow(10, (double)len); len++);

    len += (val <= 0 ? 1 : 0);


    *out = zmalloc(len + 1);
    snprintf(*out, len + 1, "%ld", val);

    return(len);
}
#if 0
static size_t xpander_convert_double(void *in, unsigned char **out)
{
    double val = *((double*)in);
    double cval = fabs(val);
    size_t len = 0;

    for(; cval >= pow(10, (double)len); len++);

    len += (val <= 0.0 ? 1 : 0);

    *out = zmalloc(len + 1);
    snprintf(*out, len + 1, "%lf", val);

    return(len);
}
#endif

static int xpander_token_section_variables(string_tokenizer_status *sts, void *pv)
{
    if(sts->buf[sts->pos] == ',')
    {
        return(1);
    }

    return(0);
}

/*Adapted from command.c*/
static int xpander_source_func_filter(string_tokenizer_status *pi, void* pv)
{
    xpander_source_tokenizer_status* xsts = pv;

    if(pi->reset)
    {
        memset(xsts, 0, sizeof(xpander_source_tokenizer_status));
        return(0);
    }

    if(xsts->quote_type == 0 && (pi->buf[pi->pos] == '"' || pi->buf[pi->pos] == '\''))
        xsts->quote_type = pi->buf[pi->pos];

    if(pi->buf[pi->pos] == xsts->quote_type)
        xsts->quotes++;

    if(xsts->quotes % 2)
        return(0);
    else
        xsts->quote_type = 0;


    if(pi->buf[pi->pos] == '(')
        if(++xsts->op == 1)
            return(1);


    if(pi->buf[pi->pos] == ';')
        return (xsts->op == 0);

    if(xsts->op == 1 && pi->buf[pi->pos] == ',')
        return (1);

    if(xsts->op && pi->buf[pi->pos] == ')')
          if(--xsts->op == 0)
              return(0);

    return (0);
}



static unsigned char *xpander_call_source_string(source *s, unsigned char *sub)
{
    unsigned char push_params = 0;
    unsigned char *ret_str = NULL;
    unsigned char execute = 0;
    unsigned char stack_pos = 0;
    unsigned char *func_name = NULL;
    unsigned char **pms = NULL;


    xpander_source_tokenizer_status xsts = { 0 };
    string_tokenizer_info    sti =
    {
        .buffer                  = sub, //store the string address here
        .filter_data             = &xsts,
        .string_tokenizer_filter = xpander_source_func_filter,
        .ovecoff                 = NULL,
        .oveclen                 = 0
    };


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

            if(sti.buffer[end - 1] == ')'&&(sti.buffer[end]!=','))
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

        if(push_params && stack_pos < XPANDER_SOURCE_RTN_PARAMETER_STACK)
        {
            if((execute && end != start) || execute == 0)
            {
                unsigned char **tmp = realloc(pms, sizeof(unsigned char*) * (stack_pos + 1));

                if(tmp)
                {
                    pms = tmp;
                    pms[stack_pos] = zmalloc((end - start) + 1);
                    strncpy(pms[stack_pos], sti.buffer + start, end - start);
                    stack_pos++;
                }
            }
        }
        else if(func_name == NULL)
        {
            func_name = zmalloc((end - start) + 1);
            strncpy(func_name, sti.buffer + start, end - start);
        }

        if(execute)
        {
            ret_str = source_call_str_rtn(s, func_name, pms, stack_pos);
            break;
        }
    }

    for(size_t i = 0; i < stack_pos; i++)
    {
        sfree((void**)&pms[i]);
    }

    sfree((void**)&pms);
    sfree((void**)&func_name);
    sfree((void**)&sti.ovecoff);

    return (ret_str);
}

static int xpander_section_variables(xpander_section_variable_info *xsvi)
{

    int ret = -1;

    xsvi->out_len = 0;
    xsvi->out_buf = NULL;
    unsigned char *req_var_start = xsvi->buffer + xsvi->marker + 1;
    size_t req_var_len = xsvi->end - xsvi->marker - 1;
    source *s = NULL;
    object *o = NULL;

    o = (object_by_name(xsvi->sd, xsvi->buffer + xsvi->start, xsvi->marker - xsvi->start));
    s = (o == NULL ? source_by_name(xsvi->sd, xsvi->buffer + xsvi->start, xsvi->marker - xsvi->start) : NULL);

    if(o)
    {
        long w = (((o->w != o->auto_w || o->w < 0) && o->auto_w != 0) ? o->auto_w : o->w);
        long h = (((o->h != o->auto_h || o->h < 0) && o->auto_h != 0) ? o->auto_h : o->h);

        if(strncasecmp("Source", req_var_start, 6) == 0)
        {
            size_t index = 0;
            unsigned char val = 0;

            if(strncasecmp("Value", req_var_start + 6, 5) == 0)
            {
                req_var_start += 5;
                val = 1;
            }

            sscanf(req_var_start + 6, "%llu", &index);
            unsigned char *sn = bind_source_name(o, index);

            if(sn)
            {
                if(val == 0)
                {
                    xsvi->out_buf = clone_string(sn);
                    xsvi->out_len = string_length(sn);
                    ret = 0;
                }
                else if(val)
                {
                    xsvi->out_buf = clone_string(source_variable(source_by_name(xsvi->sd, sn, -1), &xsvi->out_len, SOURCE_VARIABLE_EXPAND));
                    ret = 0;
                }
            }
        }
        else if(strncasecmp("X", req_var_start, 1) == 0)
        {
            xsvi->out_len = xpander_convert_long(&o->x, &xsvi->out_buf);
            ret = 0;
        }
        else if(strncasecmp("Y", req_var_start, 1) == 0)
        {
            xsvi->out_len = xpander_convert_long(&o->y, &xsvi->out_buf);
            ret = 0;
        }
        else if(strncasecmp("W", req_var_start, 1) == 0)
        {
            xsvi->out_len = xpander_convert_long(&w, &xsvi->out_buf);
            ret = 0;
        }
        else if(strncasecmp("H", req_var_start, 1) == 0)
        {
            xsvi->out_len = xpander_convert_long(&h, &xsvi->out_buf);
            ret = 0;
        }
    }
    else if(s)
    {
        unsigned char error = 0;
        unsigned char mode = XPANDER_MODIFIER_MAX_MIN  |
                             XPANDER_MODIFIER_DECIMALS |
                             XPANDER_MODIFIER_PERCENT  |
                             XPANDER_MODIFIER_SCALE;

        double value = s->d_info;
        double scale = 1.0;
        int decimals = 0;
        string_tokenizer_info sti =
        {
            .buffer = req_var_start,
            .oveclen = 0,
            .ovecoff = NULL,
            .filter_data = NULL,
            .string_tokenizer_filter = xpander_token_section_variables
        };

        string_tokenizer(&sti);

        size_t *ovec = 0;
        size_t ovecl = 0;
        ovec = sti.ovecoff;
        ovecl = sti.oveclen;

        for(size_t i = 0; i < ovecl / 2; i++)
        {
            size_t start = ovec[2 * i];
            size_t end = ovec[2 * i + 1];
            unsigned char *buf = NULL;

            if(sti.buffer[start] == ',')
            {
                start++;
            }

            if(sti.buffer[end - 1] == ']')
            {
                end--;
            }

            if(string_strip_space_offsets(sti.buffer, &start, &end) == 0)
            {
                if(sti.buffer[start] == '"' || sti.buffer[start] == '\'')
                    start++;

                if(sti.buffer[end - 1] == '"' || sti.buffer[end - 1] == '\'')
                    end--;
            }

            buf = sti.buffer + start;

            if((mode & XPANDER_MODIFIER_MAX_MIN) && strncasecmp("MaxValue", buf, 8) == 0)
            {
                mode &= ~(XPANDER_MODIFIER_MAX_MIN | XPANDER_MODIFIER_PERCENT);
                value = s->max_val;
            }

            else if((mode & XPANDER_MODIFIER_MAX_MIN) && strncasecmp("MinValue", buf, 8) == 0)
            {
                mode &= ~(XPANDER_MODIFIER_MAX_MIN | XPANDER_MODIFIER_PERCENT);
                value = s->min_val;
            }
            else if((mode & XPANDER_MODIFIER_SCALE) && buf[0] == '/')
            {
                mode &= ~XPANDER_MODIFIER_SCALE;
                unsigned char *res = NULL;
                scale = strtod(buf + 1, (char**)&res);

                if(res == buf)
                    error = 1;
            }
            else if((mode & XPANDER_MODIFIER_PERCENT) && buf[0] == '%')
            {
                mode &= ~(XPANDER_MODIFIER_MAX_MIN | XPANDER_MODIFIER_PERCENT);
                value = bind_percentual_value(s->d_info, s->min_val, s->max_val);
            }
            else if((mode & XPANDER_MODIFIER_DECIMALS))
            {
                unsigned char *res = NULL;
                decimals = (int)strtod(buf, (char**)&res);

                if(res == buf)
                    error = 1;
                else
                    mode &= ~XPANDER_MODIFIER_DECIMALS;
            }
            else
                error = 1;
        }

        if(!error)
        {
            int len = 0;
            char fmt[33] = {0};
            value = value / (scale == 0.0 ? 1.0 : scale);
            snprintf(fmt, 33, "%%.%ulf", decimals);
            xsvi->out_buf = zmalloc(33);

            if((len = snprintf(xsvi->out_buf, 33, fmt, value)) < 0)
            {
                sfree((void**)&xsvi->out_buf);
                xsvi->out_len = 0;
            }
            else
            {
                xsvi->out_len = len;
                ret = 0;
            }
        }
        else /*let's do a expensive call*/
        {
            unsigned char *tmp = zmalloc(req_var_len + 1);

            if(tmp)
            {
                strncpy(tmp, req_var_start, req_var_len);
                unsigned char *var = xpander_call_source_string(s, tmp);
                sfree((void**)&tmp);

                if(var)
                {
                    xsvi->out_buf = clone_string(var);
                    xsvi->out_len = string_length(var);
                    ret = 0;
                }
            }
        }
    }

    return(ret);
}


static int xpander_var_token_filter(string_tokenizer_status *sts, void *pv)
{
    if(sts->buf[sts->pos] == '$')
    {
        return(1);
    }

    return(0);
}


static int xpander_src_token_filter(string_tokenizer_status *sts, void *pv)
{
    size_t *sym = pv;

    if(sts->buf[sts->pos] == '[')
    {
        if((*sym) == 0)
        {
            (*sym)++;
            return(1);
        }
    }
    else if(sts->buf[sts->pos] == ']')
    {
        if((*sym))
        {
            (*sym)--;
            return(1);
        }
    }

    return(0);
}


static size_t xpander_index_variable(unsigned char *in, size_t in_len, unsigned char **out, void *pv)
{
    size_t vindex = 0;
    size_t out_len = -1;
    crosswin_monitor *cm = NULL;
    surface_data *sd = pv;
    control_data *cd = sd->cd;

    if(!strncasecmp("Screen", in, min(in_len, 6)))
    {
        if(toupper(in[6]) == 'W' || toupper(in[6]) == 'H' || toupper(in[6]) == 'X' || toupper(in[6]) == 'Y')
        {
            char val = toupper(in[6]);

            if(in_len > 7)
            {
                if(in[7] == '#')
                {
                    vindex = strtoull(in + 8, NULL, 10);
                }
                else if(toupper(in[7]) == 'P')
                {
                    vindex = 1;
                }
            }
            else
            {
                crosswin_get_position(sd->sw, NULL, NULL, &vindex);
            }

            cm = crosswin_get_monitor(&cd->c, vindex);

            if(cm && cd->c.mon_cnt >= vindex)
            {
                *out = zmalloc(64);

                switch(val)
                {
                    case 'X':
                        out_len = snprintf(*out, 64, "%ld", cm->x);
                        break;

                    case 'Y':
                        out_len = snprintf(*out, 64, "%ld", cm->y);
                        break;

                    case 'W':
                        out_len = snprintf(*out, 64, "%ld", cm->w);
                        break;

                    case 'H':
                        out_len = snprintf(*out, 64, "%ld", cm->h);
                        break;
                }
            }
        }
    }

    return(out_len);
}

int xpander(xpander_request *xr)
{

    surface_data *sd = NULL;
    control_data *cd = NULL;
    section sect = NULL;
    long sx = 0;
    long sy = 0;
    long sw = 0;
    long sh = 0;
    unsigned char escapments = 0;
    unsigned char *wbuf = xr->os;
    unsigned char has_var = 0;
    unsigned char xp_target = 3;
    unsigned char xp = 1;

    if(xr == NULL || xr->os == NULL || xr->req_type == 0 || xr->requestor == NULL) //we must have those set
        return (0);
    else if((xr->req_type & 0x4) && (xr->req_type & 0x8) && (xr->req_type & 0x10)) //allow only one type to be handled
        return (0);
    else if(!(xr->req_type & (XPANDER_SECTIONS | XPANDER_VARIABLES)))              //if we do not know what to expand just fuck off
        return (0);

    switch(xr->req_type & 0x1C)
    {
        case XPANDER_REQUESTOR_OBJECT:
        {
            sd = ((object*)xr->requestor)->sd;
            sect = ((object*)xr->requestor)->os;
            break;
        }

        case XPANDER_REQUESTOR_SOURCE:
        {
            sd = ((source*)xr->requestor)->sd;
            sect = ((source*)xr->requestor)->cs;
            break;
        }

        case XPANDER_REQUESTOR_SURFACE:
        {
            sd = xr->requestor;
            sect = sd->spm;
            break;
        }
    }

    cd = sd->cd;
    crosswin_get_position(sd->sw, &sx, &sy, NULL);
    crosswin_get_size(sd->sw, &sw, &sh);
#warning "Monitor variables incomplete"
    xpander_table tbl[] =
    {
        { "^",                               sd->sp.data_dir,                     NULL },
        { "NL",                                         "\n",                     NULL },
        { "Surfaces",                        cd->surface_dir,                     NULL },
        { "Extensions",                          cd->ext_dir,                     NULL },
        { "Semper",                             cd->root_dir,                     NULL },
        { "ThisSection",     skeleton_get_section_name(sect),                     NULL },
        { "SurfaceX",                                    &sx,     xpander_convert_long },
        { "SurfaceY",                                    &sy,     xpander_convert_long },
        { "SurfaceW",                                    &sw,     xpander_convert_long },
        { "SurfaceH",                                    &sh,     xpander_convert_long },
        { "SurfaceName",              sd->sp.surface_rel_dir,                     NULL },

#ifdef WIN32
        { "OS",                               "Windows",                          NULL }
#elif __linux__
        { "OS",                               "Linux",                            NULL }
#endif
    };


    if((xr->req_type & 0x3) == XPANDER_SECTIONS)
        xp = XPANDER_SECTIONS;
    else if((xr->req_type & 0x3) == XPANDER_VARIABLES)
        xp_target = 2;

    for(; xp < xp_target; xp++)
    {
        do
        {
            size_t sym = 0;
            size_t new_len = 0;
            size_t *ovec = 0;
            size_t ovecl = 0;
            size_t old_end = 0;
            unsigned char *new_buf = NULL;
            has_var = 0;

            string_tokenizer_info sti =
            {
                .buffer = wbuf,
                .oveclen = 0,
                .ovecoff = NULL,
                .filter_data = &sym,
                .string_tokenizer_filter = (xp & XPANDER_VARIABLES) ? xpander_var_token_filter : xpander_src_token_filter
            };

            string_tokenizer(&sti);
            ovec = sti.ovecoff;
            ovecl = sti.oveclen;

            for(size_t i = 0; i < ovecl / 2; i++)
            {

                size_t start = ovec[2 * i];
                size_t end = ovec[2 * i + 1];
                unsigned char alloc = 0;
                unsigned char *buf_start = wbuf + start;

                if(start == end)
                    continue;

                if(old_end > 0 && old_end == start)
                {
                    old_end = 0;
                    start++;
                    buf_start = wbuf + start;
                }

                if((wbuf[start + 1] == '~' ||  wbuf[end - 1] == '~')) //handle escapments
                {
                    if((wbuf[start] == '[' && wbuf[end] == ']') || (wbuf[start] == '$' && wbuf[end] == '$'))
                    {
                        old_end = end++;
                        escapments = 1;
                    }
                }

                else if((xp & XPANDER_VARIABLES) && wbuf[start] == '$' && wbuf[end] == '$') //handle surface variables
                {
                    unsigned char found = 0;

                    old_end = end;
                    start++;

                    //try to match a variable from the table
                    for(size_t ti = 0; ti < sizeof(tbl) / sizeof(xpander_table); ti++)
                    {
                        if(!strncasecmp(wbuf + start, tbl[ti].vn, end - start))
                        {
                            start = 0;
                            found = 1;
                            has_var = 1;

                            if(tbl[ti].conv_func)
                            {
                                alloc = 1;
                                end = tbl[ti].conv_func(tbl[ti].vv, &buf_start);
                            }
                            else
                            {
                                buf_start = tbl[ti].vv;
                                end = string_length(tbl[ti].vv);
                            }

                            break;
                        }
                    }

                    //try to match a surface variable
                    if(found == 0)
                    {

                        size_t len = xpander_index_variable(wbuf + start, end - start, &buf_start, sd);

                        if(len == (size_t) - 1)
                        {
                            key k = skeleton_get_key_n(sd->sv, wbuf + start, end - start);

                            if(k)
                            {
                                has_var = 1;
                                buf_start = skeleton_key_value(k);
                                start = 0;
                                found = 1;
                                end = string_length(buf_start);
                            }
                        }
                        else
                        {
                            start = 0;
                            alloc = 1;
                            found = 1;
                            end = len;
                        }
                    }

                    if(found == 0)
                    {
                        start--;
                        end++;
                    }
                }
                else if((xp & XPANDER_SECTIONS) && wbuf[start] == '[' && wbuf[end] == ']') //handle section variables
                {
                    unsigned char found = 0;

                    old_end = end;
                    start++;

                    source *s = source_by_name(sd, wbuf + start, end - start);

                    //try a source
                    if(s)
                    {
                        buf_start = source_variable(s, &end, SOURCE_VARIABLE_EXPAND);
                        start = 0;
                        found = 1;
                        has_var = 1;
                    }

                    //no source? try then a section variable
                    if(found == 0)
                    {
                        unsigned char *marker = memchr(wbuf + start, ':', end - start); //if the sv is not NULL, then we need to deal with some nasty stuff
                        xpander_section_variable_info xsvi =
                        {
                            .sd = sd,
                            .buffer = wbuf,
                            .start = start,
                            .end = end,
                            .marker = marker - wbuf,
                        };

                        if(marker && xpander_section_variables(&xsvi) == 0)
                        {
                            start = 0;
                            buf_start = xsvi.out_buf;
                            end = xsvi.out_len;
                            found = 1;
                            has_var = 1;
                            alloc = 1;
                        }
                    }

                    //still unlucky?
                    if(found == 0)
                    {
                        start--;
                        end++;
                    }
                }

                void *tmp = realloc(new_buf, new_len + (end - start) + 1);

                if(tmp)
                {
                    new_buf = tmp;
                    strncpy(new_buf + new_len, buf_start, end - start);
                    new_len += end - start;
                    new_buf[new_len] = 0; //make it a proper string
                }

                if(alloc)
                {
                    sfree((void**)&buf_start);
                }

                if(tmp == NULL)
                {
                    sfree((void**)&new_buf);
                    break;
                }
            }

            sfree((void**)&sti.ovecoff);

            if(new_buf && wbuf && has_var && !strcasecmp(wbuf, new_buf))
                has_var = 0;

            if(wbuf == xr->os)
            {
                wbuf = new_buf;
            }
            else
            {
                sfree((void**)&wbuf);
                wbuf = new_buf;
            }

            xr->es = new_buf;
        }
        while(has_var);
    }


    if(escapments && xr->es)
    {
        for(size_t i = 0; xr->es[i]; i++)
        {
            int p1 = (xr->es[i] == '[' && xr->es[i + 1] == '~') || (xr->es[i] == '$' && xr->es[i + 1] == '~');
            int p2 = (xr->es[i] == '~' && xr->es[i + 1] == ']') || (xr->es[i] == '~' && xr->es[i + 1] == '$');

            if(p1)
            {
                for(size_t j = i + 1; xr->es[j]; j++)
                {
                    xr->es[j] = xr->es[j + 1];
                }
            }

            if(p2)
            {
                for(size_t j = i; xr->es[j]; j++)
                {
                    xr->es[j] = xr->es[j + 1];
                }
            }
        }
    }

    if(xr->es == NULL)
    {
        xr->es = expand_env_var(xr->os);
    }
    else
    {
        unsigned char *temp = expand_env_var(xr->es);
        sfree((void**)&xr->es);
        xr->es = temp;
    }


    return(xr->es != NULL);
}
