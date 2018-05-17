/*
 * Ancestor provider
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */
#include <skeleton.h>
#include <stdio.h>
#include <string_util.h>
#include <mem.h>
#include <xpander.h>
#include <sources/source.h>
#include <objects/object.h>
#include <ancestor.h>
#define ANCESTOR_MAX_ITER_COUNT 1024

typedef struct
{
    /*Must be cleared after context save*/
    unsigned char must_free;
    unsigned char tokenized;
    unsigned char *value;
    size_t quotes;
    size_t last_pos;
    string_tokenizer_info sti;
    section rhead;
    list_entry current;
} ancestor_status;


static int ancestor_filter(string_tokenizer_status *sts, void* pv)
{
    size_t *quotes = pv;

    if(sts->buf[sts->pos] == '"' || sts->buf[sts->pos] == '\'')
    {
        (*quotes)++;
    }

    if((*quotes) % 2 && sts->buf[sts->pos] == ';')
    {
        return (0);
    }
    else if(sts->buf[sts->pos] == ';')
    {
        return (1);
    }

    return (0);
}

static inline section ancestor_dispatch_section(void* r, section* shead, unsigned char flag)
{
    if(r == NULL || ((flag & 0x4) && (flag & 0x8) && (flag & 0x10)))
    {
        return (NULL);
    }

    switch(flag & 0x1C)
    {
        case XPANDER_REQUESTOR_OBJECT:
            *shead = &((surface_data*)((object*)r)->sd)->skhead;
            return (((object*)r)->os);

        case XPANDER_REQUESTOR_SOURCE:
            *shead = &((surface_data*)((source*)r)->sd)->skhead;
            return (((source*)r)->cs);

        case XPANDER_REQUESTOR_SURFACE:
            if(flag & 0x20)
            {
                *shead = &((surface_data*)r)->cd->shead;
                return (((surface_data*)r)->scd);
            }
            else
            {
                *shead = &((surface_data*)r)->skhead;
                return (((surface_data*)r)->spm);
            }

        default:
            return (NULL);
    }
}

int ancestor_destroy_queue(void **qh)
{
    if(qh == NULL)
    {
        return(-1);
    }

    ancestor_queue *aq = NULL;
    ancestor_queue *taq = NULL;

    list_enum_part_safe(aq, taq, (list_entry*)*qh, current)
    {
        linked_list_remove(&aq->current);
        sfree((void**)&aq);
    }

    sfree(qh);

    return(0);
}

void *ancestor_fusion(void *r, unsigned char  *npm, unsigned char xpander_flags, unsigned char gq)
{
    unsigned short iter = 0;
    list_entry status_stack;
    section shead = NULL;
    void *result = NULL;
    ancestor_status *as = zmalloc(sizeof(ancestor_status));
    list_entry_init(&as->current);


    list_entry_init(&status_stack);

    as->rhead = ancestor_dispatch_section(r, &shead, xpander_flags); //obtain the section of the calling item (source/object)

    //Retrieve the list with ancestors
    while(iter < ANCESTOR_MAX_ITER_COUNT && as)
    {
        unsigned char stop = 0;         //self-explanatory (it stops the lookup)
        unsigned char step_in = 0;

        iter++;

        key k = skeleton_get_key(as->rhead, "Ancestor");                 //check and obtain the Ancestor list from the current section
        unsigned char* kv = skeleton_key_value(k);     //try to obtain a value

        if(kv && as->tokenized == 0)
        {
            //Go and expand the value if needed
            xpander_request xr =
            {
                .os = kv,
                .es = NULL,
                .req_type = xpander_flags,
                .requestor = r
            };

            if(xpander(&xr))
            {
                as->value = xr.es;
                as->must_free = 1;
            }
            else
            {
                as->value = kv;
            }
        }

        /*Good, let's set the string tokenizer parameters*/
        if(as->tokenized == 0)
        {
            as->sti.buffer = as->value;
            as->sti.oveclen = 0;
            as->sti.ovecoff = NULL;
            as->sti.string_tokenizer_filter = ancestor_filter;
            as->sti.filter_data = &as->quotes;
            string_tokenizer(&as->sti);
            as->tokenized = 1;

            if(gq)
            {
                as->last_pos = 0;
            }
            else
            {
                as->last_pos = (as->sti.oveclen / 2) ? (as->sti.oveclen / 2) - 1 : 0;
            }
        }

        //Look through the ancestors

        for(size_t i = as->last_pos;
                as->sti.ovecoff && (gq ? i < as->sti.oveclen / 2 : 1) ;
                gq ? (i++, as->last_pos++) : (i--, as->last_pos--))
        {
            size_t start = as->sti.ovecoff[2 * i];
            size_t end   = as->sti.ovecoff[2 * i + 1];

            //do some adjustments
            if(string_strip_space_offsets(as->value, &start, &end) == 0)
            {
                if(as->value[start] == ';')
                {
                    start++;
                }
            }

            if(string_strip_space_offsets(as->value, &start, &end) == 0)
            {
                if(as->value[start] == '"' || as->value[start] == '\'')
                    start++;

                if(as->value[end - 1] == '"' || as->value[end - 1] == '\'')
                    end--;
            }

//--------------------------------------------------------------------------------------
            //We have arranged the offsets so we're good to go

            unsigned char *tsn = zmalloc((end - start) + 1);
            strncpy(tsn, as->value + start, end - start);
            section cs = skeleton_get_section(shead, tsn);  //check if we have a section
            sfree((void**)&tsn);
//--------------------------------------------------------------------------------------
            key ank = skeleton_get_key(cs, "Ancestor");
            k = (gq ? ank : skeleton_get_key(cs, npm));

//--------------------------------------------------------------------------------------
            if(gq && cs) //found an entry so we add it to the queue (if we're generating a queue ;) )
            {
                if(result == NULL)
                {
                    result = zmalloc(sizeof(list_entry));
                    list_entry_init((list_entry*)result);
                }

                ancestor_queue *aq = zmalloc(sizeof(ancestor_queue));
                list_entry_init(&aq->current);
                aq->s = cs;
                linked_list_add(&aq->current, result);
            }

//---------------------------------------------------------------------------------------
            if(gq == 0 && k) //we've got our ancestor so we'll just leave
            {
                result = (void*)skeleton_key_value(k);
                stop = 1;
                break;
            }
            else if(cs != NULL && ank) //save the state and prepare to dig deeper if we find that the section has ancestors
            {
                if(gq)
                    as->last_pos++;
                else if(as->last_pos)
                    as->last_pos--;

                linked_list_add_last(&as->current, &status_stack);
                as = zmalloc(sizeof(ancestor_status));
                as->rhead = cs;
                list_entry_init(&as->current);
                step_in = 1;
                break;
            }
            else if(gq == 0 && i == 0)
            {
                stop = 1;
                break;
            }
        }

        if(step_in && stop == 0)
        {
            continue;
        }

        //free this context
        linked_list_remove(&as->current);

        if(as->must_free)
        {
            sfree((void**)&as->value);
        }

        sfree((void**)&as->sti.ovecoff);
        sfree((void**)&as);

        if(linked_list_empty(&status_stack))
        {
            as = element_of(status_stack.prev, ancestor_status, current); //pop the previous context
        }

        if(stop || linked_list_empty(&status_stack))
        {
            break;
        }
    }


    ancestor_status *pcas = NULL;
    list_enum_part_safe(as, pcas, &status_stack, current)
    {
        if(as->must_free)
        {
            sfree((void**)&as->value);
        }

        linked_list_remove(&as->current);
        sfree((void**)&as->sti.ovecoff);
        sfree((void**)&as);
    }

    if(iter >= ANCESTOR_MAX_ITER_COUNT)
    {
        diag_verb("%s Reached the maximum ancestor count", __FUNCTION__);
    }

    return(result);
}
