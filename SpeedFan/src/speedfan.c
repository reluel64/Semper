/* Ping source
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */
#ifdef WIN32
#include <stdlib.h>
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
    int temps[MAX_LEN];
    int fans[MAX_LEN];
    int volts[MAX_LEN];
} speedfan_shared_memory;

#pragma pack(pop)

typedef enum
{
    speedfan_unk,
    speedfan_temp,
    speedfan_rpm,
    speedfan_volt
}speedfan_senz_type;

typedef struct
{
    speedfan_senz_type sst;
    unsigned char senz_index;
} speedfan_data;


void init(void **spv,void *ip)
{
    speedfan_data *spd=malloc(sizeof(speedfan_data));
    memset(spd,0,sizeof(speedfan_data));
    *spv=spd;
}

void reset(void *spv,void *ip)
{
    speedfan_data *spd=spv;
    unsigned char *type=param_string("SensorType",EXTENSION_XPAND_SOURCES|EXTENSION_XPAND_VARIABLES,ip,"Temperature");

    spd->sst=speedfan_unk;

    if(!strcasecmp(type,"Temperature"))
        spd->sst=speedfan_temp;
    else if(!strcasecmp(type,"RPM"))
        spd->sst=speedfan_temp;
    else if(!strcasecmp(type,"Voltage"))
        spd->sst=speedfan_volt;

    spd->senz_index=(unsigned char)param_size_t("SensorIndex",ip,0);

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

double update(void *spv)
{
    static speedfan_shared_memory data= {0};
    speedfan_data *spd=spv;

    speedfan_gather_data(&data);
    if(data.version!=1)
        return(0.0);
    switch(spd->sst)
    {
        case speedfan_temp:
            return((double)data.temps[spd->senz_index>data.NumTemps?data.NumTemps:spd->senz_index]/100.0);
        case speedfan_rpm:
            return((double)data.fans[spd->senz_index>data.NumFans?data.NumFans:spd->senz_index]);
        case speedfan_volt:
            return((double)data.volts[spd->senz_index>data.NumVolts?data.NumVolts:spd->senz_index]/100.0);
        case speedfan_unk:
        	return(0.0);
    }
    return(0.0);
}

void destroy(void **spv)
{
    free(*spv);
    *spv=NULL;
}
#endif
