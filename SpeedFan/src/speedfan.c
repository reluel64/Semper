/* Ping source
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */
#include <stdlib.h>
#include <SDK/extension.h>
#include <windows.h>
#define MAX_LEN 32

#pragma pack(push, 1)
typedef struct
{
    unsigned short version;
    unsigned short flags;
    int MemSize;
    int handle;
    unsigned short NumTemps;
    unsigned short NumFans;
    unsigned short NumVolts;
    int temps[32];
    int fans[32];
    int volts[32];
} speedfan_shared_memory;

#pragma pack(pop)
typedef struct
{
    unsigned char senz_type;
    unsigned char senz_index;
} speedfan_data;


void extension_init_func(void **spv,void *ip)
{
    speedfan_data *spd=malloc(sizeof(speedfan_data));
    memset(spd,0,sizeof(speedfan_data));
    *spv=spd;
}

void extension_reset_func(void *spv,void *ip)
{
    speedfan_data *spd=spv;
    unsigned char *type=extension_string("SensorType",EXTENSION_XPAND_SOURCES|EXTENSION_XPAND_VARIABLES,ip,"temperature");

    if(!strcasecmp(type,"Temperature"))
        spd->senz_type=0;
    else if(!strcasecmp(type,"RPM"))
        spd->senz_type=1;
    else if(!strcasecmp(type,"Voltage"))
        spd->senz_type=2;

    spd->senz_index=(unsigned char)extension_size_t("SensorIndex",ip,0);

    if(spd->senz_index>=MAX_LEN)
        spd->senz_index=MAX_LEN-1;
}

static inline int speedfan_gather_data(speedfan_shared_memory *data)
{
    void *fm=OpenFileMappingA(FILE_MAP_READ,0,"Local\\SFSharedMemory_ALM");
    int ret=0;
    if(fm)
    {
        void *shd=MapViewOfFile(fm,FILE_MAP_READ,0,0,sizeof(speedfan_shared_memory));

        if(shd)
        {
            memcpy(data,shd,sizeof(speedfan_shared_memory));
            UnmapViewOfFile(shd);
            ret=0;
        }

        CloseHandle(fm);
    }

    return(ret);
}

double extension_update_func(void *spv)
{
    speedfan_data *spd=spv;
    static speedfan_shared_memory data= {0};
    speedfan_gather_data(&data);
    if(data.version!=1)
        return(0.0);
    switch(spd->senz_type)
    {
    case 0:
        return((double)data.temps[spd->senz_index>data.NumTemps?data.NumTemps:spd->senz_index]/100.0);
    case 1:
        return((double)data.fans[spd->senz_index>data.NumFans?data.NumFans:spd->senz_index]);
    case 2:
        return((double)data.volts[spd->senz_index>data.NumVolts?data.NumVolts:spd->senz_index]/100.0);
    }
    return(0.0);
}

void extension_destroy_func(void **spv)
{
    free(*spv);
    *spv=NULL;
}
