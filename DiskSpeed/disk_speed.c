#include <stdio.h>
#include <SDK/extension.h>
#include <stdlib.h>
#include <string.h>
typedef enum
{
    write_speed,
    read_speed,
    write_total,
    read_total,
    rw_total,
    rw_speed
} disk_speed_type;

typedef struct
{
    disk_speed_type dst;
    unsigned char *disk_name;
    size_t value;
    size_t old_value;
} disk_speed;



static double disk_speed_parse(disk_speed *ds)
{
    unsigned char buf[256]= {0};
    FILE *f=fopen("/proc/diskstats","r");
    size_t calc_val=0;
    unsigned char part=0;
    if(f==NULL)
        return(0.0);
#if 0
    if(ds->disk_name)
    {
        for(size_t i=0;ds->disk_name[i];i++)
        {
            if(isdigit(ds->disk_name[i]))
            {
                part=1;
                break;
            }
        }
    }
#endif
    while(fgets(buf,256,f))
    {
        size_t major=0;
        size_t minor=0;
        unsigned char dev_name[64]= {0};
        size_t r_completed=0;
        size_t r_merged=0;
        size_t sec_read=0;
        size_t time_read=0;
        size_t w_completed;
        size_t w_merged=0;
        size_t sec_write=0;
        size_t time_write=0;
        size_t dev_len=0;
        sscanf(buf,"%lu%lu%s%lu%lu%lu%lu%lu%lu%lu%lu",&major,&minor,dev_name,&r_completed,&r_merged,&sec_read,&time_read,&w_completed,&w_merged,&sec_write,&time_write);
        dev_len=strlen(dev_name);

        if(ds->disk_name==NULL&&isdigit(dev_name[dev_len-1]))
            continue;
        else if(ds->disk_name&&strcasecmp(dev_name,ds->disk_name))
            continue;

        switch(ds->dst)
        {
        case read_total:
        case read_speed:
            calc_val+=sec_read;
            break;
        case write_total:
        case write_speed:
            calc_val+=sec_write;
            break;
        case rw_speed:
        case rw_total:
            calc_val+=(sec_write+sec_read);
            break;
        default:
            calc_val=0;
            break;
        }
    }
    
    switch(ds->dst)
    {
    case read_total:
    case write_total:
    case rw_total:
        ds->value=calc_val*512;
        ds->old_value=calc_val*512;
        break;
    case read_speed:
    case write_speed:
    case rw_speed:
        calc_val*=512;
        ds->value=(ds->old_value!=0)?(calc_val-ds->old_value):0;
        ds->old_value=calc_val;
        break;
    }
    fclose(f);
    return(0);
}


void extension_init_func(void **spv,void *ip)
{
    disk_speed *ds=malloc(sizeof(disk_speed));
    memset(ds,0,sizeof(disk_speed));
    unsigned char *s=extension_string("DiskName",EXTENSION_XPAND_ALL,ip,NULL);
    if(s)
        ds->disk_name=strdup(s);
    *spv=ds;
}

void extension_reset_func(void *spv,void *ip)
{
    disk_speed *ds=spv;
    unsigned char *s=extension_string("DiskSpeedType",EXTENSION_XPAND_ALL,ip,"RWTotal");

    if(s)
    {
        if(!strcasecmp("RWTotal",s))
            ds->dst=rw_total;
        else if(!strcasecmp("RWSpeed",s))
            ds->dst=rw_speed;
        else if(!strcasecmp("ReadTotal",s))
            ds->dst=read_total;
        else if(!strcasecmp("Readspeed",s))
            ds->dst=read_speed;
        else if(!strcasecmp("WriteTotal",s))
            ds->dst=write_total;
        else if(!strcasecmp("WriteSpeed",s))
            ds->dst=write_speed;
    }
}

double extension_update_func(void *spv)
{
    disk_speed *ds=spv;
    disk_speed_parse(spv);
    return((double)ds->value);
}

void extension_destroy_func(void **spv)
{
    disk_speed *ds=*spv;
    free(ds->disk_name);
    free(*spv);
    ;
}
