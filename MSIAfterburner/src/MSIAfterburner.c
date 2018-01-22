#ifdef WIN32
#include <windows.h>
#include <MAHMSharedMemory.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDK/semper_api.h>

typedef struct
{
    unsigned char *res_name;
    size_t res_id;
} MSI_AFTERBURNER;

static inline int msi_afterburner_gather_data (MAHM_SHARED_MEMORY_HEADER* hdr,MAHM_SHARED_MEMORY_ENTRY **ent);

void init(void **spv,void *ip)
{
    *spv=malloc(sizeof(MSI_AFTERBURNER));
    memset(*spv,0,sizeof(MSI_AFTERBURNER));
}

void reset(void *spv,void *ip)
{
    MSI_AFTERBURNER *mab=spv;
    free(mab->res_name);
    mab->res_name=NULL;
    mab->res_name=strdup(param_string("ResourceName",EXTENSION_XPAND_ALL,ip,NULL));
    mab->res_id=param_size_t("resourceID",ip,0xFFFFFFFF);
}

double update(void *spv)
{
    MSI_AFTERBURNER *mab=spv;
    MAHM_SHARED_MEMORY_HEADER hdr= {0};
    MAHM_SHARED_MEMORY_ENTRY *entries=NULL;
    double val=0.0;
    if((mab->res_name==NULL&&mab->res_id==0xFFFFFFFF)||msi_afterburner_gather_data(&hdr,&entries))
    {
        return(0.0);
    }

    for(size_t i=0; entries&&i<hdr.dwNumEntries; i++)
    {
        if((mab->res_name!=NULL&&strcasecmp(mab->res_name,entries[i].szSrcName)==0)||mab->res_id==entries[i].dwSrcId)
        {
            val=entries[i].data;
            break;
        }
    }

    free(entries);

    return(val);
}

void destroy(void **spv)
{
    MSI_AFTERBURNER *mab=*spv;
    free(mab->res_name);
    free(*spv);
}

static inline int msi_afterburner_gather_data(MAHM_SHARED_MEMORY_HEADER* hdr,MAHM_SHARED_MEMORY_ENTRY **ent)
{
    void* fm = OpenFileMappingA(FILE_MAP_READ, 0, "Local\\MAHMSharedMemory");
    int ret=-1;

    if(fm)
    {
        MAHM_SHARED_MEMORY_HEADER *shd = MapViewOfFile(fm, FILE_MAP_READ, 0, 0, sizeof(MAHM_SHARED_MEMORY_HEADER));

        if(shd)
        {
            if(memcmp(&shd->dwSignature,"MHAM",4)==0 && shd->dwVersion==0x00020000)
            {
                memcpy(hdr, shd, sizeof(MAHM_SHARED_MEMORY_HEADER));
                UnmapViewOfFile(shd);
                shd=MapViewOfFile(fm, FILE_MAP_READ, 0, 0, hdr->dwHeaderSize+(hdr->dwEntrySize*hdr->dwNumEntries));
                if(shd)
                {
                    *ent=malloc(hdr->dwEntrySize*hdr->dwNumEntries);
                    memcpy(*ent, ((unsigned char*)shd)+hdr->dwHeaderSize, hdr->dwEntrySize*hdr->dwNumEntries);
                    UnmapViewOfFile(shd);
                    ret=0;
                }
            }
            else
            {
                UnmapViewOfFile(shd);
            }
        }
        CloseHandle(fm);
    }
    return (ret);
}
#endif
