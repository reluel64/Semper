/*
 * Part of Project 'Semper'
 * This file contains code that manages the internal view (skeleton) of an ini file
 * Written by Alexandru-Daniel Mărgărit
 *
 */

#include <skeleton.h>
#include <mem.h>
#include <string_util.h>
#include <linked_list.h>

typedef struct _key
{
    unsigned char* kn;
    unsigned char* kv;
    list_entry current;
} internal_key;

typedef struct _section
{
    unsigned char* sn;
    list_entry current;
    list_entry keys;
} internal_section;

section skeleton_add_section(section shead, unsigned char* sn)
{
    if(shead == NULL)
        return (NULL);

    list_entry* lh = shead;
    internal_section* is = NULL;

    list_enum_part(is, lh, current)
    {
        if((is->sn && sn && !strcasecmp(sn, is->sn)) || (sn == NULL && is->sn == NULL))
        {
            return (is);
        }
    }

    is = zmalloc(sizeof(internal_section));
    list_entry_init(&is->current);
    list_entry_init(&is->keys);
    linked_list_add_last(&is->current, shead);
    is->sn = clone_string(sn);
    return (is);
}

key skeleton_add_key(section s, unsigned char* kn, unsigned char* kv)
{
    if(s == NULL || kn == NULL)
    {
        return (NULL);
    }

    internal_section* is = s;
    internal_key* ik = NULL;
    list_enum_part(ik, &is->keys, current)
    {
        if(kn && ik->kn && !strcasecmp(kn, ik->kn))
        {
            sfree((void**)&ik->kv);
            ik->kv = clone_string(kv);
            return (ik);
        }
    }
    ik = zmalloc(sizeof(internal_key));
    list_entry_init(&ik->current);
    ik->kn = clone_string(kn);
    ik->kv = clone_string(kv);
    linked_list_add_last(&ik->current, &is->keys);
    return (ik);
}

section skeleton_get_section(section shead, unsigned char* sn)
{
    if(shead == NULL || sn == NULL)
    {
        return (NULL);
    }

    list_entry* lh = shead;
    internal_section* is = NULL;
    list_enum_part(is, lh, current)
    {
        if(is->sn && sn && !strcasecmp(sn, is->sn))
        {
            return (is);
        }
    }
    return (NULL);
}

section skeleton_first_section(section shead)
{
    list_entry* lh = shead;

    if(lh==NULL)
    {
        return(NULL);
    }

    internal_section* rs = element_of(lh->next, internal_section, current);

    if(&rs->current == lh)
    {
        return (NULL);
    }

    return (rs);
}

section skeleton_next_section(section s, section shead)
{
    if(s == NULL || shead == NULL)
    {
        return (NULL);
    }

    internal_section* is = s;

    if(is->current.next == shead)
    {
        return (NULL);
    }

    internal_section* rs = element_of(is->current.next, internal_section, current);
    return (rs);
}

unsigned char* skeleton_get_section_name(section s)
{
    if(!s)
    {
        return (NULL);
    }

    internal_section* is = s;
    return (is->sn);
}

key skeleton_get_key(section s, unsigned char* kn)
{
    if(s == NULL || kn == NULL)
    {
        return (NULL);
    }

    internal_section* is = s;
    internal_key* ik = NULL;

    list_enum_part(ik, &is->keys, current)
    {
        if(ik->kn && kn && !strcasecmp(kn, ik->kn))
        {
            return (ik);
        }
    }
    return (NULL);
}

key skeleton_get_key_n(section s, unsigned char* kn,size_t n)
{
    if(s == NULL || kn == NULL)
    {
        return (NULL);
    }

    internal_section* is = s;
    internal_key* ik = NULL;

    list_enum_part(ik, &is->keys, current)
    {
        if(ik->kn && kn && !strncasecmp(kn, ik->kn,n))
        {
            return (ik);
        }
    }
    return (NULL);
}

key skeleton_first_key(section s)
{
    if(s == NULL)
    {
        return (NULL);
    }

    internal_section* is = s;

    if(is->keys.next == &is->keys)
    {
        return (NULL);
    }

    internal_key* ik = element_of(is->keys.next, internal_key, current);

    return (ik);
}

key skeleton_next_key(key k, section s)
{
    if(!k || !s)
    {
        return (NULL);
    }

    internal_section* is = s;
    internal_key* ik = k;

    if(ik->current.next == &is->keys)
    {
        return (NULL);
    }

    internal_key* rk = element_of(ik->current.next, internal_key, current);

    return (rk);
}

unsigned char* skeleton_key_value(key k)
{
    if(k == NULL)
    {
        return (NULL);
    }

    internal_key* ik = k;
    return (ik->kv);
}

unsigned char* skeleton_key_name(key k)
{
    if(k == NULL)
    {
        return (NULL);
    }

    internal_key* ik = k;
    return (ik->kn);
}

void skeleton_key_remove(key* k)
{
    if(k&&*k)
    {
        internal_key* ik = *k;
        linked_list_remove(&ik->current);
        sfree((void**)&ik->kn);
        sfree((void**)&ik->kv);
        sfree((void**)k);
    }
}

void skeleton_remove_section(section* s)
{
    if(s&&*s)
    {
        internal_section* is = *s;
        sfree((void**)&is->sn);
        list_entry* temp = NULL;
        list_entry* pos = NULL;

        list_enum_safe(pos, temp, (&is->keys))
        {
            internal_key* k = element_of(pos, internal_key, current);
            skeleton_key_remove((void**)&k);
        }

        linked_list_remove(&is->current);
        sfree(s);
    }
}

int skeleton_destroy(section shead)
{
    if(shead==NULL)
    {
        return(-1);
    }

    list_entry* sle = shead;
    list_entry* temp = NULL;
    list_entry* pos = NULL;

    list_enum_safe(pos, temp, sle)
    {
        internal_section* sec = element_of(pos, internal_section, current);
        skeleton_remove_section((void**)&sec);
    }

    return (0);
}
