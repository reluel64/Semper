/*
 * Network traffic monitor source
 * Early implemntation
 * Part of Project 'Semper'
 * */

#include <stdio.h>
#include <mem.h>
#include <string_util.h>
#include <semper_api.h>

#if defined(WIN32)
#include <windows.h>
#include <Ntddndis.h>
#include <Iphlpapi.h>
typedef struct _MIB_IF_ROW2
{
    NET_LUID                   InterfaceLuid;
    NET_IFINDEX                InterfaceIndex;
    GUID                       InterfaceGuid;
    WCHAR                      Alias[IF_MAX_STRING_SIZE + 1];
    WCHAR                      Description[IF_MAX_STRING_SIZE + 1];
    ULONG                      PhysicalAddressLength;
    UCHAR                     PhysicalAddress[IF_MAX_PHYS_ADDRESS_LENGTH];
    UCHAR                     PermanentPhysicalAddress[IF_MAX_PHYS_ADDRESS_LENGTH];
    ULONG                      Mtu;
    IFTYPE                     Type;
    TUNNEL_TYPE                TunnelType;
    NDIS_MEDIUM                MediaType;
    NDIS_PHYSICAL_MEDIUM       PhysicalMediumType;
    NET_IF_ACCESS_TYPE         AccessType;
    NET_IF_DIRECTION_TYPE      DirectionType;
    struct
    {
        BOOLEAN HardwareInterface  : 1;
        BOOLEAN FilterInterface  : 1;
        BOOLEAN ConnectorPresent   : 1;
        BOOLEAN NotAuthenticated  : 1;
        BOOLEAN NotMediaConnected  : 1;
        BOOLEAN Paused  : 1;
        BOOLEAN LowPower  : 1;
        BOOLEAN EndPointInterface  : 1;
    } InterfaceAndOperStatusFlags;
    IF_OPER_STATUS             OperStatus;
    NET_IF_ADMIN_STATUS        AdminStatus;
    NET_IF_MEDIA_CONNECT_STATE MediaConnectState;
    NET_IF_NETWORK_GUID        NetworkGuid;
    NET_IF_CONNECTION_TYPE     ConnectionType;
    ULONG64                    TransmitLinkSpeed;
    ULONG64                    ReceiveLinkSpeed;
    ULONG64                    InOctets;
    ULONG64                    InUcastPkts;
    ULONG64                    InNUcastPkts;
    ULONG64                    InDiscards;
    ULONG64                    InErrors;
    ULONG64                    InUnknownProtos;
    ULONG64                    InUcastOctets;
    ULONG64                    InMulticastOctets;
    ULONG64                    InBroadcastOctets;
    ULONG64                    OutOctets;
    ULONG64                    OutUcastPkts;
    ULONG64                    OutNUcastPkts;
    ULONG64                    OutDiscards;
    ULONG64                    OutErrors;
    ULONG64                    OutUcastOctets;
    ULONG64                    OutMulticastOctets;
    ULONG64                    OutBroadcastOctets;
    ULONG64                    OutQLen;
} MIB_IF_ROW2, *PMIB_IF_ROW2;


typedef struct _MIB_IF_TABLE2
{
    ULONG       NumEntries;
    MIB_IF_ROW2 Table[ANY_SIZE];
} MIB_IF_TABLE2, *PMIB_IF_TABLE2;

DWORD WINAPI GetIfTable2(_Out_ PMIB_IF_TABLE2 *Table);
VOID WINAPI FreeMibTable(_In_ PVOID Memory);

#elif defined(__linux__)
#include <dirent.h>
#endif

typedef enum
{
    total,
    download,
    upload,
    unknown = 255
} network_info;

typedef struct
{
    unsigned char best; //always choose best interface
    network_info inf; //information that should be returned
    size_t if_index;   //interface index
    size_t old_bytes;  //holder for the old value(we need this value to do the actual computation)
    time_t t;
    unsigned char qty;
    size_t qty_bytes;
    double v;
} network;

static size_t network_interface_index(unsigned char *iname);
static size_t network_get_bytes(network *n);

void network_init(void**spv, void *ip)
{
    unused_parameter(ip);
    network *n = zmalloc(sizeof(network));
    *spv = n;
}

void network_reset(void *spv, void *ip)
{

    network *n = spv;
    char *t = NULL;
    unsigned char *p = param_string("Type", EXTENSION_XPAND_ALL, ip, "Total");

    if(p)
    {
        if(!strcasecmp(p, "Total"))
            n->inf = total;
        else if(!strcasecmp(p, "Download"))
            n->inf = download;
        else if(!strcasecmp(p, "Upload"))
            n->inf = upload;
        else
            n->inf = unknown;
    }

    n->qty = param_bool("Quantity", ip, 0);
    p = param_string("Interface", EXTENSION_XPAND_ALL, ip, "0");
    n-> if_index = strtoul(p, &t, 10);

#if defined(WIN32)

    if(p == (unsigned char*)t)
    {
        n->best = !strcasecmp(p, "Best");

        if(n->best)
        {
            n->if_index = network_interface_index(p);
        }
    }
    else
    {
        n->if_index++;
        n->best = 0;
    }

#endif
}



double network_update(void *spv)
{

    network *n = spv;
    time_t ct = time(NULL);

    size_t new_value = 0;

    if(ct - n->t < 1)
    {
        return(n->v); //we'll not update the speed if the interval is less than a second
    }

    new_value = network_get_bytes(n);

    if(new_value >= n->old_bytes && n->old_bytes)
    {
        n->v = (double)(new_value - n->old_bytes);
        n->qty_bytes += (new_value - n->old_bytes);
        n->old_bytes = new_value;

        if(n->qty)
        {
            n->v = (double)n->qty_bytes;
        }
    }
    else
    {
        n->old_bytes = new_value;
    }

    n->t = ct; //set the new time

    return(n->v);
}

void network_destroy(void **spv)
{
    sfree(spv);
}



static size_t network_get_bytes(network *n)
{
    size_t c_bytes = 0;
#if defined(WIN32)
    MIB_IF_TABLE2 *tbl = NULL;


    if(GetIfTable2(&tbl) != NO_ERROR)
    {
        return(0);
    }

    if(n->best)
    {
        if(GetBestInterface(INADDR_ANY, (DWORD*)&n->if_index) != NO_ERROR)
        {
            n->if_index = 0; //get the data from all the interfaces (until next call)
            n->best |= 0x2; //indicate an error
        }
        else
        {
            for(size_t i = 0; i < tbl->NumEntries; i++)
            {
                if(tbl->Table[i].InterfaceIndex == (NET_IFINDEX)n->if_index)
                {
                    n->if_index = i + 1;
                    break;
                }
            }

            n->best = 1; //all clear
        }
    }

    if(n->if_index - 1 && n->if_index - 1 <= tbl->NumEntries)
    {

        switch(n->inf)
        {
            case total:
                c_bytes += tbl->Table[n->if_index - 1].InOctets;
                c_bytes += tbl->Table[n->if_index - 1].OutOctets;
                break;

            case download:
                c_bytes += tbl->Table[n->if_index - 1].InOctets;
                break;

            case upload:
                c_bytes += tbl->Table[n->if_index - 1].OutOctets;
                break;

            case unknown:
                c_bytes = 0;
                break;
        }
    }

    else
    {
        for(size_t i = 0; i < tbl->NumEntries; i++)
        {
            if(tbl->Table[i].Type == IF_TYPE_SOFTWARE_LOOPBACK ||
                    tbl->Table[i].InterfaceAndOperStatusFlags.FilterInterface == 1)
                continue;

            switch(n->inf)
            {
                case total:
                    c_bytes += tbl->Table[i].InOctets;
                    c_bytes += tbl->Table[i].OutOctets;
                    break;

                case download:
                    c_bytes += tbl->Table[i].InOctets;
                    break;

                case upload:
                    c_bytes += tbl->Table[i].OutOctets;
                    break;

                case unknown:
                    c_bytes = 0;
                    break;
            }
        }
    }

    FreeMibTable(tbl);
#elif defined(__linux__)
    DIR *dh = opendir("/sys/class/net");
    struct dirent *dir = NULL;
    unsigned char buf[512] = {0};
    size_t rx = 0;
    size_t tx = 0;

    if(dh == NULL)
        return(0);

    while((dir = readdir(dh)) != NULL)
    {
        size_t lifindex = network_interface_index(dir->d_name);

        if(n->if_index == 0 || lifindex == n->if_index - 1)
        {
            size_t lrx = 0;
            size_t ltx = 0;

            snprintf(buf, 512, "/sys/class/net/%s/statistics/rx_bytes", dir->d_name);
            FILE *f = fopen(buf, "r");

            if(f)
            {
                fscanf(f, "%lu", &lrx);
                fclose(f);
            }

            snprintf(buf, 512, "/sys/class/net/%s/statistics/tx_bytes", dir->d_name);
            f = fopen(buf, "r");

            if(f)
            {
                fscanf(f, "%lu", &ltx);
                fclose(f);
            }

            rx += lrx;
            tx += ltx;

            if(lifindex == n->if_index - 1)
                break;
        }
    }

    closedir(dh);

    if(n->inf == total)
        c_bytes = rx + tx;
    else if(n->inf == download)
        c_bytes = rx;
    else if(n->inf == upload)
        c_bytes = tx;

#endif
    return(c_bytes);
}


static size_t network_interface_index(unsigned char *iname)
{

    size_t if_index = 0;

    if(iname == NULL)
    {
        return(0);
    }

#if defined(WIN32)

    MIB_IF_TABLE2 *tbl = NULL;
    unsigned short *uc = utf8_to_ucs(iname);

    if(GetIfTable2(&tbl) != NO_ERROR)
    {
        sfree((void**)&uc);
        return(0);
    }

    for(size_t i = 0; i < tbl->NumEntries; i++)
    {
        if(!wcsicmp(uc, tbl->Table[i].Description))
        {
            if_index = tbl->Table[i].InterfaceIndex + 1;
            break;
        }
    }

    FreeMibTable(tbl);
    sfree((void**)&uc);

#elif defined(__linux__)

    unsigned char buf[256] = {0};
    snprintf(buf, 256, "/sys/class/net/%s/ifindex", iname);
    FILE *f = fopen(buf, "r");

    if(f == NULL)
    {
        return(0);
    }
    else
    {
        fscanf(f, "%lu", &if_index);
        fclose(f);
    }

#endif
    return(if_index);
}
