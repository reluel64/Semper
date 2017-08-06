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

typedef struct
{
    unsigned char *vn;
    void *vv;
    size_t  (*conv_func)(void *d,unsigned char **out);
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

static size_t xpander_convert_long(void *in,unsigned char **out)
{
    long val=*((long*)in);
    long cval=labs(val);
    size_t len=0;

    for(; cval>=(long)pow(10,(double)len); len++);

    len+=(val<=0?1:0);


    *out=zmalloc(len+1);
    snprintf(*out,len+1,"%ld",val);

    return(len);
}

static size_t xpander_convert_double(void *in,unsigned char **out)
{
    double val=*((double*)in);
    double cval=fabs(val);
    size_t len=0;

    for(; cval>=pow(10,(double)len); len++);

    len+=(val<=0.0?1:0);

    *out=zmalloc(len+1);
    snprintf(*out,len+1,"%lf",val);

    return(len);
}

static int xpander_section_variables(xpander_section_variable_info *xsvi)
{

    int ret=-1;

    xsvi->out_len=0;
    xsvi->out_buf=NULL;
    unsigned char *req_var_start=xsvi->buffer + xsvi->marker+1;
    size_t req_var_len=xsvi->end - xsvi->marker-1;
    source *s=NULL;
    object *o=NULL;

    o=(object_by_name(xsvi->sd,xsvi->buffer + xsvi->start,xsvi->marker - xsvi->start));
    s=(o==NULL?source_by_name(xsvi->sd,xsvi->buffer + xsvi->start,xsvi->marker - xsvi->start):NULL);

    if(o)
    {
        long w = (((o->w!=o->auto_w||o->w < 0)&&o->auto_w!=0) ? o->auto_w : o->w);
        long h = (((o->h!=o->auto_h||o->h < 0)&&o->auto_h!=0) ? o->auto_h : o->h);

        if(strncasecmp("Source",req_var_start,6)==0)
        {
            size_t index=0;
            unsigned char val=0;
            if(strncasecmp("Value",req_var_start+6,5)==0)
            {
                req_var_start+=5;
                val=1;
            }
            sscanf(req_var_start+6,"%llu",&index);
            unsigned char *sn=bind_source_name(o,index);
            if(sn)
            {
                if(val==0)
                {
                    xsvi->out_buf=clone_string(sn);
                    xsvi->out_len=string_length(sn);
                    ret=0;
                }
                else if(val)
                {
                    xsvi->out_buf=clone_string(source_variable(source_by_name(xsvi->sd,sn,-1),&xsvi->out_len,SOURCE_VARIABLE_EXPAND));
                    ret=0;
                }
            }
        }
        else if(strncasecmp("X",req_var_start,1)==0)
        {
            xsvi->out_len=xpander_convert_long(&o->x,&xsvi->out_buf);
            ret=0;
        }
        else if(strncasecmp("Y",req_var_start,1)==0)
        {
            xsvi->out_len=xpander_convert_long(&o->y,&xsvi->out_buf);
            ret=0;
        }
        else if(strncasecmp("W",req_var_start,1)==0)
        {
            xsvi->out_len=xpander_convert_long(&w,&xsvi->out_buf);
            ret=0;
        }
        else if(strncasecmp("H",req_var_start,1)==0)
        {
            xsvi->out_len=xpander_convert_long(&h,&xsvi->out_buf);
            ret=0;
        }
    }
    else if(s)
    {
        if(strncasecmp("MaxValue",req_var_start,req_var_len)==0)
        {
            xsvi->out_len=xpander_convert_double(&s->max_val,&xsvi->out_buf);
            ret=0;
        }
        else if(strncasecmp("MinValue",req_var_start,req_var_len)==0)
        {
            xsvi->out_len=xpander_convert_double(&s->min_val,&xsvi->out_buf);
            ret=0;
        }
    }

    return(ret);
}


static int xpander_var_token_filter(string_tokenizer_status *sts,void *pv)
{
    if(sts->buf[sts->pos]=='$')
    {
        return(1);
    }
    return(0);
}

static int xpander_src_token_filter(string_tokenizer_status *sts,void *pv)
{
    size_t *sym=pv;
    if(sts->buf[sts->pos]=='[')
    {
        if((*sym)==0)
        {
            (*sym)++;
            return(1);
        }
    }
    else if(sts->buf[sts->pos]==']')
    {
        if((*sym))
        {
            (*sym)--;
            return(1);
        }
    }
    return(0);
}

int xpander(xpander_request *xr)
{

    surface_data *sd=NULL;
    control_data *cd=NULL;
    section sect=NULL;
    unsigned char escapments=0;
    unsigned char *wbuf=xr->os;
    unsigned char has_var=0;
    unsigned char xp_target=3;
    unsigned char xp=1;

    if(xr == NULL || xr->os == NULL || xr->req_type == 0 || xr->requestor == NULL) //we must have those set
        return (0);
    else if((xr->req_type & 0x4) && (xr->req_type & 0x8) && (xr->req_type & 0x10)) //allow only one type to be handled
        return (0);
    else if(!(xr->req_type&(XPANDER_SECTIONS|XPANDER_VARIABLES)))                  //if we do not know what to expand just fuck off
        return (0);

    switch(xr->req_type & 0x1C)
    {
    case XPANDER_REQUESTOR_OBJECT:
    {
        sd=((object*)xr->requestor)->sd;
        sect=((object*)xr->requestor)->os;
        break;
    }
    case XPANDER_REQUESTOR_SOURCE:
    {
        sd = ((source*)xr->requestor)->sd;
        sect=((source*)xr->requestor)->cs;
        break;
    }
    case XPANDER_REQUESTOR_SURFACE:
    {
        sd=xr->requestor;
        sect=sd->spm;
        break;
    }
    }

    cd=sd->cd;

    xpander_table tbl[]=
    {
        { "^",                               sd->sp.data_dir,                    NULL },
        { "NL",                                         "\n",                    NULL },
        { "Surfaces",                        cd->surface_dir,                    NULL },
        { "Extensions",                          cd->ext_dir,                    NULL },
        { "Semper",                             cd->root_dir,                    NULL },
        { "ThisSection",     skeleton_get_section_name(sect),                    NULL },
        { "ScreenW",                               &cd->c.sw,    xpander_convert_long },
        { "ScreenH",                               &cd->c.sh,    xpander_convert_long },
        { "SurfaceX",                                 &sd->x,    xpander_convert_long },
        { "SurfaceY",                                 &sd->y,    xpander_convert_long },
        { "SurfaceW",                                 &sd->w,    xpander_convert_long },
        { "SurfaceH",                                 &sd->h,    xpander_convert_long },
#ifdef WIN32
        { "OS",                               "Windows",                         NULL }
#elif __linux__
        { "OS",                               "Linux",                            NULL }
#endif
    };


    if((xr->req_type&0x3)==XPANDER_SECTIONS)
        xp=XPANDER_SECTIONS;
    else if((xr->req_type&0x3)==XPANDER_VARIABLES)
        xp_target=2;

    for(; xp<xp_target; xp++)
    {
        do
        {
            size_t sym=0;
            size_t new_len=0;
            unsigned char *new_buf=NULL;
            size_t *ovec=0;
            size_t ovecl=0;

            size_t old_end=-1;
            has_var=0;

            string_tokenizer_info sti=
            {
                .buffer=wbuf,
                .oveclen=0,
                .ovecoff=NULL,
                .filter_data=&sym,
                .string_tokenizer_filter=xp&XPANDER_VARIABLES?xpander_var_token_filter:xpander_src_token_filter
            };

            string_tokenizer(&sti);
            ovec=sti.ovecoff;
            ovecl=sti.oveclen;

            for(size_t i=0; i<ovecl/2; i++)
            {

                size_t start=ovec[2*i];
                size_t end=ovec[2*i+1];
                unsigned char alloc=0;
                unsigned char *buf_start=wbuf+start;

                if(start==end)
                    continue;

                if((wbuf[start+1]=='~' ||  wbuf[end-1]=='~')) //handle escapments
                {
                    if((wbuf[start]=='[' &&wbuf[end]==']')||(wbuf[start]=='$' && wbuf[end]=='$'))
                    {
                        //end+=1;
                        escapments=1;
                    }
                }
                else  if(old_end==start)
                {
                    start++;
                    buf_start=wbuf+start;
                }
                else if((xp&XPANDER_VARIABLES)&&wbuf[start]=='$' && wbuf[end]=='$') //handle surface variables
                {
                    unsigned char found=0;

                    old_end=end;
                    start++;

                    //try to match a variable from the table
                    for(size_t i=0; i<sizeof(tbl)/sizeof(xpander_table); i++)
                    {
                        if(!strncasecmp(wbuf+start,tbl[i].vn,end-start))
                        {
                            start=0;
                            found=1;
                            has_var=1;
                            if(tbl[i].conv_func)
                            {
                                alloc=1;
                                end=tbl[i].conv_func(tbl[i].vv,&buf_start);
                                break;
                            }
                            else
                            {
                                buf_start=tbl[i].vv;
                                end=string_length(tbl[i].vv);
                            }
                        }
                    }
                    //try to match a surface variable
                    if(found==0)
                    {
                        key k=skeleton_get_key_n(sd->sv,wbuf+start,end-start);

                        if(k)
                        {
                            has_var=1;
                            buf_start=skeleton_key_value(k);
                            start=0;
                            found=1;
                            end=string_length(buf_start);
                        }
                    }
                    if(found==0)
                    {
                        start--;
                        end++;
                        buf_start=wbuf+start;
                    }
                }
                else if((xp&XPANDER_SECTIONS)&&wbuf[start]=='[' &&wbuf[end]==']') //handle section variables
                {
                    unsigned char found=0;

                    old_end=end;
                    start++;

                    source *s=source_by_name(sd,wbuf+start,end-start);
                    //try a source
                    if(s)
                    {
                        buf_start=source_variable(s,&end,SOURCE_VARIABLE_EXPAND);
                        start=0;
                        found=1;
                        has_var=1;
                    }
                    //no source? try then a section variable
                    if(found==0)
                    {
                        unsigned char *marker=memchr(wbuf+start,':',end-start); //if the sv is not NULL, then we need to deal with some nasty stuff
                        xpander_section_variable_info xsvi=
                        {
                            .sd=sd,
                            .buffer=wbuf,
                            .start=start,
                            .end=end,
                            .marker=marker-wbuf,
                        };

                        if(marker&&xpander_section_variables(&xsvi)==0)
                        {
                            start=0;
                            buf_start=xsvi.out_buf;
                            end=xsvi.out_len;
                            found=1;
                            has_var=1;
                            alloc=1;
                        }
                    }
                    //still unlucky?
                    if(found==0)
                    {
                        start--;
                        end++;
                        buf_start=wbuf+start;
                    }
                }
                void *tmp=realloc(new_buf,new_len+(end-start)+1);
                if(tmp)
                {
                  new_buf=tmp;
                  strncpy(new_buf+new_len,buf_start,end-start);
                  new_len+=end-start;
                  new_buf[new_len]=0; //make it a proper string
                }
                if(alloc)
                    sfree((void**)&buf_start);
                if(tmp==NULL)
                {
                  sfree((void**)&new_buf);
                  break;
                }
            }

            sfree((void**)&sti.ovecoff);

            if(new_buf&&wbuf&&has_var&&!strcasecmp(wbuf,new_buf))
                has_var=0;

            if(wbuf==xr->os)
            {
                wbuf=new_buf;
            }
            else
            {
                sfree((void**)&wbuf);
                wbuf=new_buf;
            }
            xr->es=new_buf;
        }
        while(has_var);
    }


    if(escapments&&xr->es)
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

    return(xr->es!=NULL);
}
