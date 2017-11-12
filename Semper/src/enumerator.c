/*
 * Enumerator
 * Part of Project "Semper"
 * Written by Alexandru-Daniel Mărgărit
 */

/* The purpose of the enumerator is to be able to get the all the keys of the current section
 *and from the ancestors.
 * Think of it as a combination of skeleton_first_key/skeleton_next_key with the ability to also obtain the keys from
 *ancestors.
 */

#include <skeleton.h>
#include <ancestor.h>
#include <mem.h>
#include <surface.h>
#include <xpander.h>
#include <parameter.h>
#include <objects/object.h>
#include <sources/source.h>

typedef struct
{
    void* anc; //ancestor queue
    section s; // starting section
    section as; // ancestor section
    key k;      //current key
    section head; //skeleton head
} enumerator;

unsigned char* enumerator_first_value(void* r, int rt, void** ed)
{
    if(r == NULL)
    {
        return (NULL);
    }
    if(*ed == NULL)
    {
        *ed = zmalloc(sizeof(enumerator));
    }

    enumerator* e = *ed;
    section s = (rt ? ((object*)r)->os : ((source*)r)->cs);
    surface_data* sd = (rt ? ((object*)r)->sd : ((source*)r)->sd);
    e->head = &sd->skhead;

    if(e->anc)
    {
        ancestor_destroy_queue(&e->anc);
    }

    e->s = s;
    e->anc=ancestor_build_queue(r,rt ? XPANDER_OBJECT : XPANDER_SOURCE);

    if(e->anc == NULL)
    {
        e->k = skeleton_first_key(s);
        return (skeleton_key_name(e->k));
    }
    else
    {
        if(linked_list_empty((list_entry*)e->anc))
        {
            return(NULL);
        }
        ancestor_queue *aq=element_of(((list_entry*)e->anc)->prev,ancestor_queue,current);
        linked_list_remove(&aq->current);
        e->as=aq->s;
        sfree((void**)&aq);
        e->k = skeleton_first_key(e->as);
        return (skeleton_key_name(e->k));
    }

    return (NULL);
}

unsigned char* enumerator_next_value(void* ed)
{
    enumerator* e = ed;

    if(e->as)
    {
        e->k = skeleton_next_key(e->k, e->as);
        if(e->k == NULL)
        {

            if(!(linked_list_empty((list_entry*)e->anc)))
            {
                ancestor_queue *aq=element_of(((list_entry*)e->anc)->prev,ancestor_queue,current);
                linked_list_remove(&aq->current);
                e->as=aq->s;
                sfree((void**)&aq);
                e->k = skeleton_first_key(e->as);
                return (skeleton_key_name(e->k));
            }
            else
            {
                e->as = NULL;
                e->k = skeleton_first_key(e->s);
                return (skeleton_key_name(e->k));
            }
        }
        else
        {
            return (skeleton_key_name(e->k));
        }
    }
    e->k = skeleton_next_key(e->k, e->s);
    return (skeleton_key_name(e->k));
}

void enumerator_finish(void** ed)
{
    enumerator* e = *ed;
    if(e->anc)
    {
        ancestor_destroy_queue(&e->anc);
    }
    sfree(ed);
}
