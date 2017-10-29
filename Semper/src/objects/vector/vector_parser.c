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


static vector_paths *vector_paths_alloc_index(list_entry *head, size_t index)
{
    vector_paths *vp = NULL;
    vector_paths *p=NULL;
    vector_paths *n=NULL;
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

    vp = zmalloc(sizeof(vector_paths));
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



static int vector_parse_paths(object *o)
{
    void *es=NULL;
    unsigned char *eval=enumerator_first_value(o,ENUMERATOR_OBJECT,&es);
    vector *v=o->pv;
    do
    {
        vector_path_type vpt=vector_path_unknown;
        vector_param_type vpmt=vector_param_none;
        vector_path_set vps=vector_path_set_none;

        vector_paths *vp=NULL;
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

        sscanf(eval+5,"%llu",&index);
        if((vp=vector_paths_alloc_index(&v->paths_sets,index+1))==NULL)
        {
            sfree((void**)&val);
            sfree((void**)&sti.ovecoff);
            continue;
        }

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
                    // vpt=vector_path_unknown;
                    vpmt=vector_param_none;
                    vps=vector_path_set_none;
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
                if(vpt==vector_path_unknown)
                {
                    if(!strncasecmp(str,"Arc",3)&&strncasecmp(str,"ArcTo",5))
                    {
                        vpt=vector_path_arc;
                    }
                    else if(!strncasecmp(str,"Line",4)&&strncasecmp(str,"LineTo",6))
                    {
                        vpt=vector_path_line;
                    }
                    else if(!strncasecmp(str,"Rect",4))
                    {
                        vpt=vector_path_rectangle;
                    }
                    else if(!strncasecmp(str,"PathSet",7))
                    {
                        vpt=vector_path_path;
                    }
                }
                else //handle the possible parameters
                {
                    if(vpt==vector_path_path&&!strncasecmp(str,"ArcTo",5))
                    {
                        vps=vector_path_set_arc_to;
                    }
                    else if(vpt==vector_path_path&&!strncasecmp(str,"LineTo",6))
                    {
                        vps=vector_path_set_line_to;
                    }
                    else if(vpt==vector_path_path&&!strncasecmp(str,"CurveTo",7))
                    {
                        vps=vector_path_set_curve_to;
                    }
                    else if(!strncasecmp(str,"StrokeWidth",11))
                    {
                        vpmt=vector_param_stroke_width;
                    }
                    else if(!strncasecmp(str,"StrokeColor",11))
                    {
                        vpmt=vector_param_stroke_color;
                    }
                }
            }
            else if(start<end)
            {
                unsigned char pm[256]= {0};
                strncpy(pm,sti.buffer+start,min(end-start,255));

                switch(vpt)
                {
                case vector_path_path:
                {
                    switch(vps)
                    {
                        case vector_path_set_arc_to:
                        break;

                    }
                    break;
                }
                }


                printf("Param %s\n",pm);
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
