/*
 * CoreTemp source
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 * */
#ifdef WIN32
#include <windows.h>


/*
 * TODO: This source should be enhanced to
 * use only one shared memory instance
 * to gather information for the other sources
 */

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
#pragma pack (pop)

typedef enum
{
    coretemp_unk,
    coretemp_load,
    coretemp_tjmax,
    coretemp_core_count,
    coretemp_cpu_count,
    coretemp_temp,
    coretemp_cpu_spd,
    coretemp_fsb_speed,
    coretemp_vcore,
    coretemp_mul,
    coretemp_name,
    coretemp_max_temp,
    coretemp_core_speed,
    coretemp_tdp,
    coretemp_pwr
} coretemp_inf_t;

typedef struct
{
    size_t core_index;
    coretemp_inf_t cti;
    double max_temp;
    unsigned char* cpu_name;
} coretemp_data;

static inline int coretemp_gather_data(CoreTempSharedDataEx* data);
static double coretemp_calc_max_temp(CoreTempSharedDataEx* data);

void init(void** spv, void* ip)
{
    coretemp_data* cd = malloc(sizeof(coretemp_data));
    memset(cd, 0, sizeof(coretemp_data));
    *spv = cd;
}

static coretemp_inf_t coretemp_dispatch_opt(unsigned char *opt)
{
    static unsigned char* opts[] =
    {
        "Load", "TjMax", "CoreCount",
        "CpuCount", "Temperature", "CPUSpeed",
        "FSBSpeed", "Voltage", "Multiplier",
        "Name", "MaxTemp", "CoreSpeed",
        "TDP", "Power"
    };

    for(unsigned char i = 0; i < sizeof(opts) / sizeof(unsigned char*); i++)
    {
        if(strcasecmp(opt, opts[i]) == 0)
        {
            return((coretemp_inf_t)(i + 1));
        }
    }

    return(coretemp_unk);
}

void reset(void* spv, void* ip)
{
    coretemp_data* crd = spv;
    unsigned char* str_opt = NULL;
    crd->core_index = 0;
    str_opt = param_string("CoreTempInfo", EXTENSION_XPAND_SOURCES | EXTENSION_XPAND_VARIABLES, ip, "Temperature");
    crd->cti = coretemp_dispatch_opt(str_opt);
    crd->core_index = param_size_t("CoreTempIndex", ip, 0);
    crd->core_index = (crd->core_index > 127 ? 127 : crd->core_index);
}

double update(void* spv)
{
    coretemp_data* crd = spv;
    static CoreTempSharedDataEx data = { 0 };

    if(coretemp_gather_data(&data))
    {
        crd->cpu_name = "ERROR";
        return (0.0);
    }

    crd->cpu_name = data.sCPUName;

    switch(crd->cti)
    {
        case coretemp_load:
            return (data.uiLoad[crd->core_index]); // core load

        case coretemp_tjmax:
            return (data.uiTjMax[crd->core_index]); // core tjmax

        case coretemp_core_count:
            return (data.uiCoreCnt); // CPU core count

        case coretemp_cpu_count:
            return (data.uiCPUCnt); // cpu count

        case coretemp_temp:
            return (data.ucDeltaToTjMax ?
                    (float)data.uiTjMax[crd->core_index] - data.fTemp[crd->core_index] :
                    data.fTemp[crd->core_index]); // core temperature

        case coretemp_cpu_spd:
            return (data.fCPUSpeed); // global cpu speed

        case coretemp_fsb_speed:
            return (data.fFSBSpeed); // bus speed

        case coretemp_vcore:
            return (data.fVID); // voltage requested by cpu

        case coretemp_mul:
            return ((double)data.fMultipliers[crd->core_index]); // per core multiplier

        case coretemp_name:
            return (0.0);

        case coretemp_max_temp:
            return (coretemp_calc_max_temp(&data)); // highest temperature

        case coretemp_core_speed:
            return ((double)data.fMultipliers[crd->core_index] * (double)data.fFSBSpeed); // per core frequency

        case coretemp_tdp:
            return (data.ucTdpSupported ? (double)data.uiTdp[crd->core_index] : 0.0); // TDP

        case coretemp_pwr:
            return (data.ucPowerSupported ? (double)data.fPower[crd->core_index] : 0.0); // Power (Wattage)

        default:
            diag_error("%s %d CoreTemp unknown option 0x%x", __FUNCTION__, __LINE__, crd->cti);
            break;
    }

    return (0.0);
}

unsigned char* string(void* spv)
{
    coretemp_data* crd = spv;

    if(crd->cti == coretemp_name)
        return (crd->cpu_name ? crd->cpu_name : (unsigned char*)"");

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
    int ret = -1;

    if(fm)
    {
        void* shd = MapViewOfFile(fm, FILE_MAP_READ, 0, 0, sizeof(CoreTempSharedDataEx));

        if(shd)
        {
            memcpy(data, shd, sizeof(CoreTempSharedDataEx));
            UnmapViewOfFile(shd);
            ret = 0;
        }

        else
        {
            diag_error("%s %d Failed to read CoreTemp data", __FUNCTION__, __LINE__);
        }

        CloseHandle(fm);
    }
    else
    {
        diag_error("%s %d Failed to access CoreTemp data", __FUNCTION__, __LINE__);
    }

    return (ret);
}

static double coretemp_calc_max_temp(CoreTempSharedDataEx* data)
{
    double max_temp = 0.0;

    for(size_t i = 0; i < 128; i++)
    {
        if(max_temp < data->fTemp[i])
            max_temp = data->fTemp[i];
    }

    return (max_temp);
}
#endif
