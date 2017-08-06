/*
 * Iterator source
 * Part of Project Semper
 * Written by Alexandru-Daniel Mărgărit
 */

#include <sources/iterator.h>
#include <sources/extension.h>
#include <mem.h>

void iterator_init(void** spv, void* ip)
{
    unused_parameter(ip);
    iterator_info* ii = zmalloc(sizeof(iterator_info));
    *spv = ii;
}

void iterator_reset(void* spv, void* ip)
{
    iterator_info* ii = spv;

    ii->start_value = (long)extension_double("IteratorStart", ip, 0.0);
    ii->end_value = (long)extension_double("IteratorStop", ip, 100.0);
    ii->step = (long)extension_double("IteratorStep", ip, 1.0);
    ii->rep_count = extension_size_t("IteratorCount", ip, 0);
    ii->current_value = ii->start_value;
    extension_set_min((double)ii->start_value, ip, 1, 1);
    extension_set_max((double)ii->end_value, ip, 1, 1);
}

double iterator_update(void* spv)
{
    iterator_info* ii = spv;
    double ret = ii->current_value;

    if(ii->stop)
    {
        return (ret);
    }

    if((ii->curent_count != ii->rep_count) || ii->rep_count == 0)
    {
        if(ii->current_value == ii->end_value)
        {
            ii->curent_count++;                    // the loop was ended so we will increment the counter
            ii->current_value = ii->start_value;   // reset the current value
        }
        else
        {
            ii->current_value += ii->step;
        }
    }
    else
    {
        return ((double)ii->end_value);
    }

    return (ret);
}

void iterator_command(void* spv, unsigned char* command)
{
    if(command && spv)
    {
        iterator_info* ii = spv;
        if(strcasecmp(command, "Start") == 0)
        {
            ii->stop = 0;
        }
        if(strcasecmp(command, "Stop") == 0)
        {
            ii->stop = 1;
        }
    }
}

void iterator_destroy(void** spv)
{
    sfree(spv);
}


