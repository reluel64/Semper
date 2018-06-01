#ifdef WIN32
#define _WIN32_WINNT 0x0400
#define _WIN32_DCOM
#include <SDK/semper_api.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <wbemidl.h>
#include <pthread.h>
#include <sys/time.h>
typedef struct
{
    unsigned char *name;
    unsigned char *type;
    double val;
} ohm_data;
typedef struct
{
    ohm_data *data;
    size_t dlen;
    void *kill;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_t qth;
    size_t inst_cnt;
} open_hardware_monitor;

typedef struct
{
    open_hardware_monitor *inst;
    unsigned char *name;
    unsigned char *type;
} open_hardware_monitor_inst;


static void * ohm_query(void *pv);
static unsigned char *ucs_to_utf8(wchar_t *s_in, size_t *bn, unsigned char be);

void init(void **spv,void *ip)
{

    static open_hardware_monitor ohm_inst = {0};

    if(ohm_inst.inst_cnt==0)
    {
        ohm_inst.kill=semper_safe_flag_init();
        pthread_mutexattr_t mutex_attr;
        pthread_mutexattr_init(&mutex_attr);
        pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_RECURSIVE_NP);
        pthread_mutex_init(&ohm_inst.mutex,&mutex_attr);
        pthread_mutexattr_destroy(&mutex_attr);
        pthread_cond_init(&ohm_inst.cond,NULL);
        pthread_create(&ohm_inst.qth, NULL, ohm_query, &ohm_inst);
    }

    ohm_inst.inst_cnt++;
    open_hardware_monitor_inst *ohm=malloc(sizeof(open_hardware_monitor));
    memset(ohm,0,sizeof(open_hardware_monitor));
    ohm->inst=&ohm_inst;

    *spv=ohm;
}

void reset(void *spv,void *ip)
{
    open_hardware_monitor_inst *ohm=spv;
    unsigned char *temp=NULL;

    free(ohm->name);
    free(ohm->type);
    ohm->name=NULL;
    ohm->type=NULL;

    temp=param_string("Name",EXTENSION_XPAND_ALL,ip,NULL);

    if(temp)
        ohm->name=strdup(temp);

    temp=param_string("Type",EXTENSION_XPAND_ALL,ip,NULL);

    if(temp)
        ohm->type=strdup(temp);
}

double update(void *spv)
{
    open_hardware_monitor_inst *ohm=spv;
    double v=0.0;
    if(ohm->type==NULL||ohm->name==NULL)
        return(0.0);

    pthread_mutex_lock(&ohm->inst->mutex);
    for(size_t i=0; i<ohm->inst->dlen; i++)
    {
        if(!strcasecmp(ohm->inst->data[i].name,ohm->name)&&!strcasecmp(ohm->inst->data[i].type,ohm->type))
        {
            v=ohm->inst->data[i].val;
            break;
        }
    }
    pthread_mutex_unlock(&ohm->inst->mutex);
    return(v);
}



void destroy(void **spv)
{
    open_hardware_monitor_inst *ohm=*spv;
    open_hardware_monitor *ohm_inst=ohm->inst;

    if(ohm_inst->inst_cnt>0)
        ohm_inst->inst_cnt--;

    if(ohm_inst->inst_cnt==0)
    {
        semper_safe_flag_set(ohm_inst->kill,1);
        pthread_cond_signal(&ohm_inst->cond);

        if(ohm_inst->qth)
            pthread_join(ohm_inst->qth,NULL);
        pthread_cond_destroy(&ohm_inst->cond);
        semper_safe_flag_destroy(&ohm_inst->kill);
        pthread_mutex_destroy(&ohm_inst->mutex);
    }

    free(ohm->name);
    free(ohm->type);
    free(*spv);
    *spv=NULL;

}

static void * ohm_query(void *pv)
{
    pthread_mutex_t mtx;
    open_hardware_monitor *ohm=pv;
    IEnumWbemClassObject *results  = NULL;
    IWbemServices        *services=NULL;
    IWbemLocator         *locator=NULL;
    CoInitializeEx(0, 0);
    CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    CoCreateInstance(&CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (LPVOID *) &locator);

    while(semper_safe_flag_get(ohm->kill)==0)
    {
        struct timeval tv;
        struct timespec ts;
        int ret = 0;
        size_t len = 0;
        ohm_data *od=NULL;
        double value=0.0;

        HRESULT hr = locator->lpVtbl->ConnectServer(locator,  L"ROOT\\OpenHardwareMonitor", NULL, NULL, NULL, 0, NULL, NULL, &services);
        if(hr==S_OK)
        {
            hr = services->lpVtbl->ExecQuery(services, L"WQL", L"SELECT * FROM Sensor", WBEM_FLAG_BIDIRECTIONAL, NULL, &results);

            if (results != NULL)
            {
                IWbemClassObject *result = NULL;
                ULONG returnedCount = 0;


                while((hr = results->lpVtbl->Next(results, WBEM_INFINITE, 1, &result, &returnedCount)) == S_OK)
                {
                    VARIANT name= {0};
                    hr = result->lpVtbl->Get(result, L"Name", 0, &name, 0, 0);

                    if(hr==S_OK)
                    {
                        VARIANT type= {0};
                        hr = result->lpVtbl->Get(result, L"SensorType", 0, &type, 0, 0);

                        if(hr==S_OK)
                        {
                            ohm_data *tod=NULL;
                            tod=realloc(od,sizeof(ohm_data)*(len+1));
                            VARIANT val= {0};
                            if(tod!=NULL)
                            {
                                char *s_name=ucs_to_utf8(name.bstrVal,NULL,0);
                                char *s_type=ucs_to_utf8(type.bstrVal,NULL,0);
                                tod[len].name=s_name;
                                tod[len].type=s_type;
                                hr = result->lpVtbl->Get(result, L"Value", 0, &val, 0, 0);
                                if(hr==S_OK)
                                {
                                    tod[len].val=(double)val.fltVal;
                                    VariantClear(&val);
                                }
                                od=tod;
                                len++;
                            }
                        }
                        VariantClear(&type);
                    }
                    VariantClear(&name);
                    result->lpVtbl->Release(result);
                }
            }
            results->lpVtbl->Release(results);
        }
        services->lpVtbl->Release(services);

        pthread_mutex_lock(&ohm->mutex);
        if(ohm->data)
        {
            for(size_t i=0; i<ohm->dlen; i++)
            {
                free(ohm->data[i].name);
                free(ohm->data[i].type);
            }
            free(ohm->data);
            ohm->data=NULL;
        }
        ohm->data=od;
        ohm->dlen=len;
        pthread_mutex_unlock(&ohm->mutex);
        gettimeofday(&tv, NULL);

        pthread_mutex_init(&mtx,NULL);
        ts.tv_sec = time(NULL) + 1000 / 1000;
        ts.tv_nsec = tv.tv_usec * 1000 + 1000 * 1000 * (1000 % 1000);
        ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
        ts.tv_nsec %= (1000 * 1000 * 1000);

        pthread_mutex_lock(&mtx);
        ret=pthread_cond_timedwait(&ohm->cond, &mtx, &ts);
        pthread_mutex_unlock(&mtx);
        pthread_mutex_destroy(&mtx);
    }

    locator->lpVtbl->Release(locator);
    CoUninitialize();
    for(size_t i=0; i<ohm->dlen; i++)
    {
        free(ohm->data[i].name);
        free(ohm->data[i].type);
    }
    free(ohm->data);

    return(NULL);
}



static unsigned char *ucs_to_utf8(wchar_t *s_in, size_t *bn, unsigned char be)
{
    /*Query the needed bytes to be allocated*/
    size_t nb = 0;
    unsigned char *out = NULL;
    for (size_t i = 0; s_in[i]; i++)
    {
        size_t ch = s_in[i];
        if (be)
        {
            unsigned char byte_hi = 0;
            unsigned char byte_lo = 0;
            byte_hi = (unsigned char)((ch & 0xFF00) >> 8);
            byte_lo = (unsigned char)((ch & 0xFF));
            ch = ((unsigned short)byte_lo) << 8 | (byte_hi);
        }
        if (ch < 0x80)
        {
            nb++;
        }
        else if (ch >= 0x80 &&ch < 0x800)
        {
            nb += 2;
        }
        else if (ch >= 0x800 && ch < 0xFFFF)
        {
            nb += 3;
        }
        else if (ch >= 0x10000 && ch < 0x10FFFF)
        {
            nb += 4;
        }
    }
    if (nb == 0)
        return(NULL);
    out = malloc(nb + 1);
    memset(out,0,nb + 1);
    /*Let's encode unicode to UTF-8*/
    size_t di = 0;
    for (size_t i = 0; s_in[i]; i++)
    {
        size_t ch = s_in[i];
        if (be)
        {
            unsigned char byte_hi = 0;
            unsigned char byte_lo = 0;
            byte_hi = (unsigned char)((ch & 0xFF00) >> 8);
            byte_lo = (unsigned char)((ch & 0xFF));
            ch = ((unsigned short)byte_lo) << 8 | (byte_hi);
        }
        if (ch < 0x80)
        {
            out[di++] = (unsigned char)s_in[i];
        }
        else if ((size_t)ch >= 0x80 && (size_t)ch < 0x800)
        {
            out[di++] = (s_in[i] >> 6) | 0xC0;
            out[di++] = (s_in[i] & 0x3F) | 0x80;
        }
        else if (ch >= 0x800 && ch < 0xFFFF)
        {
            out[di++] = (s_in[i] >> 12) | 0xE0;
            out[di++] = ((s_in[i] >> 6) & 0x3F) | 0x80;
            out[di++] = (s_in[i] & 0x3F) | 0x80;
        }
        else if (ch >= 0x10000 &&ch < 0x10FFFF)
        {
            out[di++] = ((unsigned long)s_in[i] >> 18) | 0xF0;
            out[di++] = ((s_in[i] >> 12) & 0x3F) | 0x80;
            out[di++] = ((s_in[i] >> 6) & 0x3F) | 0x80;
            out[di++] = (s_in[i] & 0x3F) | 0x80;
        }
    }
    if(bn)
    {
        *bn = nb;
    }
    return(out);
}
#endif
