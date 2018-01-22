/*
*The glue between Objects and Sources
*Part of Project 'Semper'
*Written by Alexandru-Daniel Mărgărit
*/
#include <objects/object.h>
#include <bind.h>
#include <surface.h>
#include <mem.h>
#include <stddef.h>
#include <math.h>
#include <string_util.h>
#include <xpander.h>
#include <semper.h>
#include <parameter.h>
#include <ancestor.h>
#include <enumerator.h>
#include <sources/source.h>
#include <inttypes.h>
#include <ctype.h>

static inline void bind_dealloc_index(bind_index** bi)
{
    linked_list_remove(&(*bi)->current);
    sfree((void**)bi);
}

static void bind_dealloc(binding** b)
{
    bind_index* bi = NULL;
    bind_index* tbi = NULL;

    linked_list_remove(&(*b)->current);

    list_enum_part_safe(bi, tbi, &(*b)->index, current)
    {
        bind_dealloc_index(&bi);
    }
    sfree((void**)b);
}

void bind_destroy(object* o)
{
    binding* b = NULL;
    binding* tb = NULL;

    list_enum_part_safe(b, tb, &o->bindings, current)
    {
        bind_dealloc(&b);
    }
}

static binding* bind_alloc(object* o, source* s)
{
    binding* pos = NULL;
    list_enum_part(pos, &o->bindings, current)
    {
        if(pos->s == s)
            return (pos);
    }
    pos = zmalloc(sizeof(binding));
    list_entry_init(&pos->current);
    list_entry_init(&pos->index);
    linked_list_add_last(&pos->current, &o->bindings);
    pos->s = s;
    return (pos);
}

static int bind_alloc_index(binding* b, size_t index)
{
    bind_index* bi = NULL;
    bind_index* p = NULL;
    bind_index* n = NULL;

    list_enum_part(bi, &b->index, current)
    {
        if(bi->index < index)
            p = bi;

        if(bi->index > index)
            n = bi;

        if(n && p)
            break;

        if(bi->index == index)
            return (0);
    }

    bi = zmalloc(sizeof(bind_index));
    bi->index = index;
    list_entry_init(&bi->current);

    if(n == NULL)
    {
        linked_list_add_last(&bi->current, &b->index);
    }
    else if(p == NULL)
    {
        linked_list_add(&bi->current, &b->index);
    }
    else
    {
        _linked_list_add(&p->current, &bi->current, &n->current);
    }

    return (1);
}

static void bind_populate(object* o)
{
    void* es = NULL; // enumerator internal state
    unsigned char* ev = enumerator_first_value(o, ENUMERATOR_OBJECT, &es);

    do
    {
        if(ev == NULL)
            break;

        if(strncasecmp(ev, "Source", 6))
            continue;

        size_t index = 0;
        source* s = NULL;

        sscanf(ev+6, "%llu", &index);
        unsigned char* sn = parameter_string(o, ev, NULL, XPANDER_OBJECT);
        s = source_by_name(o->sd, sn,-1);
        sfree((void**)&sn);

        if(s&&s->die==0)
        {
            binding* bs = bind_alloc(o, s);
            bind_alloc_index(bs, index);
        }
    }
    while((ev = enumerator_next_value(es)) != NULL);

    enumerator_finish(&es);
}

unsigned char *bind_source_name(object *o,size_t index)
{
    binding *b=NULL;
    list_enum_part(b,&o->bindings,current)
    {
        bind_index*bi=NULL;
        list_enum_part(bi,&b->index,current)
        {
            if(index==bi->index)
            {
                if(b->s)
                {
                    return(skeleton_get_section_name(b->s->cs));
                }
                else
                {
                    return(NULL);
                }
            }
        }
    }
    return(NULL);
}

double bind_percentual_value(double val, double min_val, double max_val)
{
    double range = max_val - min_val;

    if(range == 0.0)
    {
        return (1.0);
    }

    val = min(val, max_val);
    val = max(val, min_val);
    val -= min_val;
    return (val / range);
}

static void bind_invalidate(object* o)
{
    binding* b = NULL;
    binding* tb = NULL;

    list_enum_part_safe(b, tb, &o->bindings, current)
    {
        if(b->s == NULL || linked_list_empty((&b->index)))
        {
            bind_dealloc(&b);
            continue;
        }
        else
        {
            unsigned char* sname = skeleton_get_section_name(b->s->cs);
            bind_index* bi = NULL;
            bind_index* tbi = NULL;

            list_enum_part_safe(bi, tbi, &b->index, current)
            {
                unsigned char buf[256] = { 0 };

                if(bi->index)
                {
                    snprintf(buf, sizeof(buf), "Source%llu", bi->index);
                }
                else
                {
                    snprintf(buf, sizeof(buf), "Source");
                }

                unsigned char* p = parameter_string(o, buf, NULL, XPANDER_OBJECT);

                if(p && sname && strcasecmp(p, sname))
                {
                    bind_dealloc_index(&bi);
                }

                sfree((void**)&p);
            }
        }

        if(b->s == NULL || linked_list_empty((&b->index)))
        {
            bind_dealloc(&b);
        }
    }
}

void bind_reset(object* o)
{
    bind_populate(o);
    bind_invalidate(o);
}

void bind_unbind(surface_data* sd, source *s)
{
    object* o = NULL;
    list_enum_part(o, &sd->objects, current)
    {
        binding* b = NULL;
        list_enum_part(b, &o->bindings, current)
        {
            if(b->s==s)
            {
                bind_dealloc(&b);
                break;
            }
        }
    }
}

static inline size_t binding_index(unsigned char* s, size_t* skip)
{
    if(s[0] == '%' && isdigit(s[1]))
    {
        unsigned char* next = NULL;
        size_t index = strtoull(s + 1, (char**)&next, 10);

        *skip = (next - s) - 1;
        return (index);
    }

    *skip = 0;
    return (0);
}

static inline size_t need_replacements(unsigned char* s)
{
    for(unsigned long i = 0; s[i]; i++)
    {
        if(s[i] == '%' && isdigit(s[i + 1]))
        {
            return (1);
        }
    }

    return (0);
}

static void bind_create_strings(object* o, string_bind* sb)
{
    binding* b = NULL;
    list_enum_part(b, &o->bindings, current)
    {
        if(!b->s || b->s->die||b->s->s_info)
        {
            continue;
        }

        source* s = b->s;


        double formula = s->d_info;
        unsigned char mul = 0;
        unsigned char multiples[] = { '\0', 'K', 'M', 'G', 'T', 'P', 'E' };

        if(sb->percentual)
        {
            formula = bind_percentual_value(s->d_info, s->min_val, s->max_val) * 100.0;
        }
        else if(sb->scale != 0.0)
        {
            formula = s->d_info / sb->scale;
        }
        else if(sb->self_scaling)
        {
            switch(sb->self_scaling)
            {
            case 1:
                for(formula = s->d_info; formula > 1024.0; formula /= 1024.0, ++mul);
                break;

            case 2:
                for(formula = s->d_info; formula > 1000.0; formula /= 1000.0, ++mul);
                break;

            case 3:
                for(formula = s->d_info; formula > 1024.0 * 1024; formula /= (1024.0 * 1024), ++mul);
                break;

            case 4:
                for(formula = s->d_info; formula > 1000.0 * 1000; formula /= (1000.0 * 1000), ++mul);
                break;
            }
        }

        unsigned char* fs = NULL;
        unsigned char fbuf[256] = { 0 };
        unsigned char out_buf[256] = { 0 };
        snprintf(fbuf, 256, "%%.%ulf", sb->decimals);
        snprintf(out_buf, 256, fbuf, formula);
        remove_trailing_zeros(out_buf);

        size_t fs_sz = (string_length(out_buf) + 3);
        fs = zmalloc(fs_sz + 1);
        strcpy(fs, out_buf);

        if(mul < sizeof(multiples) / sizeof(unsigned char) && multiples[mul])
        {
            fs[fs_sz - 3] = ' ';
            fs[fs_sz - 2] = multiples[mul];
        }

        if(s->replacements)
        {
            b->bs = replace(fs, s->replacements, s->regexp);

            if(b->bs == NULL)
            {
                b->bs = fs;
            }
            else
            {
                sfree((void**)&fs);
            }
        }
        else
        {
            b->bs = fs;
        }

        b->bsl = string_length(b->bs);
    }
}

static inline void bind_destroy_strings(object* o)
{
    binding* b = NULL;
    list_enum_part(b, &o->bindings, current)
    {
        sfree((void**)&b->bs);
        b->bsl = 0;
    }
}

static inline size_t bind_string_sz(object* o, size_t index)
{
    binding* b = NULL;
    list_enum_part(b, &o->bindings, current)
    {
        bind_index* bi = NULL;
        list_enum_part(bi, &b->index, current)
        {
            if(bi->index==index)
            {
                if(b->s&&b->s->s_info&&b->s->die==0)
                {
                    return(b->s->s_info_len);
                }
                else
                {
                    return (b->bsl);
                }
            }
        }
    }

    return (0);
}

static inline unsigned char* bind_string(object* o, size_t index, size_t* sz)
{
    binding* b = NULL;
    *sz = 0;

    list_enum_part(b, &o->bindings, current)
    {
        bind_index* bi = NULL;
        list_enum_part(bi, &b->index, current)
        {
            if(bi->index == index)
            {
                if(b->s&&b->s->s_info&&b->s->die==0)
                {
                    *sz=b->s->s_info_len;
                    return(b->s->s_info);
                }
                else
                {
                    *sz = b->bsl;
                    return (b->bs);
                }
            }
        }
    }
    return (NULL);
}

static inline size_t bind_query_memory(object* o, string_bind* sb)
{
    size_t mem = 0;

    for(size_t i = 0; sb->s_in[i]; i++)
    {
        if(sb->s_in[i] == '%')
        {
            size_t skip = 0;
            size_t vi = binding_index(sb->s_in + i, &skip);
            size_t bsz = bind_string_sz(o, vi);

            if(skip)
            {
                if(bsz)
                    mem += bsz;
                else
                    mem += skip + 1;

                i += skip;
                continue;
            }

            mem++;
            continue;
        }

        mem++;
    }

    return (mem);
}

size_t bind_update_string(object* o, string_bind* sb)
{

    size_t nmbl = 0;
    size_t di = 0;

    if(!o || !sb || !sb->s_in)
    {
        return (0);
    }

    if(!need_replacements(sb->s_in))
    {
        sb->s_out = sb->s_in;
        return (string_length(sb->s_out));
    }

    bind_create_strings(o, sb);
    nmbl = bind_query_memory(o, sb);
    sb->s_out = zmalloc(nmbl + 1);

    for(size_t i = 0; sb->s_in[i]; i++)
    {
        if(sb->s_in[i] == '%')
        {
            size_t skip = 0;
            size_t vi = binding_index(sb->s_in + i, &skip);
            size_t bsl = 0;

            if(skip)
            {
                unsigned char* bs = NULL;

                if((bs = bind_string(o, vi, &bsl)) != NULL)
                {
                    strcpy(sb->s_out + di, bs);
                    di += bsl;
                }
                else
                {
                    strncpy(sb->s_out + di, sb->s_in + i, skip + 1);
                    di += skip + 1; // skip number + '%'
                }

                i += skip; // skip number because '%' will be skipped in the next iteration
                continue;
            }

            sb->s_out[di++] = sb->s_in[i];
            continue;
        }

        sb->s_out[di++] = sb->s_in[i];
    }

    bind_destroy_strings(o);
    return (nmbl);
}

int bind_update_numeric(object* o, bind_numeric* bn)
{
    if(o == NULL || bn == NULL)
    {
        return (-1);
    }

    bn->max = 1;
    bn->min = 0;
    bn->val = 0;

    binding* b = NULL;

    list_enum_part(b, &o->bindings, current)
    {
        if(b->s == NULL || b->s->die)
        {
            continue;
        }

        bind_index* bi = NULL;

        list_enum_part(bi, &b->index, current)
        {
            if(bi->index == bn->index)
            {
                bn->max = b->s->max_val;
                bn->min = b->s->min_val;
                bn->val = b->s->d_info;

                if(bn->val > bn->max)
                {
                    bn->max = bn->val;
                }

                if(bn->max == 0.0)
                {
                    bn->max = 1.0;
                }

                return (0);
            }
        }
    }

    return (-1);
}
