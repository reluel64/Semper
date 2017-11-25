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
    unsigned char lvl;
    unsigned char *pm;
    size_t pm_len;
    object *o;
    void *pv;
    double params[PARAMS_LENGTH];
    param_parse_func func;
};


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
                if(vpi->func(vpi))
                    break;
                vpi->param=0;
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
            if(vpi->func(vpi))
                break;
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
    else if(vpi->pm && vpi->param>0&&vpi->param<=PARAMS_LENGTH)
    {
        unsigned char lpm[256]= {0};
        strncpy(lpm,vpi->pm,min(vpi->pm_len,255));
        vpi->params[vpi->param-1]=atof(lpm);
    }
    else if(vpi->pm==NULL)
    {
        vector_path_set_common *vpsc=NULL;

        switch(vpi->vpt)
        {
            case vector_path_set_line_to:
            {
                if(vpi->param>=2)
                {
                    vector_path_line_to *vplt=zmalloc(sizeof(vector_path_line_to));
                    vpsc=&vplt->vsc;
                    vplt->dx=vpi->params[0];
                    vplt->dy=vpi->params[1];
                }
                break;
            }
            case vector_path_set_curve_to:
            {
                if(vpi->param>=6)
                {
                    vector_path_curve_to *vpct=zmalloc(sizeof(vector_path_curve_to));
                    vpsc=&vpct->vsc;
                    vpct->dx1=vpi->params[0];
                    vpct->dy1=vpi->params[1];
                    vpct->dx2=vpi->params[2];
                    vpct->dy2=vpi->params[3];
                    vpct->dx3=vpi->params[4];
                    vpct->dy3=vpi->params[5];
                }
                break;
            }
            case vector_path_set_arc_to:
            {
                if(vpi->param>=7)
                {
                    vector_path_arc_to *vpat=zmalloc(sizeof(vector_path_arc_to));
                    vpsc=&vpat->vsc;
                    vpat->rx=vpi->params[0];
                    vpat->ry=vpi->params[1];
                    vpat->ex=vpi->params[3];
                    vpat->ey=vpi->params[4];
                    vpat->angle=vpi->params[2];
                    vpat->sweep=vpi->params[5];
                    vpat->large=vpi->params[6];
                }
                break;
            }
            default:
                break;
        }

        if(vpsc)
        {
            vpsc->vpt=vpi->vpt;
            list_entry_init(&vpsc->current);
            linked_list_add_last(&vpsc->current,(list_entry*)vpi->pv);
        }
    }
    return(0);
}

static int vector_parse_paths(vector_parser_info *vpi)
{
    vector_path_common *vpc=NULL;

    if(vpi->param==0)
    {
        memset(&vpi->params,0,sizeof(vpi->params));

        if(!strncasecmp(vpi->pm,"Rect",4))
            vpi->vpt=vector_path_rectangle;

        else if(!strncasecmp(vpi->pm,"Arc",3))
            vpi->vpt=vector_path_arc;

        else if(!strncasecmp(vpi->pm,"Line",4))
            vpi->vpt=vector_path_line;

        else if(!strncasecmp(vpi->pm,"Curve",5))
            vpi->vpt=vector_path_curve;

        else if(!strncasecmp(vpi->pm,"Ellipse",7))
            vpi->vpt=vector_path_ellipse;

        else if(!strncasecmp(vpi->pm,"PathSet",7))
            vpi->vpt=vector_path_set;
    }
    else if(vpi->pm && vpi->param>0&&vpi->param<=PARAMS_LENGTH)
    {

        unsigned char lpm[256]= {0};
        strncpy(lpm,vpi->pm,min(vpi->pm_len,255));

        if(vpi->vpt==vector_path_set&&vpi->param==4)
        {
            vector_parser_info lvpi= {0};
            unsigned char *lval=parameter_string(vpi->o,lpm,NULL,XPANDER_OBJECT);

            if(lval)
            {
                list_entry t_list= {0};
                lvpi.pm=lval;
                lvpi.o=vpi->o;
                lvpi.func=vector_parse_path_set;
                lvpi.pv=&t_list;

                list_entry_init(&t_list);
                vector_parse_option(&lvpi);

                if(!linked_list_empty(&t_list))
                {
                    vector_path *vp=zmalloc(sizeof(vector_path));
                    vp->x=vpi->params[0];
                    vp->y=vpi->params[1];
                    vp->closed=(vpi->params[2]>0.0);
                    vpc=&vp->vpc;

                    list_entry_init(&vp->path_sets);
                    linked_list_replace(&t_list,&vp->path_sets);
                }

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
        switch(vpi->vpt)
        {
            case vector_path_line:
            {
                if(vpi->param>=4)
                {
                    vector_line *vl=zmalloc(sizeof(vector_line));
                    vpc=&vl->vpc;
                    vl->sx=vpi->params[0];
                    vl->sy=vpi->params[1];
                    vl->dx=vpi->params[2];
                    vl->dy=vpi->params[3];
                }
                break;
            }
            case vector_path_curve:
            {
                if(vpi->param>=6)
                {
                    vector_curve *vc=zmalloc(sizeof(vector_curve));
                    vpc=&vc->vpc;
                    vc->sx     = vpi->params[0];
                    vc->sy     = vpi->params[1];
                    vc->cx1    = vpi->params[2];
                    vc->cy1    = vpi->params[3];
                    vc->cx2    = vpi->params[4];
                    vc->cy2    = vpi->params[5];
                    vc->ex     = vpi->params[6];
                    vc->ey     = vpi->params[7];
                    vc->closed = (vpi->params[8]>0.0);
                }
                break;
            }
            case vector_path_arc:
            {
                if(vpi->param>=9)
                {
                    vector_arc *va=zmalloc(sizeof(vector_arc));
                    vpc=&va->vpc;
                    va->sx     =  vpi->params[0];
                    va->sy     =  vpi->params[1];
                    va->rx     =  vpi->params[2];
                    va->ry     =  vpi->params[3];
                    va->ex     =  vpi->params[4];
                    va->ey     =  vpi->params[5];
                    va->angle  =  vpi->params[6];
                    va->sweep  =  vpi->params[7];
                    va->large  =  vpi->params[8];
                    va->closed = (vpi->params[9]>0.0);
                }
                break;
            }
            case vector_path_rectangle:
            {
                if(vpi->param>=4)
                {
                    vector_rectangle *vr=zmalloc(sizeof(vector_rectangle));
                    vpc=&vr->vpc;
                    vr->x     = vpi->params[0];
                    vr->y     = vpi->params[1];
                    vr->w     = vpi->params[2];
                    vr->h     = vpi->params[3];
                    vr->rx    = vpi->params[4];
                    vr->ry    = vpi->params[5];
                }
                break;
            }
            case vector_path_ellipse:
            {
                if(vpi->param>=3)
                {
                    vector_ellipse *ve=zmalloc(sizeof(vector_ellipse));
                    vpc=&ve->vpc;
                    ve->xc=vpi->params[0];
                    ve->yc=vpi->params[1];
                    ve->rx=vpi->params[2];
                    ve->ry=ve->rx;

                    if(vpi->param>=4)
                    {
                        ve->ry=vpi->params[3];
                    }
                }
                break;
            }

            default:
                break;
        }
    }

    if(vpc)
    {
        list_entry_init(&vpc->current);
        vpi->pv=vpc;
        return(1); //stop the loop
    }

    return(0);
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

        else  if(!strncasecmp(vpi->pm,"Skew",4))
            vpi->vpmt=vector_param_skew;

        else if(!strncasecmp(vpi->pm,"Rotate",6))
            vpi->vpmt=vector_param_rotate;

        else if(!strncasecmp(vpi->pm,"Scale",5))
            vpi->vpmt=vector_param_scale;

        else if(!strncasecmp(vpi->pm,"Offset",6))
            vpi->vpmt=vector_param_offset;

    }
    else if(vpi->pm && vpi->param>0&&vpi->param<=PARAMS_LENGTH)
    {
        unsigned char lpm[256]= {0};
        strncpy(lpm,vpi->pm,min(vpi->pm_len,255));

        if(vpi->vpmt==vector_param_shared&&vpi->param==1&&vpi->lvl<1)
        {
            vector_parser_info lvpi= {0};
            unsigned char *lval=parameter_string(vpi->o,lpm,NULL,XPANDER_OBJECT);

            if(lval)
            {
                lvpi.lvl=1;
                lvpi.pm=lval;
                lvpi.o=vpi->o;
                lvpi.func=vector_parse_attributes;
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
    return(0);
}



int vector_parser_init(object *o)
{
    void *es=NULL;
    unsigned char *eval=enumerator_first_value(o,ENUMERATOR_OBJECT,&es);
    vector *v=o->pv;
    static param_parse_func ppf[] =
    {
        vector_parse_paths,
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
        if(vpi.pv)
        {
            printf("Valid path found\n");
        }
        sfree((void**)&val);

    }
    while((eval=enumerator_next_value(es))!=NULL);

    enumerator_finish(&es);
    return(0);
}
