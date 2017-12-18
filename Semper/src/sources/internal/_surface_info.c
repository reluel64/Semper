/*Surface source*/

#include <sources/source.h>
#include <semper_api.h>
#include <semper.h>
#include <surface.h>
#include <mem.h>
#include <string_util.h>
#include <skeleton.h>
typedef struct _surface_info
{
    control_data* cd;
    unsigned char data;
    unsigned char* sname;
    unsigned char *str;
    unsigned char coord; //0 - x | 1- y
    unsigned char def[64];
} surface_info;

void surface_info_init(void** spv, void* ip)
{

    *spv = zmalloc(sizeof(surface_info));
    surface_info* si = *spv;
    source* s = ip;
    surface_data* sd = s->sd;
    si->cd = sd->cd;
}

void surface_info_reset(void* spv, void* ip)
{

    surface_info* si = spv;
    unsigned char *temp=param_string("Info",EXTENSION_XPAND_ALL,ip,"Coordinates");

    if(temp)
    {
        if(!strcasecmp(temp,"Coordinates"))
        {
            si->data=0;
            si->coord=param_bool("CoordIndex",ip,0);
        }
        else if(!strcasecmp(temp,"General"))
            si->data=1;
    }

    sfree((void**)&si->sname);
    si->sname = clone_string(param_string("Surface_Name", EXTENSION_XPAND_VARIABLES, ip, NULL));

}

static unsigned char *surface_info_meta(surface_data *sd,unsigned char *field)
{
    section s=skeleton_get_section(&sd->skhead,"Surface-Meta");
    key k=skeleton_get_key(s,field);
    return(skeleton_key_value(k));
}

double surface_info_update(void* spv)
{

    surface_info* si = spv;
    surface_data* sd = surface_by_name(si->cd, si->sname);
    size_t  flags=0;

    if(sd == NULL)
        return (0.0);

    if(si->data==1)
    {
        flags|=((size_t)sd->keep_on_screen)                          <<0;
        flags|=((size_t)sd->draggable)                               <<1;
        flags|=((size_t)sd->snp)                                     <<2;
        flags|=((size_t)sd->rim)                                     <<3;
        flags|=((size_t)sd->clkt)                                    <<4;
        flags|=((size_t)(surface_info_meta(sd,"Name")!=NULL))        <<5;
        flags|=((size_t)(surface_info_meta(sd,"Author")!=NULL))      <<6;
        flags|=((size_t)(surface_info_meta(sd,"Version")!=NULL))     <<7;
        flags|=((size_t)(surface_info_meta(sd,"License")!=NULL))     <<8;
        flags|=((size_t)(surface_info_meta(sd,"Information")!=NULL)) <<9;
    }
    else
    {
        return ((double)(si->coord?sd->y:sd->x));
    }

    return ((double)flags);
}

unsigned char* surface_info_string(void* spv)
{

    surface_info* si = spv;
    surface_data* sd = surface_by_name(si->cd, si->sname);
    memset(si->def, 0, sizeof(si->def));

    if(si->data==0)
    {
        return(NULL);
    }

    if(sd==NULL)
    {
        return("");
    }

    if(si->data==1)
    {
        sfree((void**)&si->str);
        size_t nm=0;
        size_t bpos=0;
        unsigned char *t=NULL;
        static unsigned char  *name_tbl[]=
        {
            "Name",
            "Author",
            "Version",
            "License",
            "Information"
        };

        for(unsigned char i=0; i<sizeof(name_tbl)/sizeof(void *); i++)
        {
            t=surface_info_meta(sd,name_tbl[i]);

            if(t)
            {
                nm+=string_length(name_tbl[i])+string_length(t)+3; //allocate extra space for newline,space and null
            }
        }

        si->str=zmalloc(nm+1);

        for(unsigned char i=0; i<sizeof(name_tbl)/sizeof(void *); i++)
        {
            t=surface_info_meta(sd,name_tbl[i]);

            if(t)
            {
                if(bpos)
                {
                    si->str[bpos++]='\n';
                }

                bpos+=snprintf(si->str+bpos,nm-bpos,"%s: %s",name_tbl[i],t);
            }
        }

        return (si->str);
    }

    return(NULL);
}

void surface_info_destroy(void** spv)
{

    surface_info* s = *spv;
    sfree((void**)&s->str);
    sfree((void**)&s->sname);
    sfree(spv);
}
