/*
 * GPU-Z source
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 * */
#include <SDK/extension.h>
#include <windows.h>

#define string_length(s) ((s==NULL?0:strlen(s)))

/*
 * https://www.techpowerup.com/forums/threads/gpu-z-shared-memory-layout.65258/
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
/*Copied from Semper*/


static wchar_t *utf8_to_ucs(unsigned char *str)
{
    size_t len = string_length(str);

    if (len == 0)
    {
        return(NULL);
    }
    size_t di = 0;
    wchar_t *dest = malloc((len + 1) * 2);
    memset(dest,0,(len + 1) * 2);
    for (size_t si = 0; si < len; si++)
    {
        if (str[si] <= 0x7F)
        {
            dest[di] = str[si];
        }
        else if (str[si] >= 0xE0 && str[si] <= 0xEF)
        {
            dest[di] = (((str[si++]) & 0xF) << 12);
            dest[di] = dest[di] | (((unsigned short)(str[si++]) & 0x103F) << 6);
            dest[di] = dest[di] | (((unsigned short)(str[si]) & 0x103F));
        }
        else if (str[si] >= 0xc0 && str[si] <= 0xDF)
        {
            dest[di] = ((((unsigned short)str[si++]) & 0x1F) << 6);
            dest[di] = dest[di] | (((unsigned short)str[si]) & 0x103F);
        }
        di++;
    }
    return(dest);
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
    free(gd->opt);

    gd->str_val=NULL;
    gd->opt=NULL;

    unsigned char *s=param_string("GPUZInfo",EXTENSION_XPAND_SOURCES|EXTENSION_XPAND_VARIABLES,ip,"GPU Temperature");
    gd->opt=utf8_to_ucs(s);
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
    free(gd->str_val);
    gd->str_val=NULL;

    if(gd->found==1)
        return(NULL);

    for(size_t i=0; i<MAX_RECORDS; i++)
    {
        if(!_wcsicmp(gd->dp->data[i].key,gd->opt))
        {
            gd->str_val=ucs_to_utf8(gd->dp->data[i].value,NULL,0);
            return(gd->str_val);
        }
    }

    return(NULL);
}

void destroy(void **spv)
{
    gpuz_data *gd=*spv;
    free(gd->str_val);
    free(gd->opt);
    free(*spv);

    *spv=NULL;
}
