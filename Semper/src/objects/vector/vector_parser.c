#include <string_util.h>
#include <objects/vector.h>
#include <enumerator.h>
#include <ancestor.h>
#include <xpander.h>
#include <mem.h>
#include <parameter.h>
#if 1
typedef struct
{
    size_t op;
    size_t quotes;
    unsigned char quote_type;
} string_parser_status;

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


static void *vector_alloc_index(list_entry *head, size_t index,size_t bytes)
{
    vector_path_common *vp = NULL;
    vector_path_common *p=NULL;
    vector_path_common *n=NULL;
    list_enum_part(vp, head, current)
    {
        if(vp->index < index)
            p = vp;
        if(vp->index > index)
            n = vp;
        if(n && p)
            break;
        if(vp->index == index)
            return (NULL);
    }

    vp = zmalloc(bytes);
    vp->stroke_w=1.0;
    vp->index = index;
    list_entry_init(&vp->current);

    if(n == NULL)
    {
        linked_list_add_last(&vp->current, head);
    }
    else if(p == NULL)
    {
        linked_list_add(&vp->current, head);
    }
    else
    {
        _linked_list_add(&p->current, &vp->current, &n->current);
    }

    return (vp);
}




static int vector_subpath_fill(vector_subpath_common *vsc,unsigned char *pm,unsigned char param)
{
    if(vsc == NULL)
        return(-1);
    switch(vsc->vpt)
    {
    case vector_subpath_arc_to:
    {
        vector_path_arc_to *vpat=(vector_path_arc_to*)vsc;
        switch(param)
        {
        case 1:
            vpat->radius=atof(pm);
            break;
        case 2:
            vpat->start_angle=atof(pm);
            break;
        case 3:
            vpat->end_angle=atof(pm);
            break;
        }
        break;
    }
    case vector_subpath_line_to:
    {
        vector_path_line_to *vplt=(vector_path_line_to*)vsc;

        switch(param)
        {
        case 1:
            vplt->dx=atof(pm);
            break;
        case 2:
            vplt->dy=atof(pm);
            break;
        }
        break;
    }
    case vector_subpath_curve_to:
    {
        vector_path_curve_to *vpct=(vector_path_curve_to*)vsc;
        switch(param)
        {
        case 1:
            vpct->dx1=atof(pm);
            break;
        case 2:
            vpct->dy1=atof(pm);
            break;
        case 3:
            vpct->dx2=atof(pm);
            break;
        case 4:
            vpct->dy2=atof(pm);
            break;
        case 5:
            vpct->dx3=atof(pm);
            break;
        case 6:
            vpct->dy3=atof(pm);
            break;
        }
        break;
    }
    default:
        break;
    }
    return(0);
}

static void *vector_fill_base_paths(vector *v,unsigned char *str,size_t index)
{
    vector_path_common *vpc=NULL;

    if(!strncasecmp(str,"Arc",3)&&strncasecmp(str,"ArcTo",5))
    {
        if((vpc=vector_alloc_index(&v->paths,index+1,sizeof(vector_line)))!=NULL)
        {
            vpc->vpt=vector_path_arc;
        }
    }
    else if(!strncasecmp(str,"Line",4)&&strncasecmp(str,"LineTo",6))
    {
        if((vpc=vector_alloc_index(&v->paths,index+1,sizeof(vector_line)))!=NULL)
        {
            vpc->vpt=vector_path_line;
        }
    }
    else if(!strncasecmp(str,"Rect",4))
    {
        if((vpc=vector_alloc_index(&v->paths,index+1,sizeof(vector_rectangle)))!=NULL)
        {
            vpc->vpt=vector_path_rectangle;
        }
    }
    else if(!strncasecmp(str,"PathSet",7))
    {
        if((vpc=vector_alloc_index(&v->paths,index+1,sizeof(vector_path)))!=NULL)
        {
            vpc->vpt=vector_path_path;
            list_entry_init(&((vector_path*)vpc)->paths);
        }
    }
    return(vpc);
}

static int vector_parse_paths(object *o)
{
    void *es=NULL;
    unsigned char *eval=enumerator_first_value(o,ENUMERATOR_OBJECT,&es);
    vector *v=o->pv;
    do
    {
        vector_param_type vpmt=vector_param_none;
        vector_path_common *vpc=NULL;

        vector_subpath_common *vsc=NULL;
        string_parser_status spa= {0};
        unsigned char param=0;
        size_t index=0;
        if(eval==NULL)
            break;

        if(strncasecmp("Path",eval,4))
            continue;

        unsigned char *val=parameter_string(o,eval,NULL,XPANDER_OBJECT);

        if(val==NULL)
            continue;

        string_tokenizer_info    sti=
        {
            .buffer                  = val, //store the string address here
            .filter_data             = &spa,
            .string_tokenizer_filter = vector_parse_filter,
            .ovecoff                 = NULL,
            .oveclen                 = 0
        };


        string_tokenizer(&sti);

        sscanf(eval+4,"%llu",&index);

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
                    param=1;
                }
                else if(sti.buffer[start]==',')
                {
                    start++;
                    param++;
                }
                else if(sti.buffer[start]==';')
                {
                    vsc=NULL;
                    vpmt=vector_param_none;
                    param=0;
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

            unsigned char *str=sti.buffer+start;
            size_t len=end-start;

            if(param==0)
            {
                if(!vpc)
                {
                    if((vpc=vector_fill_base_paths(v,str,index))==NULL)
                    {
                        continue;
                    }
                }
                else if(vpc) //handle the possible parameters
                {
                    if(vpc->vpt==vector_path_path&&!strncasecmp(str,"ArcTo",5))
                    {
                        vsc=zmalloc(sizeof(vector_path_arc_to));
                        list_entry_init(&vsc->current);
                        linked_list_add(&vsc->current,&((vector_path*)vpc)->paths);
                        vsc->vpt=vector_subpath_arc_to;

                    }
                    else if(vpc->vpt==vector_path_path&&!strncasecmp(str,"LineTo",6))
                    {
                        vsc=zmalloc(sizeof(vector_path_line_to));
                        list_entry_init(&vsc->current);
                        linked_list_add(&vsc->current,&((vector_path*)vpc)->paths);
                        vsc->vpt=vector_subpath_line_to;

                    }
                    else if(vpc->vpt==vector_path_path&&!strncasecmp(str,"CurveTo",7))
                    {
                        vsc=zmalloc(sizeof(vector_path_curve_to));
                        list_entry_init(&vsc->current);
                        linked_list_add(&vsc->current,&((vector_path*)vpc)->paths);
                        vsc->vpt=vector_subpath_curve_to;

                    }
                    else if(!strncasecmp(str,"StrokeWidth",11))
                        vpmt=vector_param_stroke_width;

                    else if(!strncasecmp(str,"Stroke",6))
                        vpmt=vector_param_stroke;

                    else if(!strncasecmp(str,"Fill",4))
                        vpmt=vector_param_fill;

                    else if(!strncasecmp(str,"LineJoin",8))
                        vpmt=vector_param_join;

                    else if(!strncasecmp(str,"LineCap",7))
                        vpmt=vector_param_cap;
                }
            }
            else if(vpc&&start<end) //let's handle the parameters
            {
                unsigned char pm[256]= {0};
                strncpy(pm,sti.buffer+start,min(end-start,255));
                if(vpmt==vector_param_none)
                {
                    switch(vpc->vpt)
                    {
                    default:
                        break;
                    case vector_path_path:
                    {
                        vector_subpath_fill(vsc,pm,param);
                        break;
                    }
                    case vector_path_rectangle:
                    {
                        vector_rectangle *vr=(vector_rectangle*)vpc;

                        switch(param)
                        {
                        case 1:
                            vr->x=atof(pm);
                            break;
                        case 2:
                            vr->y=atof(pm);
                            break;
                        case 3:
                            vr->w=atof(pm);
                            break;
                        case 4:
                            vr->h=atof(pm);
                            break;
                        }
                        break;
                    }
                    case vector_path_line:
                    {
                        vector_line *vl=(vector_line*)vpc;

                        switch(param)
                        {
                        case 1:
                            vl->sx=atof(pm);
                            break;
                        case 2:
                            vl->sy=atof(pm);
                        case 3:
                            vl->dx=atof(pm);
                            break;
                        case 4:
                            vl->dy=atof(pm);
                            break;
                        }
                        break;
                    }
                    case vector_path_arc:
                    {
                        vector_arc *va=(vector_arc*)vpc;

                        switch(param)
                        {
                        case 1:
                            va->xc=atof(pm);
                            break;
                        case 2:
                            va->yc=atof(pm);
                        case 3:
                            va->radius=atof(pm);
                            break;
                        case 4:
                            va->sa=DEG2RAD(atof(pm));
                            break;
                        case 5:
                            va->ea=DEG2RAD(atof(pm));
                            break;
                        }
                        break;
                    }
                    }
                }
                else
                {
                    switch(vpmt)
                    {
                    default:
                        break;
                    case vector_param_stroke_width:
                        vpc->stroke_w=atof(pm);
                        vpmt=vector_param_done;
                        break;

                    case vector_param_stroke:
                        vpc->stroke_color=string_to_color(pm);
                        vpmt=vector_param_done;
                        break;

                    case vector_param_fill:
                        vpc->fill_color=string_to_color(pm);
                        vpmt=vector_param_done;
                        break;

                    case vector_param_join:
                        if(!strncasecmp(pm,"Round",5))
                            vpc->join=CAIRO_LINE_JOIN_ROUND;
                        else if(!strncasecmp(pm,"Bevel",5))
                            vpc->join=CAIRO_LINE_JOIN_BEVEL;
                        vpmt=vector_param_done;
                        break;

                    case vector_param_cap:
                        if(!strncasecmp(pm,"ROUND",5))
                            vpc->cap=CAIRO_LINE_CAP_ROUND;
                        else if(!strncasecmp(pm,"SQUARE",6))
                            vpc->cap=CAIRO_LINE_CAP_SQUARE;
                        vpmt=vector_param_done;
                        break;
                    }
                }
            }
        }
        sfree((void**)&val);
        sfree((void**)&sti.ovecoff);
        sti.buffer=NULL;
    }
    while((eval=enumerator_next_value(es))!=NULL);

    enumerator_finish(&es);
    return(0);
}
#endif





int vector_parser_init(object *o)
{
    vector_parse_paths(o);
}
