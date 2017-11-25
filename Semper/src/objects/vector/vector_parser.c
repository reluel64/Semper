#include <string_util.h>
#include <objects/vector.h>
#include <enumerator.h>
#include <ancestor.h>
#include <xpander.h>
#include <mem.h>
#include <parameter.h>
#define PARAMS_LENGTH 10

typedef struct _vector_parse_func  vector_parser_info;
typedef  int (*param_parse_func)(vector_parser_info *pvpi);

typedef struct
{
    size_t op;
    size_t quotes;
    unsigned char quote_type;
} string_parser_status;

typedef enum
{
    vector_parser_path_mode,
    vector_parser_attrib_mode,
    vector_parser_subpath_mode
} vector_parser_mode;

struct _vector_parse_func
{
    cairo_matrix_t mtx;
    vector_param_type vpmt;
    vector_path_type   vpt;
    unsigned char param;
    unsigned char valid;
    unsigned char *lvl;
    unsigned char *pm;
    size_t pm_len;
    object *o;

    double params[PARAMS_LENGTH];
    param_parse_func func;
};


/*The parsing process should be done in the following sequence*
 * 1) Read the matrix settings for the path
 * 2) Read the path, validate the path
 * 3) Read the other attributes
 */


static int vector_parse_filter(string_tokenizer_status *pi, void* pv)
{
    string_parser_status* sps = pv;

    if(pi->reset)
    {
        memset(sps,0,sizeof(string_parser_status));
        return(0);
    }

    if(sps->quote_type==0 && (pi->buf[pi->pos]=='"'||pi->buf[pi->pos]=='\''))
        sps->quote_type=pi->buf[pi->pos];

    if(pi->buf[pi->pos]==sps->quote_type)
        sps->quotes++;

    if(sps->quotes%2)
        return(0);
    else
        sps->quote_type=0;

    if(pi->buf[pi->pos] == '(')
        if(++sps->op==1)
            return(1);

    if(sps->op&&pi->buf[pi->pos] == ')')
        if(--sps->op==0)
            return(0);

    if(pi->buf[pi->pos] == ';')
        return (sps->op==0);

    if(sps->op%2&&pi->buf[pi->pos] == ',')
        return (1);

    return (0);
}


static int vector_parse_option(vector_parser_info *vpi)
{
    string_parser_status spa= {0};
    string_tokenizer_info    sti=
    {
        .buffer                  = vpi->pm,
        .filter_data             = &spa,
        .string_tokenizer_filter = vector_parse_filter,
        .ovecoff                 = NULL,
        .oveclen                 = 0
    };

    string_tokenizer(&sti);

    for(size_t i=0; i<sti.oveclen/2; i++)
    {
        size_t start = sti.ovecoff[2*i];
        size_t end   = sti.ovecoff[2*i+1];

        if(start==end)
        {
            continue;
        }

        /*Clean spaces*/

        if(string_strip_space_offsets(sti.buffer,&start,&end)==0)
        {
            if(sti.buffer[start]=='(')
            {
                start++;
                vpi->param=1;
            }
            else if(sti.buffer[start]==',')
            {
                start++;
                vpi->param++;
            }
            else if(sti.buffer[start]==';')
            {
                vpi->pm=NULL;
                vpi->func(vpi);
                vpi->vpmt=vector_param_none;
                vpi->vpt=vector_path_unknown;
                start++;
            }
            if(sti.buffer[end-1]==')')
            {
                end--;
            }
        }

        if(string_strip_space_offsets(sti.buffer,&start,&end)==0)
        {
            if(sti.buffer[start]=='"'||sti.buffer[start]=='\'')
                start++;

            if(sti.buffer[end-1]=='"'||sti.buffer[end-1]=='\'')
                end--;
        }

        vpi->pm=sti.buffer+start;
        vpi->pm_len=end-start;

        if(start<end)
        {
            if(vpi->param==0)
            {
                vpi->func(vpi);
            }
            else //let's handle the parameters
            {
                vpi->func(vpi);
            }
        }

        if(i+1==sti.oveclen/2)
        {
            vpi->pm=NULL;
            vpi->func(vpi);
        }
    }
    sfree((void**)&sti.ovecoff);
    vpi->pm=sti.buffer; //restore the buffer state

    return(0);
}


static int vector_parse_matrix(vector_parser_info *vpi)
{
    if(vpi->param==0)
    {
        memset(&vpi->params,0,sizeof(vpi->params));

        if(!strncasecmp(vpi->pm,"Skew",4))
            vpi->vpmt=vector_param_skew;

        else if(!strncasecmp(vpi->pm,"Rotate",6))
            vpi->vpmt=vector_param_rotate;

        else if(!strncasecmp(vpi->pm,"Scale",5))
            vpi->vpmt=vector_param_scale;

        else if(!strncasecmp(vpi->pm,"Offset",6))
            vpi->vpmt=vector_param_offset;
    }
    else if(vpi->pm && vpi->param>1&&vpi->param<=PARAMS_LENGTH)
    {
        unsigned char lpm[256]= {0};
        strncpy(lpm,vpi->pm,min(vpi->pm_len,255));
        vpi->params[vpi->param-1]=atof(lpm);
    }
    else if(vpi->pm==NULL)
    {
#warning "Implement check"
        switch(vpi->vpmt)
        {
        case vector_param_offset:
            if(vpi->param>3)
                cairo_matrix_translate(&vpi->mtx,vpi->params[0],vpi->params[1]);
            break;
        }
    }
}

static int vector_parse_path_set(vector_parser_info *vpi)
{
    if(vpi->param==0)
    {
        memset(&vpi->params,0,sizeof(vpi->params));

        if(!strncasecmp(vpi->pm,"ArcTo",4))
            vpi->vpt=vector_path_set_arc_to;

        else if(!strncasecmp(vpi->pm,"CurveTo",6))
            vpi->vpt=vector_path_set_curve_to;

        else if(!strncasecmp(vpi->pm,"LineTo",5))
            vpi->vpt=vector_path_set_line_to;
    }
    else if(vpi->pm && vpi->param>1&&vpi->param<=PARAMS_LENGTH)
    {
        unsigned char lpm[256]= {0};
        strncpy(lpm,vpi->pm,min(vpi->pm_len,255));
        vpi->params[vpi->param-1]=atof(lpm);
    }
    else if(vpi->pm==NULL)
    {
#warning "Implement check"
    }
}

static int vector_parse_paths(vector_parser_info *vpi)
{
    if(vpi->param==0)
    {
        memset(&vpi->params,0,sizeof(vpi->params));

        if(!strncasecmp(vpi->pm,"Rect",4))
            vpi->vpt=vector_path_rectangle;

        else if(!strncasecmp(vpi->pm,"Arc",6))
            vpi->vpt=vector_path_arc;

        else if(!strncasecmp(vpi->pm,"Line",5))
            vpi->vpt=vector_path_line;

        else if(!strncasecmp(vpi->pm,"Ellipse",6))
            vpi->vpt=vector_path_ellipse;

        else if(!strncasecmp(vpi->pm,"PathSet",7))
            vpi->vpt=vector_path_set;
    }
    else if(vpi->pm && vpi->param>1&&vpi->param<=PARAMS_LENGTH)
    {
        unsigned char lpm[256]= {0};
        strncpy(lpm,vpi->pm,min(vpi->pm_len,255));

        if(vpi->vpt==vector_path_set&&vpi->param==3)
        {
            vector_parser_info lvpi= {0};
            unsigned char *lval=parameter_string(vpi->o,lpm,NULL,XPANDER_OBJECT);

            if(lval)
            {
                lvpi.pm=lval;
                lvpi.o=vpi->o;
                lvpi.func=vector_parse_path_set;
                vector_parse_option(&lvpi);
                sfree((void**)&lval);
            }
        }
        else
        {
            vpi->params[vpi->param-1]=atof(lpm);
        }
    }
    else if(vpi->pm==NULL)
    {
#warning "Implement check"
    }
}


static int vector_parse_attributes(vector_parser_info *vpi)
{
    if(vpi->param==0)
    {
        memset(&vpi->params,0,sizeof(vpi->params));

        if(!strncasecmp(vpi->pm,"StrokeWidth",11))
            vpi->vpmt=vector_param_stroke_width;

        else if(!strncasecmp(vpi->pm,"Stroke",6))
            vpi->vpmt=vector_param_stroke;

        else if(!strncasecmp(vpi->pm,"Fill",4))
            vpi->vpmt=vector_param_fill;

        else if(!strncasecmp(vpi->pm,"LineJoin",8))
            vpi->vpmt=vector_param_join;

        else if(!strncasecmp(vpi->pm,"LineCap",7))
            vpi->vpmt=vector_param_cap;

        else if(!strncasecmp(vpi->pm,"Attributes",10))
            vpi->vpmt=vector_param_shared;

    }
    else if(vpi->pm && vpi->param>1&&vpi->param<=PARAMS_LENGTH)
    {
        unsigned char lpm[256]= {0};
        strncpy(lpm,vpi->pm,min(vpi->pm_len,255));

        if(vpi->vpt==vector_path_set&&vpi->param==3)
        {
            vector_parser_info lvpi= {0};
            unsigned char *lval=parameter_string(vpi->o,lpm,NULL,XPANDER_OBJECT);

            if(lval)
            {
                lvpi.pm=lval;
                lvpi.o=vpi->o;
                lvpi.func=vector_parse_path_set;
                vector_parse_option(&lvpi);
                sfree((void**)&lval);
            }
        }
        else
        {
            vpi->params[vpi->param-1]=atof(lpm);
        }
    }
    else if(vpi->pm==NULL)
    {
#warning "Implement check"
    }
}



int vector_parser_init(object *o)
{
    void *es=NULL;
    unsigned char *eval=enumerator_first_value(o,ENUMERATOR_OBJECT,&es);
    static param_parse_func ppf[] =
    {
        vector_parse_paths,
        vector_parse_matrix,
        vector_parse_attributes
    };

    do
    {
        vector_parser_info vpi= {0};
        cairo_matrix_init_identity(&vpi.mtx);
        if(eval==NULL)
            break;

        if(strncasecmp("Path",eval,4))
            continue;

        unsigned char *val=parameter_string(o,eval,NULL,XPANDER_OBJECT);

        if(val==NULL)
            continue;

        vpi.pm=val;
        vpi.o=o;

        for(unsigned char i=0; i<sizeof(ppf)/sizeof(param_parse_func); i++)
        {
            vpi.func=ppf[i];
            vector_parse_option(&vpi);
        }

        sfree((void**)&val);

    }
    while((eval=enumerator_next_value(es))!=NULL);

    enumerator_finish(&es);
}
