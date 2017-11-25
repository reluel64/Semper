/*
Source action
Part of Project "S"
Written by Alexandru-Daniel Mărgărit
*/
#include <sources/source.h>
#include <sources/action.h>
#include <semper.h>
#include <mem.h>
#include <string_util.h>
#include <xpander.h>
#include <skeleton.h>
#include <parameter.h>
#include <enumerator.h>
#include <math_parser.h>
#define PCRE_STATIC
#include <pcre.h>

/*
This code takes care of both SourceActions and SourceConditions
*/


void action_init(source* s)
{
    if(s!=NULL && s->sa == NULL)
    {
        s->sa = zmalloc(sizeof(source_action));
    }

    source_action* sa = s->sa;

    list_entry_init(&sa->abv);
    list_entry_init(&sa->blw);
    list_entry_init(&sa->equ);
    list_entry_init(&sa->match);
    list_entry_init(&sa->cond);
}

static action* action_alloc(list_entry* head, size_t index)
{
    action* a = NULL;
    action* p = NULL;
    action* n = NULL;

    list_enum_part(a, head, current)
    {

        if(a->index == index)
            return (a);

        if(a->index > index)
            n = a;

        if(a->index < index)
            p = a;

        if(n && p)
            break;
    }

    a = zmalloc(sizeof(action));
    list_entry_init(&a->current);
    a->index = index;

    if(p == NULL)
        linked_list_add(&a->current, head);
    else if(n == NULL)
        linked_list_add_last(&a->current, head);
    else
        _linked_list_add(&p->current, &a->current, &n->current);

    return (a);
}

static void action_remove(action** a)
{
    sfree((void**)&(*a)->actstr);
    sfree((void**)&(*a)->cond_e);
    sfree((void**)&(*a)->cond_f);
    sfree((void**)&(*a)->cond_t);
    linked_list_remove(&(*a)->current);
    sfree((void**)a);
}

void action_destroy(source* s)
{
    source_action* sa = s->sa;
    action* a = NULL;
    action* ta = NULL;

    list_enum_part_safe(a, ta, &sa->blw, current)
    action_remove(&a);

    list_enum_part_safe(a, ta, &sa->equ, current)
    action_remove(&a);

    list_enum_part_safe(a, ta, &sa->abv, current)
    action_remove(&a);

    list_enum_part_safe(a, ta, &sa->cond, current)
    action_remove(&a);

    list_enum_part_safe(a, ta, &sa->match, current)
    action_remove(&a);

    sfree((void**)&s->sa);
}

static int action_populate(source* s)
{
    void* es = NULL;
    unsigned char* v = enumerator_first_value(s, ENUMERATOR_SOURCE, &es);
    source_action* sa = s->sa;

    do
    {
        size_t index = 0;


        if(!v)
        {
            break;
        }

        if(strncasecmp(v, "Source", 6))
        {
            continue;
        }

        if(!strncasecmp(v + 6, "Above", 5) || !strncasecmp(v + 6, "Equal", 5) || !strncasecmp(v + 6, "Below", 5))
        {
            unsigned char set_val = 0;

            if(strncasecmp(v + 11, "Action", 6))
            {
                set_val = 1;
            }

            if(set_val)
            {
                sscanf(v + 11, "%llu", &index);
            }
            else
            {
                sscanf(v + 17, "%llu", &index);
            }

            action* a = NULL;

            if(strncasecmp(v + 6, "Above", 5) == 0)
            {
                a = action_alloc(&sa->abv, index);
            }
            else if(strncasecmp(v + 6, "Below", 5) == 0)
            {
                a = action_alloc(&sa->blw, index);
            }

            else if(strncasecmp(v + 6, "Equal", 5) == 0)
            {
                a = action_alloc(&sa->equ, index);
            }

            if(set_val)
            {
                a->val = parameter_double(s, v, 0.0, XPANDER_SOURCE);
            }
            else
            {
                sfree((void**)&a->actstr);
                a->actstr = parameter_string(s, v, NULL, XPANDER_SOURCE);
            }
        }
        else if(!strncasecmp(v + 6, "Condition", 9) || !strncasecmp(v + 6, "Match", 5))
        {
            unsigned char type = 0;
            unsigned char _bool = 0;
            unsigned char scond = 0;

            if(!strncasecmp(v + 6, "Condition", 9))
            {
                type = 1;
            }

            if(type)
            {
                if(!strncasecmp(v + 15, "True", 4))
                {
                    _bool = 1;
                    scond = 1;
                }
                else if(!strncasecmp(v + 15, "False", 5))
                {
                    scond = 1;
                }
            }
            else
            {
                if(!strncasecmp(v + 11, "True", 4))
                {
                    _bool = 1;
                    scond = 1;
                }
                else if(!strncasecmp(v + 11, "False", 5))
                {
                    scond = 1;
                }
            }

            if(type)
            {
                scond?sscanf(v + 15 + (_bool ? 4 : 5), "%llu", &index):sscanf(v + 15, "%llu", &index);
            }
            else
            {
                scond?sscanf(v + 11 + (_bool ? 4 : 5), "%llu", &index):sscanf(v + 11, "%llu", &index);
            }

            action* a = action_alloc(type == 0 ? &sa->match : &sa->cond, index);

            if(scond)
            {
                sfree((void**)(_bool ? &a->cond_t : &a->cond_f));

                if(_bool)
                {
                    sfree((void**)&a->cond_t);
                    a->cond_t = parameter_string(s, v, NULL, XPANDER_SOURCE);
                }
                else
                {
                    sfree((void**)&a->cond_f);
                    a->cond_f = parameter_string(s, v, NULL, XPANDER_SOURCE);
                }
            }
            else
            {
                sfree((void**)&a->cond_e);
                a->cond_e = parameter_string(s, v, NULL, XPANDER_SOURCE);
            }
        }
    }
    while((v = enumerator_next_value(es)));

    enumerator_finish(&es);
    return (0);
}

static void action_remove_invalid(source* s)
{
    source_action* sa = s->sa;

    list_entry* le[] = { &sa->abv, &sa->equ, &sa->blw, &sa->cond, &sa->match };

    for(unsigned char i = 0; i < 5; i++)
    {
        action* a = NULL;
        action* ta = NULL;
        list_enum_part_safe(a, ta, le[i], current)
        {
            if((i < 2 && a->actstr == NULL) || (i > 2 && a->cond_e == NULL))
            {
                action_remove(&a);
            }
        }
    }
}

void action_reset(source* s)
{
    action_populate(s);
    action_remove_invalid(s);
}


static int action_match(unsigned char* str, unsigned char* pattern)
{
    if(str == NULL || pattern == NULL)
    {
        return (0);
    }

    int match_count = 0;
    int err_off = 0;
    const char* err = NULL;
    size_t strl = string_length(str);
    int ovector[300];
    pcre* pc = pcre_compile((char*)pattern, 0, &err, &err_off, NULL);

    if(pc == NULL)
    {
        return (0);
    }

    match_count = pcre_exec(pc, NULL, (char*)str, strl, 0, 0, ovector, sizeof(ovector) / sizeof(int)); /*good explanation at http://libs.wikia.com/wiki/Pcre_exec*/
    pcre_free(pc);

    return (match_count > 0);
}

static int action_math_parser(unsigned char *vn,size_t len,double *v,void *pv)
{
    source *s=source_by_name(pv,vn,len);

    if(s)
    {
        *v=(double)s->d_info;
        return(0);
    }

    return(-1);
}

void action_execute(source* s)
{

    source_action* sa = s->sa;
    list_entry* le[] = { &sa->blw, &sa->equ, &sa->abv };
    action* a = NULL;

    for(unsigned char i = 0; i < 3; i++)
    {
        list_enum_part(a, le[i], current)
        {
            unsigned char _bool = 0;

            switch(i)
            {
                case 0:
                    _bool = a->val > s->d_info;
                    break;

                case 1:
                    _bool = a->val == s->d_info;
                    break;

                case 2:
                    _bool = a->val < s->d_info;
                    break;
            }

            if(_bool && (a->done == 0 || s->always_do))
            {
                a->actstr? command(s->sd, &a->actstr):0;
                a->done = 1;
            }
            else if(!(_bool))
            {
                a->done = 0;
            }
        }
    }

    list_enum_part(a, &sa->cond, current)
    {
        double v=0.0;

        if(math_parser(a->cond_e,&v,action_math_parser,s->sd)==0)
        {
            unsigned char bool_true=v>0.0;

            if(bool_true && (a->done != 1 || s->always_do))
            {
                a->cond_t?command(s->sd, &a->cond_t):0;
                a->done = 1;
            }
            else if(!bool_true && (a->done != 2 || s->always_do))
            {
                a->cond_f?command(s->sd, &a->cond_f):0;
                a->done = 2;
            }
        }
    }

    list_enum_part(a, &sa->match, current)
    {
        unsigned char bool_true = (unsigned char) ((s->s_info&&a->cond_e)?action_match(s->s_info, a->cond_e):0);

        if(bool_true && (a->done != 1 || s->always_do))
        {
            a->cond_t?command(s->sd, &a->cond_t):0;
            a->done = 1;
        }
        else if(!bool_true && (a->done != 2 || s->always_do))
        {
            a->cond_f?command(s->sd, &a->cond_f):0;
            a->done = 2;
        }
    }
}
