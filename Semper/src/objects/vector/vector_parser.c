#include <string_util.h>
#include <objects/vector.h>
#include <cairo.h>
#include <enumerator.h>
#include <ancestor.h>
#include <xpander.h>
#include <mem.h>
#include <parameter.h>
#include <objects/vector/vector_clipper_glue.h>
#define PARAMS_LENGTH 10

typedef struct _vector_parse_func  vector_parser_info;
typedef  int (*param_parse_func)(vector_parser_info *pvpi);

typedef struct
{
    size_t op;
    size_t quotes;
    unsigned char quote_type;
} string_parser_status;


struct _vector_parse_func
{
    cairo_t *cr;                /*Ideally would be to have the context
                                 *that we use for drawing but since it is not available here,
                                 *we create our own
                                 * */
    vector_param_type vpmt;
    vector_path_type   vpt;
    vector_clip_type  vct;
    unsigned char param;
    unsigned char valid;
    unsigned char lvl;
    unsigned char *pm;
    unsigned char *opm;
    size_t pm_len;
    object *o;
    void *pv;
    double params[PARAMS_LENGTH];
    param_parse_func func;
};


typedef struct
{
    size_t index;
    vector_clip_type vct;
    list_entry current;
} vector_join_path;

typedef struct
{
    vector_path_common vpc;
    size_t sub_index;
    list_entry join;
} vector_join_list;


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
    vpi->opm=vpi->pm;
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
                vpi->vct=0;
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


static void vector_parser_apply_matrix(vector_parser_info *vpi, vector_path_common *vpc,cairo_matrix_t *mtx)
{
    cairo_path_t *path=vpc->cr_path;
    for (int i=0; i < path->num_data; i += path->data[i].header.length)
    {
        cairo_path_data_t *data = &path->data[i];
        switch (data->header.type)
        {
            case CAIRO_PATH_MOVE_TO:
                cairo_matrix_transform_point(mtx,&data[1].point.x,&data[1].point.y);
                break;

            case CAIRO_PATH_LINE_TO:
                cairo_matrix_transform_point(mtx,&data[1].point.x,&data[1].point.y);
                break;

            case CAIRO_PATH_CURVE_TO:
                cairo_matrix_transform_point(mtx,&data[1].point.x,&data[1].point.y);
                cairo_matrix_transform_point(mtx,&data[2].point.x,&data[2].point.y);
                cairo_matrix_transform_point(mtx,&data[3].point.x,&data[3].point.y);
                break;

            case CAIRO_PATH_CLOSE_PATH:
                break;
        }
    }
    /*update the extents*/
    cairo_new_path(vpi->cr);
    cairo_append_path(vpi->cr,path);
    cairo_path_extents(vpi->cr,&vpc->ext.x,&vpc->ext.y,&vpc->ext.width,&vpc->ext.height);
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
        switch(vpi->vpt)
        {
            case vector_path_set_line_to:
            {
                if(vpi->param>=2)
                {
                    cairo_line_to(vpi->cr, vpi->params[0], vpi->params[1]);
                }
                break;
            }
            case vector_path_set_curve_to:
            {
                if(vpi->param >= 4)
                {
                    if(vpi->param>=6)
                    {
                        cairo_curve_to(vpi->cr, vpi->params[0],vpi->params[1],vpi->params[2],vpi->params[3],vpi->params[4],vpi->params[5]);
                    }
                    else
                    {
                        cairo_curve_to(vpi->cr, vpi->params[0],vpi->params[1],vpi->params[0],vpi->params[1],vpi->params[4],vpi->params[5]);
                    }
                }
                break;
            }
            case vector_path_set_arc_to:
            {
                if(vpi->param>=7)
                {
                    /*  vector_path_arc_to *vpat=zmalloc(sizeof(vector_path_arc_to));
                      vpsc=&vpat->vsc;
                      vpat->rx    = vpi->params[0];
                      vpat->ry    = vpi->params[1];
                      vpat->ex    = vpi->params[3];
                      vpat->ey    = vpi->params[4];
                      vpat->angle = vpi->params[2];
                      vpat->sweep = vpi->params[5];
                      vpat->large = vpi->params[6];*/
                }
                break;
            }
            default:
                break;
        }
    }
    return(0);
}


static int vector_parse_fused_paths(vector_parser_info *vpi)
{
    if(vpi->param==0)
    {
        if(!strncasecmp(vpi->pm,"XOR",3))
            vpi->vct=vector_clip_xor;

        else if(!strncasecmp(vpi->pm,"Union",5))
            vpi->vct=vector_clip_union;

        else if(!strncasecmp(vpi->pm,"Intersect",9))
            vpi->vct=vector_clip_intersect;

        else if(!strncasecmp(vpi->pm,"DifferenceA",11))
            vpi->vct=vector_clip_diff;

        else if(!strncasecmp(vpi->pm,"DifferenceB",11))
            vpi->vct=vector_clip_diff_b;
    }
    else if(vpi->pm && vpi->vct > 0 && vpi->param==1)
    {

        vector_join_path *vjp=zmalloc(sizeof(vector_join_path));
        list_entry_init(&vjp->current);
        linked_list_add_last(&vjp->current,vpi->pv);
        if(!strncasecmp("Path",vpi->pm,4))
        {
            sscanf(vpi->pm+4,"%llu",&vjp->index);
        }
        else
        {
            sscanf(vpi->pm,"%llu",&vjp->index);
        }
        vjp->vct=vpi->vct;
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

        else if(!strncasecmp(vpi->pm,"Join",4))
            vpi->vpt=vector_path_join;

    }
    else if(vpi->pm && vpi->param > 0&&vpi->param <= PARAMS_LENGTH)
    {

        unsigned char lpm[256]= {0};
        strncpy(lpm,vpi->pm,min(vpi->pm_len,255));

        if(vpi->vpt==vector_path_set&&vpi->param==4)
        {

            unsigned char *lval=parameter_string(vpi->o,lpm,NULL,XPANDER_OBJECT);

            if(lval)
            {
                vector_parser_info lvpi= {0};
                lvpi.pm=lval;
                lvpi.cr=vpi->cr;
                lvpi.o=vpi->o;
                lvpi.func=vector_parse_path_set;
                lvpi.pv=NULL;
                cairo_move_to(vpi->cr,vpi->params[0],vpi->params[1]);
                vector_parse_option(&lvpi);

                if((vpi->params[2]>0.0))
                {
                    cairo_close_path(vpi->cr);
                }

                cairo_path_t *temp_pth=cairo_copy_path_flat(vpi->cr);

                if((temp_pth->num_data>2&&vpi->params[2]==0.0)||(vpi->params[2]!=0.0&&temp_pth->num_data>5))
                {
                    vpc=zmalloc(sizeof(vector_path_common));
                    vpc->cr_path=temp_pth;
                }
                else
                {
                    cairo_path_destroy(temp_pth);
                }

                sfree((void**)&lval);
            }
        }
        else if(vpi->vpt==vector_path_join && vpi->param == 1)
        {
            list_entry t_list= {0};
            list_entry_init(&t_list);
            vector_parser_info lvpi= {0};
            lvpi.pm=vpi->pm;
            lvpi.cr=vpi->cr;
            lvpi.o=vpi->o;
            lvpi.func=vector_parse_fused_paths;
            lvpi.pv=&t_list;
            vector_parse_option(&lvpi);

            if(!linked_list_empty(&t_list))
            {
                vector_join_list *vjl=zmalloc(sizeof(vector_join_list));
                list_entry_init(&vjl->join);
                linked_list_replace(&t_list,&vjl->join);
                vpc=&vjl->vpc;
                vpc->must_join=1;
                if(!strncasecmp("Path",vpi->pm,4))
                {
                    sscanf(vpi->pm+4,"%llu",&vjl->sub_index);
                }
                else
                {
                    sscanf(vpi->pm,"%llu",&vjl->sub_index);
                }

                ((vector*)vpi->o->pv)->check_join=1;
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
                    vpc=zmalloc(sizeof(vector_path_common));
                    cairo_move_to(vpi->cr,vpi->params[0],vpi->params[1]);
                    cairo_line_to(vpi->cr,vpi->params[2],vpi->params[3]);
                    vpc->cr_path=cairo_copy_path_flat(vpi->cr);
                }
                break;
            }
            case vector_path_curve:
            {
                if(vpi->param>=6)
                {
                    vpc=zmalloc(sizeof(vector_path_common));
                    cairo_move_to(vpi->cr,vpi->params[0],vpi->params[1]);

                    if(vpi->param>=8)
                    {
                        cairo_curve_to(vpi->cr, vpi->params[2], vpi->params[3], vpi->params[4], vpi->params[5], vpi->params[6], vpi->params[7]);
                    }
                    else
                    {
                        cairo_curve_to(vpi->cr, vpi->params[2], vpi->params[3], vpi->params[2], vpi->params[3], vpi->params[4], vpi->params[5]);
                    }

                    if((vpi->params[6]>0.0))
                    {
                        cairo_close_path(vpi->cr);
                    }
                    vpc->cr_path=cairo_copy_path_flat(vpi->cr);
                }
                break;
            }
            case vector_path_arc:
            {
                /*  if(vpi->param>=9)
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
                  }*/
                break;
            }
            case vector_path_rectangle:
            {
                if(vpi->param>=4)
                {
                    vpc=zmalloc(sizeof(vector_path_common));

                    if(vpi->param==4)
                    {
                        cairo_rectangle(vpi->cr,vpi->params[0],vpi->params[1],vpi->params[2],vpi->params[3]);
                    }

                    /* vector_rectangle *vr=zmalloc(sizeof(vector_rectangle));
                     vpc=&vr->vpc;
                     vr->x     = vpi->params[0];
                     vr->y     = vpi->params[1];
                     vr->w     = vpi->params[2];
                     vr->h     = vpi->params[3];
                     vr->rx    = vpi->params[4];
                     vr->ry    = vpi->params[5];*/
                    vpc->cr_path=cairo_copy_path_flat(vpi->cr);
                }
                break;
            }
            case vector_path_ellipse:
            {
                if(vpi->param>=3)
                {
                    vpc=zmalloc(sizeof(vector_path_common));

                    cairo_translate(vpi->cr,vpi->params[0],vpi->params[1]);

                    if(vpi->param>=4)
                    {
                        cairo_scale(vpi->cr, vpi->params[2] / 2.0, vpi->params[3] / 2.0);
                    }
                    else
                    {
                        cairo_scale(vpi->cr, vpi->params[2] / 2.0, vpi->params[2] / 2.0);
                    }

                    cairo_arc(vpi->cr,0.0, 0.0, 1.0, 0.0, 2*M_PI);
                    vpc->cr_path=cairo_copy_path_flat(vpi->cr);
                }
                break;
            }

            default:
                break;
        }
    }

    if(vpc)
    {
        vpc->stroke_w=1.0;
        cairo_identity_matrix(vpi->cr);
        list_entry_init(&vpc->current);
        cairo_path_extents(vpi->cr,&vpc->ext.x,&vpc->ext.y,&vpc->ext.width,&vpc->ext.height);
        vpi->pv=vpc;
        return(1); //stop the loop
    }

    return(0);
}


static int vector_parse_attributes(vector_parser_info *vpi)
{
    vector_path_common *vpc=vpi->pv;

    if(vpc->cr_path==NULL)
        return(1);

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

        if(vpi->vpmt==vector_param_shared&&vpi->lvl<1)
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
        else if(vpi->vpmt == vector_param_join)
        {
            if(strncasecmp("Bevel",vpi->pm,5)==0)
                vpc->join=CAIRO_LINE_JOIN_BEVEL;

            else if(strncasecmp("Round",vpi->pm,5)==0)
                vpc->join=CAIRO_LINE_JOIN_ROUND;

            else
                vpc->join=CAIRO_LINE_JOIN_MITER;
        }
        else if(vpi->vpmt == vector_param_cap )
        {
            if(strncasecmp("Square",vpi->pm,4)==0)
                vpc->cap=CAIRO_LINE_CAP_SQUARE;

            else if(strncasecmp("Round",vpi->pm,5)==0)
                vpc->cap=CAIRO_LINE_CAP_BUTT;

            else
                vpc->cap=CAIRO_LINE_JOIN_MITER;
        }
        else if(vpi->vpmt==vector_param_stroke)
        {
            vpc->stroke_color=string_to_color(lpm);
        }
        else if(vpi->vpmt == vector_param_fill)
        {
            vpc->fill_color=string_to_color(lpm);
        }
        else if(vpi->vpmt== vector_param_stroke_width)
        {
            vpc->stroke_w=atof(lpm);
        }
        else
        {
            vpi->params[vpi->param-1]=atof(lpm);
        }
    }
    else if(vpi->pm==NULL)
    {
        switch(vpi->vpmt)
        {
            case vector_param_rotate:
            {
                if(vpi->param>=1)
                {
                    cairo_matrix_t mtx= {0};

                    if(vpi->param>=3)
                    {
                        cairo_matrix_init_translate(&mtx,vpi->params[0]+vpc->ext.x,vpi->params[1]+vpc->ext.y);
                        cairo_matrix_rotate(&mtx,DEG2RAD(vpi->params[2]));
                        cairo_matrix_translate(&mtx,-(vpi->params[0]+vpc->ext.x),-(vpi->params[1]+vpc->ext.y));
                    }
                    else
                    {
                        cairo_matrix_init_translate(&mtx,(vpc->ext.width+vpc->ext.x)/2.0,(vpc->ext.height+vpc->ext.y)/2.0);
                        cairo_matrix_rotate(&mtx,DEG2RAD(vpi->params[0]));
                        cairo_matrix_translate(&mtx,-(vpc->ext.width+vpc->ext.x)/2.0,-(vpc->ext.height+vpc->ext.y)/2.0);
                    }

                    vector_parser_apply_matrix(vpi,vpc,&mtx);
                }
                break;
            }
            case vector_param_scale:
            {
                if(vpi->param>=2)
                {
                    cairo_matrix_t mtx= {0};

                    if(vpi->param>=4)
                    {
                        cairo_matrix_init_translate(&mtx,vpi->params[0]+vpc->ext.x,vpi->params[1]+vpc->ext.y);
                        cairo_matrix_scale(&mtx,vpi->params[2],vpi->params[3]);
                        cairo_matrix_translate(&mtx,-(vpi->params[0]+vpc->ext.x),-(vpi->params[1]+vpc->ext.y));
                    }
                    else
                    {
                        cairo_matrix_init_translate(&mtx,(vpc->ext.width+vpc->ext.x)/2.0,(vpc->ext.height+vpc->ext.y)/2.0);
                        cairo_matrix_scale(&mtx,vpi->params[0],vpi->params[1]);
                        cairo_matrix_translate(&mtx,-(vpc->ext.width+vpc->ext.x)/2.0,-(vpc->ext.height+vpc->ext.y)/2.0);
                    }

                    vector_parser_apply_matrix(vpi,vpc,&mtx);
                }
                break;
            }
            case vector_param_offset:
            {
                cairo_matrix_t mtx= {0};

                if(vpi->param>=2)
                {
                    cairo_matrix_init_translate(&mtx,vpi->params[0],vpi->params[1]);
                    vector_parser_apply_matrix(vpi,vpc,&mtx);
                }
            }
            default:
                if(vpi->vpmt!=0)
                    diag_error("%s %d Unhnadled type %d",__FUNCTION__,__LINE__,vpi->vpmt);
                break;
        }
#warning "Incomplete implementation"
    }
    return(0);
}


static vector_path_common *vector_parse_get_join_path(vector *v,size_t index,size_t join_count)
{
    vector_path_common *vpc=NULL;
    list_enum_part(vpc,&v->paths,current)
    {
        if(vpc->cr_path && vpc->index==index && vpc->join_cnt!=join_count)
        {
            vpc->join_cnt=join_count;
            return(vpc);
        }
    }
    return(NULL);
}


static void vector_parser_destroy_join_list(vector_join_list *vjl)
{
    vector_join_path *vjp=NULL;
    vector_join_path *tvjp=NULL;
    list_enum_part_safe(vjp,tvjp,&vjl->join,current)
    {
        linked_list_remove(&vjp->current);
        sfree((void**)&vjp);
    }
}

static void vector_parser_destroy_item(vector_path_common **vpc)
{
    linked_list_remove(&(*vpc)->current);
    if((*vpc)->must_join)
    {
        vector_parser_destroy_join_list((vector_join_list*)(*vpc));
    }
    if((*vpc)->cr_path)
    {
        cairo_path_destroy((*vpc)->cr_path);
    }

    if((*vpc)->gradient)
    {
        cairo_pattern_destroy((*vpc)->gradient);
        (*vpc)->gradient=NULL;
    }
    sfree((void**)vpc);
}

void vector_parser_destroy(object *o)
{
    vector *v=o->pv;
    while(v->paths.next!=NULL && !linked_list_empty(&v->paths))
    {
        vector_path_common  *vpc = element_of(v->paths.next, vector_path_common, current);
        vector_parser_destroy_item(&vpc);
    }
}

static int vector_parser_sort(list_entry *l1,list_entry *l2,void *pv)
{
    vector_path_common *vpc1=element_of(l1,vector_path_common,current);
    vector_path_common *vpc2=element_of(l2,vector_path_common,current);
    int res=0;


    if(vpc1->index<vpc2->index)
        res=-1;
    else if(vpc1->index>vpc2->index)
        res=1;
    return(res);
}

int vector_parser_init(object *o)
{
    vector *v=o->pv;
    void *es=NULL;
    unsigned char *eval=enumerator_first_value(o,ENUMERATOR_OBJECT,&es);
    cairo_surface_t *dummy_surface=cairo_image_surface_create(CAIRO_FORMAT_A1,0,0);
    cairo_t *cr=cairo_create(dummy_surface);
    static param_parse_func ppf[] =
    {
        vector_parse_paths,
        vector_parse_attributes
    };

    do
    {

        vector_parser_info vpi= {0};
        vpi.cr=cr;
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
            cairo_new_path(cr);
            vpi.func=ppf[i];
            vector_parse_option(&vpi);
            if(vpi.pv==NULL)
                break;
        }
        if(vpi.pv)
        {
            vector *v=o->pv;
            vector_path_common *vpc=vpi.pv;
            sscanf(eval+4,"%llu",& vpc->index);
            linked_list_add_last(&vpc->current,&v->paths);
        }
        sfree((void**)&val);

    }
    while((eval=enumerator_next_value(es))!=NULL);
    enumerator_finish(&es);
    merge_sort(&v->paths,vector_parser_sort,NULL);
    if(v->check_join)
    {

        vector_path_common *vpc=NULL;
        size_t join_count=0;
        list_enum_part(vpc,&v->paths,current)
        {
            if(vpc->must_join)
            {
                join_count++;

                vector_join_list *vjl=(vector_join_list*)vpc;
                vector_path_common *vpc_root=vector_parse_get_join_path(v,vjl->sub_index,join_count);

                if(vpc_root==NULL)
                {
                    diag_error("%s %d Cannot join paths as the root path is missing\n",__FUNCTION__,__LINE__);
                    continue;
                }

                vector_join_path *vjp=NULL;
                vector_join_path *tvjp=NULL;
                cairo_new_path(cr);
                void *clipper=vector_init_clipper();
                void *paths=vector_init_paths();

                //Convert the path from cairo to Clipper
                cairo_append_path(cr,vpc_root->cr_path);
                cairo_close_path(cr);
                vector_cairo_to_clip(cr,paths);


                list_enum_part_safe(vjp,tvjp,&vjl->join,current)
                {
                    cairo_new_path(cr);
                    vector_path_common *vpc_clip=vector_parse_get_join_path(v,vjp->index,join_count);

                    if(vpc_clip&&vpc_clip->cr_path)
                    {

                        unsigned char reversed=vjp->vct==vector_clip_diff_b;
                        unsigned char correct_diff=reversed?vjp->vct-3:vjp->vct-1;

                        vector_add_paths(clipper,paths,reversed,1);

                        cairo_append_path(cr,vpc_clip->cr_path);
                        cairo_close_path(cr);
                        vector_cairo_to_clip(cr,paths);
                        vector_add_paths(clipper,paths,!reversed,1);
                        vector_clip_paths(clipper,paths,correct_diff);

                        linked_list_remove(&vjp->current);
                        sfree((void**)&vjp);

                    }
                }

                cairo_new_path(cr);
                vector_clip_to_cairo(cr,paths);
                vector_destroy_paths(&paths);
                vector_destroy_clipper(&clipper);

                if(vpc->cr_path)
                {
                    cairo_path_destroy(vpc->cr_path);
                    vpc->cr_path=NULL;
                }
                memcpy(vpc,vpc_root,offsetof(vector_path_common,gradient));
                vpc->cr_path=cairo_copy_path_flat(cr);
                vpc->must_join=0;
            }
        }

        /*Cleanup the redundant paths*/
        vector_path_common *tvpc=NULL;
        list_enum_part_safe(vpc,tvpc,&v->paths,current)
        {
            if(vpc->must_join==0&&vpc->join_cnt)
            {
                vector_parser_destroy_item(&vpc);
            }
        }
    }


    cairo_destroy(cr);
    cairo_surface_destroy(dummy_surface);
    return(0);
}
