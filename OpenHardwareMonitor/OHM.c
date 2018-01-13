#define _WIN32_WINNT 0x0400
#define _WIN32_DCOM

#include <SDK/semper_api.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <wbemidl.h>
#include <pthread.h>
typedef struct
{
    unsigned char *name;
    unsigned char *type;
    unsigned char work;
    pthread_mutex_t mutex;
    double value;
    pthread_t qth;

} open_hardware_monitor;

static void * ohm_query(void *pv);

static unsigned char *ucs_to_utf8(wchar_t *s_in, size_t *bn, unsigned char be);

void init(void **spv,void *ip)
{
    open_hardware_monitor *ohm=malloc(sizeof(open_hardware_monitor));
    memset(ohm,0,sizeof(open_hardware_monitor));

    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&ohm->mutex,&mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);
    *spv=ohm;
}

void reset(void *spv,void *ip)
{
    open_hardware_monitor *ohm=spv;
    unsigned char *temp=NULL;

    pthread_mutex_lock(&ohm->mutex);
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

    pthread_mutex_unlock(&ohm->mutex);
}

double update(void *spv)
{
    open_hardware_monitor *ohm=spv;
    int status=0;
    double v=0;
    if(ohm->type==NULL||ohm->name==NULL)
        return(0.0);

    if(ohm->qth==0)
    {
        ohm->work=1;
        if(pthread_create(&ohm->qth, NULL, ohm_query, ohm)==0)
        {
            while(ohm->work==1)
            {
                sched_yield();
            }
        }
        else
        {
            ohm->work=0;
            diag_error("Failed to start WMI query thread");
        }
    }

    if(ohm->work==0&&ohm->qth)
    {
        pthread_join(ohm->qth,NULL);
        memset(&ohm->qth,0,sizeof(pthread_t));
    }

    pthread_mutex_lock(&ohm->mutex);
    v=ohm->value;
    pthread_mutex_unlock(&ohm->mutex);
    return(v);
}



void destroy(void **spv)
{
    open_hardware_monitor *ohm=*spv;
    if(ohm->qth)
        pthread_join(ohm->qth,NULL);
    pthread_mutex_destroy(&ohm->mutex);

    free(ohm->name);
    free(ohm->type);
    free(*spv);
    *spv=NULL;

}


static void * ohm_query(void *pv)
{
    open_hardware_monitor *ohm=pv;
    IEnumWbemClassObject *results  = NULL;
    IWbemServices        *services=NULL;
    IWbemLocator         *locator=NULL;
    ohm->work=2;


    CoInitializeEx(0, 0);
    CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    CoCreateInstance(&CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (LPVOID *) &locator);
    HRESULT hr = locator->lpVtbl->ConnectServer(locator,  L"ROOT\\OpenHardwareMonitor", NULL, NULL, NULL, 0, NULL, NULL, &services);
    if(hr==S_OK)
    {
        hr = services->lpVtbl->ExecQuery(services, L"WQL", L"SELECT * FROM Sensor", WBEM_FLAG_BIDIRECTIONAL, NULL, &results);

        if (results != NULL)
        {
            IWbemClassObject *result = NULL;
            ULONG returnedCount = 0;
            unsigned char found=0;

            while(found==0&&(hr = results->lpVtbl->Next(results, WBEM_INFINITE, 1, &result, &returnedCount)) == S_OK)
            {
                VARIANT name= {0};
                hr = result->lpVtbl->Get(result, L"Name", 0, &name, 0, 0);

                if(hr==S_OK)
                {
                    VARIANT type= {0};
                    hr = result->lpVtbl->Get(result, L"SensorType", 0, &type, 0, 0);

                    if(hr==S_OK)
                    {

                        char *s_name=ucs_to_utf8(name.bstrVal,NULL,0);
                        char *s_type=ucs_to_utf8(type.bstrVal,NULL,0);

                        pthread_mutex_lock(&ohm->mutex);

                        if(s_name&&s_type&&!strcasecmp(s_name,ohm->name)&&!strcasecmp(s_type,ohm->type))
                        {
                            pthread_mutex_unlock(&ohm->mutex);
                            VARIANT val= {0};
                            hr = result->lpVtbl->Get(result, L"Value", 0, &val, 0, 0);
                            if(hr==S_OK)
                            {
                                ohm->value=(double)val.fltVal;
                                VariantClear(&val);
                                found=1;
                            }
                        }
                        else
                        {
                            pthread_mutex_unlock(&ohm->mutex);
                        }

                        free(s_name);
                        free(s_type);
                        VariantClear(&type);
                    }
                    VariantClear(&name);
                }

                result->lpVtbl->Release(result);
            }
            results->lpVtbl->Release(results);

            if(found==0)
            {
                pthread_mutex_lock(&ohm->mutex);
                ohm->value=0.0;
                pthread_mutex_unlock(&ohm->mutex);
            }
        }
        services->lpVtbl->Release(services);
    }
    locator->lpVtbl->Release(locator);
    CoUninitialize();
    ohm->work=0;

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
