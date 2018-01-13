#define _WIN32_WINNT 0x0400
#define _WIN32_DCOM

#include <SDK/semper_api.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <wbemidl.h>

typedef struct
{
    unsigned char *name;
    unsigned char *type;
    IWbemLocator         *locator;

} open_hardware_monitor;


static double ohm_query(open_hardware_monitor *ohm);
static unsigned char *ucs_to_utf8(wchar_t *s_in, size_t *bn, unsigned char be);
void init(void **spv,void *ip)
{
    open_hardware_monitor *ohm=malloc(sizeof(open_hardware_monitor));
    memset(ohm,0,sizeof(open_hardware_monitor));
    CoInitializeEx(0, COINIT_MULTITHREADED);
    CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    CoCreateInstance(&CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (LPVOID *) &ohm->locator);
    *spv=ohm;
}

void reset(void *spv,void *ip)
{
    open_hardware_monitor *ohm=spv;
    free(ohm->name);
    free(ohm->type);
    ohm->name=NULL;
    ohm->type=NULL;
    unsigned char *temp=NULL;

    temp=param_string("Name",EXTENSION_XPAND_ALL,ip,NULL);

    if(temp)
        ohm->name=strdup(temp);

    temp=param_string("Type",EXTENSION_XPAND_ALL,ip,NULL);

    if(temp)
        ohm->type=strdup(temp);
}


double update(void *spv)
{
    open_hardware_monitor *ohm=spv;
    if(ohm->type==NULL||ohm->name==NULL)
        return(0.0);
    return(ohm_query(ohm));
}



void destroy(void **spv)
{
    open_hardware_monitor *ohm=*spv;
    ohm->locator->lpVtbl->Release(ohm->locator);
    CoUninitialize();
    free(ohm->name);
    free(ohm->type);
    free(*spv);
    *spv=NULL;

}


static double ohm_query(open_hardware_monitor *ohm)
{
    double value=0;
    IEnumWbemClassObject *results  = NULL;
    IWbemServices        *services=NULL;

    HRESULT hr = ohm->locator->lpVtbl->ConnectServer(ohm->locator,  L"ROOT\\OpenHardwareMonitor", NULL, NULL, NULL, 0, NULL, NULL, &services);
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
                VARIANT type= {0};

                hr = result->lpVtbl->Get(result, L"Name", 0, &name, 0, 0);

                if(hr==S_OK)
                {
                    hr = result->lpVtbl->Get(result, L"SensorType", 0, &type, 0, 0);

                    if(hr==S_OK)
                    {
                        char *s_name=ucs_to_utf8(name.bstrVal,NULL,0);
                        char *s_type=ucs_to_utf8(type.bstrVal,NULL,0);

                        if(s_name&&s_type&&!strcasecmp(s_name,ohm->name)&&!strcasecmp(s_type,ohm->type))
                        {
                            VARIANT val= {0};
                            hr = result->lpVtbl->Get(result, L"Value", 0, &val, 0, 0);
                            if(hr==S_OK)
                            {
                                value=val.fltVal;
                            }
                        }
                        free(s_name);
                        free(s_type);
                    }
                }
                result->lpVtbl->Release(result);
            }
            results->lpVtbl->Release(results);
        }
        services->lpVtbl->Release(services);
    }
    return(value);
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
