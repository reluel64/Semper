/*
 * Memory usage source
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */
#include <sources/memory.h>
#include <sources/source.h>
#include <sources/extension.h>
#include <mem.h>
#include <string_util.h>
#include <strings.h>
#ifdef WIN32
#include <windows.h>
#elif __linux__
#include <sys/sysinfo.h>
#endif

void memory_init(void** spv, void* ip)
{
    unused_parameter(ip);
    *spv = zmalloc(sizeof(unsigned char));
}
#ifdef __linux__

static size_t memory_get_linux(char *field)
{
    FILE *f=fopen("/proc/meminfo","r");
    unsigned char buf[256]= {0};
    size_t res=0;
    size_t fieldsz=string_length(field);
    if(f!=NULL)
    {
        while(fgets(buf,255,f))
        {
            if(!strncasecmp(field,buf,fieldsz))
            {
                unsigned char *unit=NULL;
                res= strtoull(buf+fieldsz,&unit,10);

                if(unit&&unit!=buf+fieldsz)
                {
                    switch(toupper(unit[1]))
                    {
                    case 'K':
                        res*=1024;
                        break;
                    case 'M':
                        res*=1024*1024;
                        break;
                    }
                }

                break;
            }
        }
        fclose(f);
    }
    return(res);
}
#endif

void memory_reset(void* spv, void* ip)
{
    unsigned char* type = spv;
    unsigned char* mode = extension_string("Memory", EXTENSION_XPAND_SOURCES | EXTENSION_XPAND_VARIABLES, ip, "Physical");
#ifdef WIN32
    MEMORYSTATUSEX ms = { 0 };
    ms.dwLength = sizeof(MEMORYSTATUSEX);

    GlobalMemoryStatusEx(&ms);
#endif


    if(mode)
    {
        if(strcasecmp(mode, "Physical") == 0)
        {
#ifdef WIN32
            extension_set_max((double)ms.ullTotalPhys, ip, 1, 1);
#elif __linux__
            extension_set_max((double)memory_get_linux("MemTotal:"), ip, 1, 1);
#endif
            *type = 0;
        }
        else if(strcasecmp(mode, "Virtual") == 0)
        {
#ifdef WIN32
            extension_set_max((double)ms.ullTotalVirtual, ip, 1, 1);
#endif
            *type = 1;
        }
        else if(strcasecmp(mode, "Pagefile") == 0)
        {
#ifdef WIN32
            extension_set_max((double)ms.ullTotalPageFile, ip, 1, 1);
#endif
            *type = 2;
        }
    }


    *type |= (extension_bool("Total", ip, 0) << 2);
}

double memory_update(void* spv)
{
    unsigned char* type = spv;
#ifdef WIN32
    MEMORYSTATUSEX ms = { 0 };
    ms.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&ms);

    switch(*type & 0xfb)
    {
    case 0:
        return (*type & 0x4 ? (double)ms.ullTotalPhys : (double)(ms.ullTotalPhys - ms.ullAvailPhys));
    case 1:
        return (*type & 0x4 ? (double)ms.ullTotalVirtual : (double)(ms.ullTotalVirtual - ms.ullAvailVirtual));
    case 2:
        return (*type & 0x4 ? (double)ms.ullTotalPageFile : (double)(ms.ullTotalPageFile - ms.ullAvailPageFile));
    }
#elif __linux__
    size_t total=memory_get_linux("MemTotal:");
    size_t free=memory_get_linux("MemFree:")+memory_get_linux("Cached:")+memory_get_linux("Buffers:");
    
    switch(*type & 0xfb)
    {
    case 0:
        return ((*type & 0x4 ? (double)total : (double)(total- free)));
         //case 1:
        //return (*type & 0x4 ? (double)ms.ullTotalVirtual : (double)(ms.ullTotalVirtual - ms.ullAvailVirtual));
        // case 2:
        //return (*type & 0x4 ? (double)inf.totalswap : (double)(inf.totalswap - inf.freeswap));
    }
#endif
    return (0.0);
}

void memory_destroy(void **spv)
{
    sfree(spv);
}
