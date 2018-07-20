/*
 * GPU-Z source
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 * */
 #ifdef WIN32
#include <SDK/semper_api.h>
#include <windows.h>

#define string_length(s) ((s==NULL?0:strlen(s)))

/*
 * https://www.techpowerup.com/forums/threads/gpu-z-shared-memory-layout.65258
 * */
#define MAX_RECORDS 128
#pragma pack(push, 1)
typedef struct
{
    WCHAR key[256];
    WCHAR value[256];
} GPUZ_RECORD;

typedef struct
{
    WCHAR name[256];
    WCHAR unit[8];
    UINT32 digits;
    double value;
} GPUZ_SENSOR_RECORD;

typedef struct
{
    UINT32 version; 	 // Version number, 1 for the struct here
    volatile LONG busy;	 // Is data being accessed?
    UINT32 lastUpdate; // GetTickCount() of last update
    GPUZ_RECORD data[MAX_RECORDS];
    GPUZ_SENSOR_RECORD sensors[MAX_RECORDS];
} GPUZ_SH_MEM;
#pragma pack(pop)

typedef struct
{
    GPUZ_SH_MEM *dp;
    unsigned short *opt;
    unsigned char *str_val;
    unsigned char found;
} gpuz_data;


static inline int gpuz_gather_data(GPUZ_SH_MEM *data);
/*********************************************/

void init(void **spv,void *ip)
{
    *spv=malloc(sizeof(gpuz_data));

    memset(*spv,0,sizeof(gpuz_data));
}

void reset(void *spv,void *ip)
{
    gpuz_data *gd=spv;
    free(gd->str_val);
    semper_free(gd->opt);

    gd->str_val=NULL;
    gd->opt=NULL;

    unsigned char *s=param_string("GPUZInfo",EXTENSION_XPAND_SOURCES|EXTENSION_XPAND_VARIABLES,ip,"GPU Temperature");
    gd->opt=semper_utf8_to_ucs(s);
}

double update(void *spv)
{
    static GPUZ_SH_MEM data= {0};

    gpuz_data *gd=spv;
    gd->found=0;
    gd->dp=&data;

    if(gpuz_gather_data(&data))
    {
        gd->found=1;                    //nah, we did not find anything....not even some useful data
        return(-1.0);
    }

    for(size_t i=0; i<MAX_RECORDS; i++)
    {
        if(!_wcsicmp(gd->dp->sensors[i].name,gd->opt))
        {
            gd->found=1;
            return(gd->dp->sensors[i].value);
        }
    }
    return(0.0);
}

unsigned char *string(void *spv)
{
    gpuz_data *gd=spv;
    semper_free((void**)&gd->str_val);


    if(gd->found==1)
        return(NULL);

    for(size_t i=0; i<MAX_RECORDS; i++)
    {
        if(!_wcsicmp(gd->dp->data[i].key,gd->opt))
        {
            gd->str_val=semper_ucs_to_utf8(gd->dp->data[i].value,NULL,0);
            return(gd->str_val);
        }
    }

    return(NULL);
}

void destroy(void **spv)
{
    gpuz_data *gd=*spv;
    semper_free((void**)&gd->str_val);
    free(gd->opt);
    free(*spv);

    *spv=NULL;
}

static inline int gpuz_gather_data(GPUZ_SH_MEM *data)
{
    void *fm=OpenFileMappingA(FILE_MAP_READ,0,"Local\\GPUZShMem");
    int ret=-1;
    if(fm)
    {
        void *shd=MapViewOfFile(fm,FILE_MAP_READ,0,0,sizeof(GPUZ_SH_MEM));

        if(shd)
        {
            memcpy(data,shd,sizeof(GPUZ_SH_MEM));
            UnmapViewOfFile(shd);
            ret=0;
        }

        CloseHandle(fm);
    }
    return(ret);
}




#endif
