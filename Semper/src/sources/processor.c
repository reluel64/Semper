/*
 * Processor source
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */
#include <sources/processor.h>
#include <mem.h>
#include <sources/extension.h>
#include <string_util.h>
#ifdef WIN32
#include <windows.h>
#include <ntsecapi.h>

typedef struct _PROCESSOR_POWER_INFORMATION
{
    ULONG Number;
    ULONG MaxMhz;
    ULONG CurrentMhz;
    ULONG MhzLimit;
    ULONG MaxIdleState;
    ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION
{
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER Reserved1[2];
    ULONG Reserved2;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

typedef struct _SYSTEM_PROCESS_INFORMATION
{
    ULONG NextEntryOffset;
    BYTE Reserved1[52];
    PVOID Reserved2[3];
    HANDLE UniqueProcessId;
    PVOID Reserved3;
    ULONG HandleCount;
    BYTE Reserved4[4];
    PVOID Reserved5[11];
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    LARGE_INTEGER Reserved6[6];
} SYSTEM_PROCESS_INFORMATION;

#elif __linux__
#include <dirent.h>
#endif
typedef struct _processor
{
    unsigned char total;
    unsigned char inf_type;
    unsigned char *process_name;
    size_t core_count;
    size_t usage_cpu_no;
    size_t idle_time_old;
    size_t system_time_old;
#ifdef __linux__
    size_t freq_cpu_no;
#endif
#ifdef WIN32

    HMODULE ntdll;
    HMODULE PowrProf;
    FARPROC CallNtPowerInformation;
    FARPROC NtQuerySystemInformation;
    FARPROC NtQueryInformationProcess;
    /*Freq*/
    PROCESSOR_POWER_INFORMATION* ppi;
    size_t ppi_sz;

#endif

} processor;

static double processor_frequency(processor* p, unsigned char max);

#ifdef __linux__

static size_t processor_core_count(void)
{
    unsigned char buf[256]= {0};
    FILE *f=fopen("/proc/cpuinfo","r");
    size_t c_count=0;
    if(f!=NULL)
    {
        while(fgets(buf,255,f))
        {
            if(!strncasecmp(buf,"processor",9))
            {
                c_count++;
            }
        }
        fclose(f);
    }
    return(c_count);
}


static double processor_frequency_linux(processor *p,unsigned char max)
{
    unsigned char buf[256]= {0};
    snprintf(buf,256,"/sys/devices/system/cpu/cpu%lu/cpufreq/%s",p->freq_cpu_no,max?"cpuinfo_max_freq":"cpuinfo_cur_freq");
    FILE *f=fopen(buf,"r");
    double val=0.0;
    if(f!=NULL)
    {
        fscanf(f,"%lf",&val);
        fclose(f);
    }
    return(val);
}
/*
static size_t processor_process_count_linux(void)
{
    size_t pcount=0;
    DIR *dh=opendir("/proc");
    struct dirent *ent=NULL;
    if(dh)
    {
        while((ent=readdir(dh))!=NULL)
        {
            unsigned char is_pid=1; //assume that this is a PID
            for(size_t i=0; i<256&&ent->d_name[i]; i++)
            {
                if(ent->d_name[i]<'0'||ent->d_name[i]>'9')
                {
                    is_pid=0;
                    break;
                }
            }
            if(is_pid)
                pcount++;
        }
        closedir(dh);
    }
    return(pcount);
}
*/
#endif


void processor_init(void** spv, void* ip)
{
    unused_parameter(ip);
    processor* p = zmalloc(sizeof(processor));
#ifdef WIN32
    p->ntdll = LoadLibraryW(L"ntdll.dll");
    p->NtQuerySystemInformation = GetProcAddress(p->ntdll, "NtQuerySystemInformation");
    p->NtQueryInformationProcess = GetProcAddress(p->ntdll, "NtQueryInformationProcess");
    p->PowrProf = LoadLibraryW(L"PowrProf");
    p->CallNtPowerInformation = GetProcAddress(p->PowrProf, "CallNtPowerInformation");
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    p->core_count = si.dwNumberOfProcessors;
#elif __linux__
    p->core_count=processor_core_count();
#endif

    *spv = p;
}

void processor_reset(void* spv, void* ip)
{
    processor* p = spv;
    sfree((void**)&p->process_name);
    unsigned char* inf_type = extension_string("Processor", EXTENSION_XPAND_SOURCES | EXTENSION_XPAND_VARIABLES, ip, "Usage");
    p->total = extension_bool("Total", ip, 0);
    if(inf_type)
    {
        if(!strcasecmp(inf_type, "Usage"))
        {
            p->inf_type = 0;
            extension_set_max(100.0, ip, 1, 1);
#ifdef WIN32
            p->usage_cpu_no = extension_size_t("CoreUsage", ip, 0);
#endif
        }
        else if(!strcasecmp(inf_type, "ProcessCount"))
        {
            p->inf_type = 1;
        }
        else if(!strcasecmp(inf_type, "CoreCount"))
        {
            p->inf_type = 2;
        }
        else if(!strcasecmp(inf_type, "Frequency"))
        {
            p->inf_type = 3;
#ifdef WIN32

            sfree((void**)&p->ppi);
            p->ppi = zmalloc(sizeof(PROCESSOR_POWER_INFORMATION) * p->core_count);
            p->ppi_sz = sizeof(PROCESSOR_POWER_INFORMATION) * p->core_count;
            p->CallNtPowerInformation(11, NULL, 0, p->ppi, p->ppi_sz);
            extension_set_max(processor_frequency(p, 1), ip, 1, 0);
#elif __linux__
            p->freq_cpu_no=0;
            double max=0.0;
            extension_set_max(processor_frequency_linux(p,1),ip,1,1);
            p->freq_cpu_no=extension_size_t("CoreIndexFrequency", ip, 0);

#endif
        }
        else if(!strcasecmp(inf_type,"ProcessRunning"))
        {
            p->inf_type=4;
            p->process_name=clone_string(extension_string("ProcessName", EXTENSION_XPAND_ALL, ip, NULL));
        }
    }
}


#ifdef WIN32
static inline double processor_usage_calculate_win32(void* spv, size_t idle, size_t system)
{
    processor* p = spv;

    size_t system_delta = system - p->system_time_old;
    size_t idle_delta = idle - p->idle_time_old;

    double usage = 100.0 - ((double)idle_delta / (double)system_delta) * 100.0;


    p->idle_time_old = idle;
    p->system_time_old = system;

    return (CLAMP(usage,0.0, 100.0));
}
#endif
static double processor_usage(processor* p)
{
    double ret = 0.0;

#ifdef WIN32
    size_t attempt = 64;
    size_t buf_sz = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * p->core_count;
    size_t out_buf = 0;
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION* sppi = zmalloc(buf_sz);
    while(--attempt)
    {
        if(p->NtQuerySystemInformation(8, sppi, buf_sz, &out_buf))
        {
            sfree((void**)&sppi);
            buf_sz *= 2;
            sppi = zmalloc(buf_sz);
        }
        else
            break;
    }
    if(attempt == 0)
    {
        sfree((void**)&sppi);
        return (0.0);
    }
    if(p->usage_cpu_no)
    {
        ret = processor_usage_calculate_win32(p, sppi[p->usage_cpu_no - 1].IdleTime.QuadPart,
                                              sppi[p->usage_cpu_no - 1].KernelTime.QuadPart +
                                              sppi[p->usage_cpu_no - 1].UserTime.QuadPart);
    }
    else
    {
        size_t idle=0;
        size_t kernel=0;
        size_t user=0;
        for(size_t i=0; i<p->core_count; i++)
        {
            idle+= sppi[i].IdleTime.QuadPart;
            kernel+= sppi[i].KernelTime.QuadPart ;
            user+=sppi[i].UserTime.QuadPart;
        }
        ret = processor_usage_calculate_win32(p, idle,kernel+user);
    }
    sfree((void**)&sppi);
#elif __linux__
    size_t used_cpu=0;
    size_t idle_cpu=0;
    size_t usage_cpu_no=p->usage_cpu_no;
    unsigned char row[256]= {0};
    unsigned char cpu[32]= {0};
    size_t cpul=0;

    cpul=snprintf(cpu,32,usage_cpu_no?"cpu%lu":"cpu",usage_cpu_no-1);
    FILE *f=fopen("/proc/stat","r");
    if(f==NULL)
    {
        return(0.0);
    }
    while(fgets(row,255,f))
    {
        if(strncasecmp(cpu,row,cpul)==0)
        {
            size_t user=0;
            size_t nice=0;
            size_t sys=0;
            size_t idle=0;
            size_t iowait=0;
            size_t irq=0;
            size_t softirq=0;
            size_t steal=0;
            sscanf(row+cpul+1,"%lu %lu %lu %lu %lu %lu %lu %lu",&user,&nice,&sys,&idle,&iowait,&irq,&softirq,&steal);
            idle_cpu=iowait+idle;
            used_cpu=(user+nice+sys+idle_cpu+irq+softirq+steal);
            break;
        }
    }

    size_t diff_idle=idle_cpu-p->idle_time_old;
    size_t diff_total=used_cpu-p->system_time_old;

    p->idle_time_old=idle_cpu;
    p->system_time_old=used_cpu;

    ret=(1000.0*(diff_total-(double)diff_idle)/(double)diff_total)/10.0;


#endif
    return(ret);
}



static size_t processor_process_count(processor *p)
{

    size_t processes = 0;
#ifdef WIN32
    processes=1; //count the idle process (Windows only)
    size_t buf_sz = sizeof(SYSTEM_PROCESS_INFORMATION);
    size_t buf_sz_out = 0;
    SYSTEM_PROCESS_INFORMATION* spi = zmalloc(buf_sz);
    size_t attempt = 128; /*we are pretty generous with 128 attempts*/

    while(p->NtQuerySystemInformation(5, spi, buf_sz, &buf_sz_out) && --attempt)
    {
        sfree((void**)&spi);
        buf_sz = buf_sz_out;
        spi = zmalloc(buf_sz);
    }

    if(attempt == 0)
    {
        sfree((void**)&spi);
        return (0);
    }

    for(SYSTEM_PROCESS_INFORMATION* sppi = spi; sppi->NextEntryOffset;)
    {
        if(p->inf_type==4)
        {
            processes=0;
            unsigned char found=0;
            UNICODE_STRING *us=(UNICODE_STRING*)(((unsigned char*)sppi)+0x38);
            unsigned char *proc=ucs_to_utf8(us->Buffer,NULL,0);
            if(proc&&p->process_name&&!strcasecmp(proc,p->process_name))
            {
                found=1;
            }
            sfree((void**)&proc);
            if(found==1)
            {
                processes=1;
                break;
            }
        }
        else
        {
            processes++;
        }

        sppi = (SYSTEM_PROCESS_INFORMATION*)((unsigned char*)sppi + sppi->NextEntryOffset);
    }
    sfree((void**)&spi);

#elif __linux__
    DIR *dh=opendir("/proc");
    struct dirent *ent=NULL;
    if(dh)
    {
        while((ent=readdir(dh))!=NULL)
        {
            unsigned char is_pid=1; //assume that this is a PID
            for(size_t i=0; i<256&&ent->d_name[i]; i++)
            {
                if(ent->d_name[i]<'0'||ent->d_name[i]>'9')
                {
                    is_pid=0;
                    break;
                }
            }
            if(is_pid)
                processes++;
        }
        closedir(dh);
    }
#endif

    return (processes);
}




static double processor_frequency(processor* p, unsigned char max)
{
#ifdef WIN32
    double val = 0.0;

    p->CallNtPowerInformation(11, NULL, 0, p->ppi, p->ppi_sz);

    for(size_t i = 0; i < p->core_count; i++)
    {
        if(max)
        {
            if(val < (double)p->ppi[0].MaxMhz)
            {
                val = (double)p->ppi[0].MaxMhz;
            }
        }
        else
        {
            if(val < (double)p->ppi[i].CurrentMhz)
            {
                val = (double)p->ppi[i].CurrentMhz;
            }
        }
    }
    return (val);
#elif __linux__
    unsigned char buf[256]= {0};
    snprintf(buf,256,"/sys/devices/system/cpu/cpu%lu/cpufreq/%s",p->freq_cpu_no,max?"scaling_max_freq":"scaling_cur_freq");
    FILE *f=fopen(buf,"r");
    double val=0.0;
    if(f!=NULL)
    {
        fscanf(f,"%lf",&val);
        fclose(f);
    }
    return(val/1000);
#endif
}

double processor_update(void* spv)
{
    processor* p = spv;

    switch(p->inf_type)
    {
    case 1:
        return ((double)processor_process_count(spv));
    case 2:
        return ((double)p->core_count);
    case 3:
        return (processor_frequency(p, p->total));
    case 4:
        return((double)processor_process_count(spv));
    default:
        return (processor_usage(spv));

    }
    return (0.0);
}

void processor_destroy(void** spv)
{
    processor* p = *spv;
    sfree((void**)&p->process_name);
#ifdef WIN32
    sfree((void**)&p->ppi);
    FreeLibrary(p->ntdll);
#endif
    sfree(spv);
}
