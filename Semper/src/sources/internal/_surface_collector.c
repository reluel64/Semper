/*Surface Collector source
 *Part of Project 'Semper'
 *For internal use only
 *Written by Alexandru-Daniel Mărgărit
 * */

#include <semper.h>
#include <sources/extension.h>
#include <sources/source.h>
#include <mem.h>
#include <string_util.h>
#include <linked_list.h>

typedef struct _surfaces_collector_data
{
    /*Parent*/
    size_t count;
    size_t base;
    size_t index;

    /*Child*/
    struct _surfaces_collector_data* parent;
    unsigned char* name;
    /*Common*/
    void *ip;
    void* cd;
} surfaces_collector_data;

void surfaces_collector_init(void** spv, void* ip)
{

    surfaces_collector_data* scd = zmalloc(sizeof(surfaces_collector_data));
    source* s = ip;
    surface_data* sd = s->sd;
    scd->cd = sd->cd;
    scd->ip=ip;
    *spv = scd;
}

void surfaces_collector_reset(void* spv, void* ip)
{

    surfaces_collector_data* scd = spv;
    void* p = extension_get_parent(extension_string("Parent", EXTENSION_XPAND_VARIABLES, ip, NULL), ip);

    scd->parent = extension_private(p);

    if(scd->parent)
        scd->index = (size_t)extension_double("ChildIndex", ip, 0);
}

double surfaces_collector_update(void* spv)
{

    surfaces_collector_data* scd = spv;
    control_data* cd = scd->cd;
    surface_data* sd = NULL;
    scd->count = 0;

    if(scd->parent)
        return ((scd->parent->base + scd->index < scd->parent->count));

    list_enum_part(sd, &cd->surfaces, current)
    {
        scd->count++;
    }

    return ((double)scd->count);
}

unsigned char* surfaces_collector_string(void* spv)
{

    surfaces_collector_data* scd = spv;
    control_data* cd = scd->cd;
    surface_data* sd = NULL;

    sfree((void**)&scd->name);

    if(!scd->parent)
    {
        return ("");
    }

    size_t i = scd->parent->base + scd->index;

    list_enum_part(sd, &cd->surfaces, current)
    {
        if(i && i < scd->parent->count)
            i--;
        else
            break;
    }

    if(sd && i == 0)
        scd->name = clone_string(sd->sp.surface_rel_dir);
    else
        return("");

    return (scd->name);
}

void surfaces_collector_destroy(void** spv)
{
    surfaces_collector_data* scd = *spv;
    sfree((void**)&scd->name);
    sfree(spv);
}

void surfaces_collector_command(void* spv, unsigned char* command)
{

    surfaces_collector_data* scd = spv;

    if(command && !scd->parent)
    {
        if(!strcasecmp("IndexUp", command) && scd->base + 1 < scd->count)
            scd->base++;
        else if(!strcasecmp("IndexDown", command) && scd->base > 0)
            scd->base--;
    }
    else if(command&&scd->parent)
    {
        if(strcasecmp("Unload",command)==0)
        {
            control_data* cd = scd->cd;
            size_t i = scd->parent->base + scd->index;
            surface_data* sd = NULL;
            list_enum_part(sd, &cd->surfaces, current)
            {
                if(i && i < scd->parent->count)
                    i--;
                else
                    break;
            }

            if(sd && i == 0)
            {
                size_t path_len=string_length(sd->sp.surface_rel_dir+1);

                unsigned char *temp=zmalloc(18+path_len); //space for null and for slash
                snprintf(temp,18+path_len,"unLoadSurface(%s)",sd->sp.surface_rel_dir);
                extension_send_command(scd->ip,temp);
                sfree((void**)&temp);
            }
        }
    }
}
