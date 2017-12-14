/*
 * CoreTemp source
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 * */

#include <SDK/extension.h>
#include <windows.h>


/*copied from CoreTemp SDK*/
#pragma pack(push)
#pragma pack(4)
typedef struct core_temp_shared_data_ex
{
    // Original structure (CoreTempSharedData)
    unsigned int uiLoad[256];
    unsigned int uiTjMax[128];
    unsigned int uiCoreCnt;
    unsigned int uiCPUCnt;
    float fTemp[256];
    float fVID;
    float fCPUSpeed;
    float fFSBSpeed;
    float fMultiplier;
    char sCPUName[100];
    unsigned char ucFahrenheit;
    unsigned char ucDeltaToTjMax;
    // uiStructVersion = 2
    unsigned char ucTdpSupported;
    unsigned char ucPowerSupported;
    unsigned int uiStructVersion;
    unsigned int uiTdp[128];
    float fPower[128];
    float fMultipliers[256];
} CoreTempSharedDataEx, *LPCoreTempSharedDataEx, **PPCoreTempSharedDataEx;
#pragma (pop)
typedef struct
{
    size_t core_index;
    unsigned char opt;
    double max_temp;
    unsigned char* cpu_name;
} coretemp_data;

static inline int coretemp_gather_data(CoreTempSharedDataEx* data);
static double coretemp_max_temp(CoreTempSharedDataEx* data);

void init(void** spv, void* ip)
{
    coretemp_data* cd = malloc(sizeof(coretemp_data));
    memset(cd, 0, sizeof(coretemp_data));
    *spv = cd;
}

void reset(void* spv, void* ip)
{
    coretemp_data* crd = spv;
    unsigned char opt = 0;
    crd->core_index = 0;
    unsigned char* str_opt = extension_string("CoreTempInfo", EXTENSION_XPAND_SOURCES | EXTENSION_XPAND_VARIABLES, ip, "Temperature");

    static unsigned char* opts[] =
    {
        "Load",    "TjMax",      "CoreCount", "CpuCount", "Temperature", "CPUSpeed", "FSBSpeed",
        "Voltage", "Multiplier", "Name",      "MaxTemp",  "CoreSpeed",   "TDP",      "Power"
    };

    for(opt = 0; opt < sizeof(opts) / sizeof(unsigned char*); opt++)
    {
        if(!strcasecmp(str_opt, opts[opt]))
            break;
    }

    if(opt < sizeof(opts) / sizeof(unsigned char*))
    {
        crd->opt = opt;
    }

    crd->core_index = extension_size_t("CoreTempIndex", ip, 0);
    crd->core_index = (crd->core_index > 127 ? 127 : crd->core_index);
}

double update(void* spv)
{
    coretemp_data* crd = spv;
    static CoreTempSharedDataEx data = { 0 };

    if(coretemp_gather_data(&data))
    {
        crd->cpu_name = "ERROR";
        return (-1.0);
    }

    crd->cpu_name = data.sCPUName;

    switch(crd->opt)
    {
    case 0:
        return (data.uiLoad[crd->core_index]); // core load
    case 1:
        return (data.uiTjMax[crd->core_index]); // core tjmax
    case 2:
        return (data.uiCoreCnt); // CPU core count
    case 3:
        return (data.uiCPUCnt); // cpu count
    case 4:
        return (data.fTemp[crd->core_index]); // core temperature
    case 5:
        return (data.fCPUSpeed); // global cpu speed
    case 6:
        return (data.fFSBSpeed); // bus speed
    case 7:
        return (data.fVID); // voltage requested by cpu
    case 8:
        return ((double)data.fMultipliers[crd->core_index]); // per core multiplier
    case 9:
        return (0.0);
    case 10:
        return (coretemp_max_temp(&data)); // highest temperature
    case 11:
        return ((double)data.fMultipliers[crd->core_index] * (double)data.fFSBSpeed); // per core frequency
    case 12:
        return (data.ucTdpSupported?(double)data.uiTdp[crd->core_index]:0.0); // TDP
    case 13:
        return (data.ucPowerSupported?(double)data.fPower[crd->core_index]:0.0); // Power (Wattage)
    }

    return (0.0);
}

unsigned char* string(void* spv)
{
    coretemp_data* crd = spv;
    if(crd->opt==0)
    {
        return (crd->cpu_name?crd->cpu_name:(unsigned char*)"");
    }
    return(NULL);
}

void destroy(void** spv)
{
    free(*spv);
    *spv = NULL;
}

static inline int coretemp_gather_data(CoreTempSharedDataEx* data)
{
    void* fm = OpenFileMappingA(FILE_MAP_READ, 0, "Local\\CoreTempMappingObjectEx");
    int ret=-1;

    if(fm)
    {
        void* shd = MapViewOfFile(fm, FILE_MAP_READ, 0, 0, sizeof(CoreTempSharedDataEx));

        if(shd)
        {
            memcpy(data, shd, sizeof(CoreTempSharedDataEx));
            UnmapViewOfFile(shd);
            ret=0;
        }
        CloseHandle(fm);
    }
    return (ret);
}

static double coretemp_max_temp(CoreTempSharedDataEx* data)
{
    double max_temp = 0.0;

    for(size_t i = 0; i < 128; i++)
    {
        if(max_temp < data->fTemp[i])
            max_temp = data->fTemp[i];
    }
    return (max_temp);
}
