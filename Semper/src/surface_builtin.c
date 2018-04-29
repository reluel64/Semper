/*
 * Built-in surface support
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 * */

#include <semper.h>
#include <ini_parser.h>
#include <surface.h>
#include <objects/object.h>
#include <sources/source.h>
#include <mem.h>
#include <string_util.h>
#include <surface_builtin.h>
#include <event.h>

static inline unsigned char* surface_builtin_code(size_t* size,surface_builtin_type tp)
{
    static char catalog_code[] =
    {
#include <registry.h>
    };
    static char tooltip_code[]=
    {
#include <tooltip.h>
    };

    if(size)
    {
        switch(tp)
        {
        default:
        {
            return(NULL);
        }

        case catalog:
        {
            *size=sizeof(catalog_code);
            return(catalog_code);
        }

        case tooltip:
        {
            *size=sizeof(tooltip_code);
            return(tooltip_code);
        }
        }
    }

    return (NULL);
}


int surface_builtin_init(void *holder,surface_builtin_type tp)
{

    if(holder==NULL)
    {
        return(-1);
    }

    switch(tp)
    {
    case catalog:
    {
        control_data *cd=holder;

        if(cd->srf_reg == NULL)
        {
            long w = 0;
            long h = 0;
            long x = 0;
            long y = 0;
            size_t buf_sz=0;
            unsigned char *buf=surface_builtin_code(&buf_sz,tp);
            surface_data* sd =surface_load_memory(cd,buf,buf_sz,NULL);
            crosswin_set_monitor(sd->sw,0);
            crosswin_monitor_resolution(&cd->c,sd->sw, &w, &h);
            crosswin_monitor_origin(&cd->c,sd->sw,&x,&y);
            crosswin_set_position(sd->sw, (x+w / 2) - sd->w / 2, (y+h / 2) - sd->h / 2);
            cd->srf_reg = sd;
            return(0);
        }

        break;

    }

    case tooltip:
    {
        object *o=holder;
        surface_data *osd=o->sd;
        control_data *cd=osd->cd;

        if(o->ttip == NULL&&(o->ot.text||o->ot.title))
        {
            size_t buf_sz=0;
            unsigned char *buf=surface_builtin_code(&buf_sz,tp);
            surface_data* sd =surface_load_memory(cd,buf,buf_sz,NULL);
            sd->hidden=1;
            crosswin_set_position(sd->sw, (o->x+osd->x),  o->y+o->h+osd->y);

            crosswin_click_through(sd->sw,1);

            crosswin_set_window_z_order(sd->sw,crosswin_topmost);
            o->ttip=sd;
            crosswin_hide(sd->sw);
            object_tooltip_update(o);

            return(0);
        }

        break;
    }
    }

    return (-1);
}

int surface_builtin_destroy(void **pv)
{
    surface_destroy(*pv);
    *pv=NULL; /*set this to null as the surface will die on it's own*/
    return (0);
}

void surface_registry_dump(void)
{
    FILE* f = fopen("registry.ini", "w");

    if(f)
    {
        size_t buf_sz = 0;
        unsigned char* buf = surface_builtin_code(&buf_sz,catalog);
        fwrite(buf, 1, buf_sz, f);
        fclose(f);
    }
}

int surface_registry_file_to_string(unsigned char* s, unsigned char* d)
{
    if(s == NULL || d == NULL)
        return (-1);

    FILE* sf = fopen(s, "r");
    FILE* df = fopen(d, "w");

    if(sf && df)
    {
        unsigned char buf = 0;
        unsigned char* nl = "\\n";
        fputc('"', df);

        while(fread(&buf, 1, 1, sf))
        {
            if(buf == '\n')
                fwrite(nl, 2, 1, df);
            else if(buf=='"')
            {
                fputc('\\',df);
                fputc('"',df);
            }
            else
            {
                fwrite(&buf, 1, 1, df);
            }
        }

        fputc('"', df);
        fclose(sf);
        fclose(df);
        return (0);
    }

    if(sf)
    {
        fclose(sf);
    }

    if(df)
    {
        fclose(df);
    }

    return (-1);
}
