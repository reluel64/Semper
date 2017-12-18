#include <semper_api.h>
#include <mem.h>
#include <string_util.h>

typedef struct _string_source_info
{
    unsigned char* str;
    unsigned char option;
    size_t len;
} string_source_info;

void string_source_create(void** pv, void* ip)
{
    unused_parameter(ip);
    * pv = zmalloc(sizeof(string_source_info));
}

void string_source_reset(void* pv, void* ip)
{
    string_source_info* ssi = pv;
    sfree((void**)&ssi->str);
    ssi->len = 0;
    ssi->option = 0;
    unsigned char* str = param_string("String", EXTENSION_XPAND_SOURCES | EXTENSION_XPAND_VARIABLES, ip, NULL);

    if(str)
    {
        ssi->str = clone_string(str);
        ssi->len = utf8_len(str,0);
    }

    unsigned char* opt = param_string("Mode", EXTENSION_XPAND_SOURCES | EXTENSION_XPAND_VARIABLES, ip, "String");

    if(opt)
    {
        if(strcasecmp("Length", opt) == 0)
        {
            ssi->option = 0;
        }
        else if(strcasecmp("Number",opt)==0)
        {
            ssi->option=1;
        }
        else if(strcasecmp("String",opt)==0)
        {
            ssi->option=3;
        }
    }
}

double string_source_update(void* pv)
{
    string_source_info* ssi = pv;

    switch(ssi->option)
    {
        case 0:
            return ((double)ssi->len);

        case 1:
            return(strtod(ssi->str,NULL));

        default:
            return(-1.0);
    }
}

unsigned char* string_source_string(void* pv)
{
    string_source_info* ssi = pv;
    return (ssi->str);
}

void string_source_destroy(void** pv)
{
    string_source_info* ssi = *pv;
    sfree((void**)&ssi->str);
    sfree(*pv);
}
