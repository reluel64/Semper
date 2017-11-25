/*
 * Average list routines
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */
#include <sources/source.h>
#include <mem.h>

static inline void average_destroy_value(avg_val** av)
{
    linked_list_remove(&(*av)->current);
    sfree((void**)av);
}

void average_destroy(average** a)
{
    average* ta = *a;

    if(ta)
    {
        avg_val* av = NULL;
        avg_val* tav = NULL;
        list_enum_part_safe(av, tav, &ta->values, current)
        {
            average_destroy_value(&av);
        }
        sfree((void**)a);
    }
}

static int average_reset(average* a, size_t avg_count)
{
    a->target = avg_count;
    avg_val* av = NULL;
    avg_val* tav = NULL;

    list_enum_part_backward_safe(av, tav, &a->values, current)
    {
        if((a->target >= a->count) || a->count == 0)
        {
            break;
        }

        average_destroy_value(&av);
        a->count--;
    }
    return (1);
}

double average_update(average** avg, size_t avg_count, double value)
{
    if(avg_count == 0)
    {
        return (value);
    }

    if(*avg == NULL)
    {
        *avg = zmalloc(sizeof(average));
        list_entry_init(&(*avg)->values);
    }

    average* a = *avg;

    average_reset(a, avg_count);

    if(a->count < a->target)
    {
        avg_val* av = zmalloc(sizeof(avg_val));
        list_entry_init(&av->current);
        av->value = value;
        a->count++;
        a->total += value;
        linked_list_add(&av->current, &a->values);
    }
    else
    {
        double old_value = 0.0;
        double new_value = value;
        avg_val* av = zmalloc(sizeof(avg_val));
        list_entry_init(&av->current);
        avg_val* pav = element_of(a->values.prev, avg_val, current);
        old_value = pav->value;
        a->total -= old_value;
        a->total += new_value;

        av->value = value;
        average_destroy_value(&pav);
        linked_list_add(&av->current, &a->values);
    }

    return (a->total / (double)a->count);
}
