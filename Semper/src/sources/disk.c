/*
 * Disk usage source
 * Part of Project Smeper
 * Written by Alexandru-Daniel Mărgărit
 */
#include <sources/disk.h>
#include <mem.h>
#include <semper_api.h>
#include <string_util.h>
#ifdef WIN32
#include <windows.h>
#elif __linux__
#include <sys/statfs.h>
#endif

#ifdef __linux__
static unsigned char disk_check_removable(unsigned char *p)
{
    unsigned char removable=0;
    unsigned char path[256]= {0};
    unsigned char base[32]= {0};
    sscanf(p,"/dev/%[A-z]31s",base);
    snprintf(path,256,"/sys/block/%s/removable",base);
    FILE *f=fopen(path,"r");

    if(f)
    {
        removable=fgetc(f)!='0';
        fclose(f);
    }
    else
        removable=2;

    return(removable);
}
#endif

void disk_create(void** spv, void* ip)
{
    unused_parameter(ip);
    disk* d = zmalloc(sizeof(disk));
    *spv = d;
}

void disk_reset(void* spv, void* ip)
{
    disk* d = spv;

    sfree((void**)&d->name);

    d->name = clone_string(param_string("Disk", EXTENSION_XPAND_SOURCES | EXTENSION_XPAND_VARIABLES, ip, NULL));
    d->label = param_bool("Label", ip, 0);


#ifdef WIN32

    if(d->total_bytes == NULL)
    {
        d->total_bytes = zmalloc(sizeof(ULARGE_INTEGER));
    }

    if(d->free_bytes == NULL)
    {
        d->free_bytes = zmalloc(sizeof(ULARGE_INTEGER));
    }

    unsigned short* buf = utf8_to_ucs(d->name);
    GetDiskFreeSpaceExW(buf, NULL, d->total_bytes, NULL);
    sfree((void**)&buf);
    source_set_max((double)((ULARGE_INTEGER*)d->total_bytes)->QuadPart, ip, 1, 0);
#elif __linux__
    struct statfs s= {0};

    if(d->total_bytes==NULL)
    {
        d->total_bytes=zmalloc(sizeof(size_t));
    }

    if(d->free_bytes==NULL)
    {
        d->free_bytes=zmalloc(sizeof(size_t));
    }

    if(statfs(d->name,&s)==0)
    {
        ((size_t*)d->total_bytes)[0]=s.f_blocks*s.f_bsize;
    }

#endif
    d->type = param_bool("Type", ip, 0);
    d->total = param_bool("Total", ip, 0);

}

double disk_update(void* spv)
{
    disk* d = spv;


#ifdef WIN32

    if(d->type == 0)
    {
        unsigned short* buf = utf8_to_ucs(d->name);
        memset(d->total_bytes, 0, sizeof(ULARGE_INTEGER));
        memset(d->free_bytes, 0, sizeof(ULARGE_INTEGER));
        GetDiskFreeSpaceExW(buf, NULL, d->total_bytes, d->free_bytes);
        sfree((void**)&buf);

        if(!d->total)
        {
            return ((double)((ULARGE_INTEGER*)d->free_bytes)->QuadPart);
        }
        else
        {
            return ((double)((ULARGE_INTEGER*)d->total_bytes)->QuadPart);
        }
    }
    else
    {
        unsigned short* buf = utf8_to_ucs(d->name);
        double ret = GetDriveTypeW(buf);
        sfree((void**)&buf);
        return (ret);
    }

#elif __linux__

    if(d->type==0)
    {
        struct statfs s= {0};

        if(statfs(d->name,&s)==0)
        {
            ((size_t*)d->total_bytes)[0]=s.f_blocks*s.f_bsize;
            ((size_t*)d->free_bytes)[0]=s.f_bfree*s.f_bsize;
        }
        else
        {
            ((size_t*)d->total_bytes)[0]=0;
            ((size_t*)d->free_bytes)[0]=0;
        }

        if(d->total)
        {

            return((double)((size_t*)d->total_bytes)[0]);
        }
        else
        {

            return((double)((size_t*)d->free_bytes)[0]);
        }
    }

#endif
    return (0.0);
}

unsigned char* disk_string(void* spv)
{
    disk* d = spv;

    static unsigned char* types[] =
    {
        "Unknown",
        "Removed",
        "Removable",
        "Fixed",
        "Network",
        "CD-ROM",
        "RAM-Disk"
    };

    if(d->type && d->label == 0)
    {

#ifdef WIN32
        return (types[GetDriveType(d->name)]);
#elif __linux__
        unsigned char removable=disk_check_removable(d->name);
        // (removable==0)?return(types[3]):0;
        //  (removable==1)?return(types[2]):0;
        // (removable==2)?return(types[1]):0;
        return(NULL);
#endif
    }
    else if(d->type == 0 && d->label)
    {
        sfree((void**)&d->ret_str);
        unsigned short buf[256];
        unsigned short* buf2 = utf8_to_ucs(d->name);
        memset(buf, 0, sizeof(buf));
#ifdef WIN32
        GetVolumeInformationW(buf2, buf, 255, NULL, NULL, NULL, NULL, 0);
#endif
        d->ret_str = ucs_to_utf8(buf, NULL, 0);
        sfree((void**)&buf2);
        return (d->ret_str);
    }

    return (NULL);
}

void disk_destroy(void** spv)
{
    disk* d = *spv;
    sfree((void**)&d->free_bytes);
    sfree((void**)&d->name);
    sfree((void**)&d->ret_str);
    sfree((void**)&d->total_bytes);
    sfree(spv);
}
