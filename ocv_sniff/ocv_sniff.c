#include <SDK/semper_api.h>
#include <ocv_sniff.h>
#include <windows.h>
typedef struct
{
    float cx[2];
    float cy[2];
    float ca[2];
    float left[2];
    float right[2];
    char good;
} OCV_SHARED;

typedef enum
{
    sniff_unk,
    sniff_x,
    sniff_xf,
    sniff_y,
    sniff_yf,
    sniff_a,
    sniff_af,
    sniff_l,
    sniff_lf,
    sniff_r,
    sniff_rf
} ocv_sniff_type;

typedef struct
{
    OCV_SHARED oc_sh;
    ocv_sniff_type tp;
} ocv_sniff;

static inline int ocv_sniff_gather_data(OCV_SHARED *data);

void init(void **spv,void *ip)
{
    *spv=malloc(sizeof(ocv_sniff));
    memset(*spv,0,sizeof(ocv_sniff));
}

void reset(void *spv,void *ip)
{
    ocv_sniff *os=spv;
    unsigned char *pm=param_string("SniffType",EXTENSION_XPAND_ALL,ip,NULL);

    if(pm)
    {
        if(!strcasecmp(pm,"X"))
            os->tp=sniff_x;

        else if(!strcasecmp(pm,"XF"))
            os->tp=sniff_xf;

        else if(!strcasecmp(pm,"Y"))
            os->tp=sniff_y;

        else if(!strcasecmp(pm,"YF"))
            os->tp=sniff_yf;

        else if(!strcasecmp(pm,"A"))
            os->tp=sniff_a;

        else if(!strcasecmp(pm,"AF"))
            os->tp=sniff_af;

        else if(!strcasecmp(pm,"L"))
            os->tp=sniff_l;

        else if(!strcasecmp(pm,"LF"))
            os->tp=sniff_lf;

        else if(!strcasecmp(pm,"R"))
            os->tp=sniff_r;

        else if(!strcasecmp(pm,"RF"))
            os->tp=sniff_rf;

        else
            os->tp=sniff_unk;
    }
}


double update(void *spv)
{
    ocv_sniff *os=spv;
    ocv_sniff_gather_data(&os->oc_sh);

    switch(os->tp)
    {
    case sniff_x:
        return(os->oc_sh.cx[0]);

    case sniff_xf:
        return(os->oc_sh.cx[1]);

    case sniff_y:
        return(os->oc_sh.cy[0]);

    case sniff_yf:
        return(os->oc_sh.cy[1]);

    case sniff_a:
        return(os->oc_sh.ca[0]);

    case sniff_af:
        return(os->oc_sh.ca[1]);

    case sniff_l:
        return(os->oc_sh.left[0]);

    case sniff_lf:
        return(os->oc_sh.left[1]);

    case sniff_r:
        return(os->oc_sh.right[0]);

    case sniff_rf:
        return(os->oc_sh.right[1]);

    }

    return(0.0);
}


void destroy(void **spv)
{
    free(*spv);
}


static inline int ocv_sniff_gather_data(OCV_SHARED *data)
{
    void *fm=OpenFileMappingA(FILE_MAP_READ,0,"OCV_SHARED_DATA");
    int ret=0;
    if(fm)
    {
        void *shd=MapViewOfFile(fm,FILE_MAP_READ,0,0,sizeof(OCV_SHARED));

        if(shd)
        {
            OCV_SHARED *pos=shd;
            if(pos->good)
            {
                memcpy(data,shd,sizeof(OCV_SHARED));
                pos->good=0;
            }

            UnmapViewOfFile(shd);
            ret=0;
        }

        CloseHandle(fm);
    }

    return(ret);
}
