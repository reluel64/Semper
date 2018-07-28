/*WiFi information source
 * Part of Project 'Semper'
 * Written by Alexandru-Daniel Mărgărit
 */

#include <mem.h>
#include <string_util.h>
#include <semper_api.h>

#ifdef WIN32
#include <windows.h>
#include <wlanapi.h>
#endif

typedef enum
{
    ssid = 1,
    quality,
    encryption,
    auth,
    phy,
    list
} wifi_info;

typedef struct
{
    wifi_info inf;
    size_t wifi_index;
    size_t list_limit;
    unsigned char list_lvl;
    unsigned char *str_val;
    void *wifi_handle;
} wifi_data;

#ifdef WIN32


typedef

enum _IDOT11_PHY_TYPE
{
    _dot11_phy_type_unknown = 0,
    _dot11_phy_type_any = dot11_phy_type_unknown,
    _dot11_phy_type_fhss = 1,
    _dot11_phy_type_dsss = 2,
    _dot11_phy_type_irbaseband = 3,
    _dot11_phy_type_ofdm = 4,
    _dot11_phy_type_hrdsss = 5,
    _dot11_phy_type_erp = 6,
    _dot11_phy_type_ht = 7,
    _dot11_phy_type_vht         = 8,
    _dot11_phy_type_IHV_start = 0x80000000,
    _dot11_phy_type_IHV_end = 0xffffffff
} I_DOT11_PHY_TYPE;

static unsigned char *wifi_get_chiper_algorithm(DOT11_CIPHER_ALGORITHM value)
{
    switch(value)
    {
        case DOT11_CIPHER_ALGO_NONE:
            return "NONE";

        case DOT11_CIPHER_ALGO_WEP40:
            return "WEP40";

        case DOT11_CIPHER_ALGO_TKIP:
            return "TKIP";

        case DOT11_CIPHER_ALGO_CCMP:
            return "AES";

        case DOT11_CIPHER_ALGO_WEP104:
            return "WEP104";

        case DOT11_CIPHER_ALGO_WPA_USE_GROUP:
            return "WPA-GROUP";

        case DOT11_CIPHER_ALGO_WEP:
            return "WEP";

        default:
            return "N/A";
    }
}

static unsigned char *wifi_get_auth_algorithm(DOT11_AUTH_ALGORITHM value)
{
    switch(value)
    {
        case DOT11_AUTH_ALGO_80211_OPEN:
            return "Open";

        case DOT11_AUTH_ALGO_80211_SHARED_KEY:
            return "Shared";

        case DOT11_AUTH_ALGO_WPA:
            return "WPA-Enterprise";

        case DOT11_AUTH_ALGO_WPA_PSK:
            return "WPA-Personal";

        case DOT11_AUTH_ALGO_WPA_NONE:
            return "WPA-NONE";

        case DOT11_AUTH_ALGO_RSNA:
            return "WPA2-Enterprise";

        case DOT11_AUTH_ALGO_RSNA_PSK:
            return "WPA2-Personal";

        default:
            return "N/A";
    }
}



static unsigned char* wifi_get_phy(I_DOT11_PHY_TYPE value)
{
    switch(value)
    {
        case _dot11_phy_type_fhss:
            return "FHSS";

        case _dot11_phy_type_dsss:
            return "DSSS";

        case _dot11_phy_type_irbaseband:
            return "IR-Band";

        case _dot11_phy_type_ofdm:
            return "802.11a";

        case _dot11_phy_type_hrdsss:
            return "802.11b";

        case _dot11_phy_type_erp:
            return "802.11g";

        case _dot11_phy_type_ht:
            return "802.11n";

        case _dot11_phy_type_vht:
            return "802.11ac";

        default:
            return "N/A";
    }
}
#endif

void wifi_init(void **spv, void*ip)
{
    unused_parameter(ip);

    wifi_data *wd = zmalloc(sizeof(wifi_data));
#ifdef WIN32
    unsigned long nver = 0;
    WlanOpenHandle(2, NULL, &nver, &wd->wifi_handle);
#endif
    *spv = wd;
}

void wifi_reset(void *spv, void *ip)
{
    wifi_data *wd = spv;
    unsigned char *s = NULL;
    wd->wifi_index = param_size_t("WiFiInterfaceIndex", ip, 0);
    wd->list_limit = param_size_t("WiFiListEntries", ip, 10);
    wd->list_lvl = (unsigned char)param_size_t("WifiListLevel", ip, 0);
    s = param_string("WifiInfo", 0x3, ip, "List");

    if(s)
    {
        if(!strcasecmp(s, "List"))
        {
            wd->inf = list;
        }
        else if(!strcasecmp(s, "PHY"))
        {
            wd->inf = phy;
        }
        else if(!strcasecmp(s, "Auth"))
        {
            wd->inf = auth;
        }
        else if(!strcasecmp(s, "Encryption"))
        {
            wd->inf = encryption;
        }
        else if(!strcasecmp(s, "Quality"))
        {
            wd->inf = quality;
        }
        else if(!strcasecmp(s, "SSID"))
        {
            wd->inf = ssid;
        }
    }

    if(wd->inf == quality)
    {
        source_set_max(100.0, ip, 1, 1);
        source_set_min(0.0, ip, 1, 1);
    }
    else
    {
        source_set_max(0.0, ip, 1, 0);
        source_set_min(0.0, ip, 1, 0);
    }
}

double wifi_update(void *spv)
{
    double ret = -1.0;
    wifi_data *wd = spv;

    sfree((void**)&wd->str_val);
#ifdef WIN32
    unsigned long nver = 0;

    if(wd->wifi_handle == NULL && WlanOpenHandle(2, NULL, &nver, &wd->wifi_handle) != ERROR_SUCCESS)
    {
        return(-1.0);
    }

    WLAN_INTERFACE_INFO_LIST *if_list = NULL;
    WLAN_INTERFACE_INFO *if_info = NULL;
    WlanEnumInterfaces(wd->wifi_handle, NULL, &if_list);

    if(if_list == NULL)
    {
        return(0.0);
    }

    if(wd->wifi_index < if_list->dwNumberOfItems)
    {
        if_info = &if_list->InterfaceInfo[wd->wifi_index];
    }

    if(wd->inf != list && if_info)
    {
        WLAN_CONNECTION_ATTRIBUTES *attr = NULL;
        size_t sz = 0;

        if(if_info->isState == 1 &&
                WlanQueryInterface(wd->wifi_handle, &if_info->InterfaceGuid, wlan_intf_opcode_current_connection, NULL, (unsigned long*)&sz, (void**)&attr, NULL) == ERROR_SUCCESS)
        {

            switch(wd->inf)
            {
                case quality:
                    ret = (double)attr->wlanAssociationAttributes.wlanSignalQuality;
                    break;

                case ssid:
                    wd->str_val = zmalloc(attr->wlanAssociationAttributes.dot11Ssid.uSSIDLength + 1);
                    strncpy(wd->str_val, attr->wlanAssociationAttributes.dot11Ssid.ucSSID, attr->wlanAssociationAttributes.dot11Ssid.uSSIDLength);
                    break;

                case phy:
                    wd->str_val = clone_string(wifi_get_phy(attr->wlanAssociationAttributes.dot11PhyType));
                    break;

                case encryption:
                    wd->str_val = clone_string(wifi_get_chiper_algorithm(attr->wlanSecurityAttributes.dot11CipherAlgorithm));
                    break;

                case auth:
                    wd->str_val = clone_string(wifi_get_auth_algorithm(attr->wlanSecurityAttributes.dot11AuthAlgorithm));
                    break;

                default:
                    wd->str_val = clone_string("Unknown Option");
            }

            WlanFreeMemory(attr);
        }
    }
    else if(if_info)
    {
        size_t bn = 0;
        size_t buf_pos = 0;
        size_t dups = 0;

        WLAN_AVAILABLE_NETWORK_LIST *av_lis = NULL;

        if(WlanGetAvailableNetworkList(wd->wifi_handle, &if_info->InterfaceGuid, 0, NULL, &av_lis) == ERROR_SUCCESS)
        {
            unsigned char ssid_buf[256] = {0};

            for(size_t i = 0; i < av_lis->dwNumberOfItems && i < wd->list_limit; i++)
            {
                strncpy(ssid_buf, av_lis->Network[i].dot11Ssid.ucSSID, av_lis->Network[i].dot11Ssid.uSSIDLength);
                bn += av_lis->Network[i].dot11Ssid.uSSIDLength + 2;

                if(wd->list_lvl >= 1)
                    bn += string_length(wifi_get_phy(av_lis->Network[i].dot11PhyTypes[0])) + sizeof(" | Band: ");

                if(wd->list_lvl >= 2)
                    bn += string_length(wifi_get_chiper_algorithm(av_lis->Network[i].dot11DefaultCipherAlgorithm)) + sizeof(" | Encryption: ");

                if(wd->list_lvl >= 3)
                    bn += string_length(wifi_get_auth_algorithm(av_lis->Network[i].dot11DefaultAuthAlgorithm)) + sizeof(" | Auth: ");

                if(wd->list_lvl >= 4)
                    bn += 5 + sizeof(" | Quality: ");
            }

            wd->str_val = zmalloc(bn + 1);

            for(size_t i = 0; i < av_lis->dwNumberOfItems && (i - dups) < wd->list_limit; i++)
            {
                memset(ssid_buf, 0, sizeof(ssid_buf));
                strncpy(ssid_buf, av_lis->Network[i].dot11Ssid.ucSSID, av_lis->Network[i].dot11Ssid.uSSIDLength);

                if(strstr(wd->str_val, ssid_buf))
                {
                    dups++;
                    continue;
                }

                strncpy(wd->str_val + buf_pos, av_lis->Network[i].dot11Ssid.ucSSID, av_lis->Network[i].dot11Ssid.uSSIDLength);
                buf_pos += (av_lis->Network[i].dot11Ssid.uSSIDLength);

                if(wd->list_lvl >= 1)
                    buf_pos += sprintf(wd->str_val + buf_pos, " | Band: %s", wifi_get_phy(av_lis->Network[i].dot11PhyTypes[0]));

                if(wd->list_lvl >= 2)
                    buf_pos += sprintf(wd->str_val + buf_pos, " | Encryption: %s", wifi_get_chiper_algorithm(av_lis->Network[i].dot11DefaultCipherAlgorithm));

                if(wd->list_lvl >= 3)
                    buf_pos += sprintf(wd->str_val + buf_pos, " | Auth: %s", wifi_get_auth_algorithm(av_lis->Network[i].dot11DefaultAuthAlgorithm));

                if(wd->list_lvl >= 4)
                    buf_pos += sprintf(wd->str_val + buf_pos, " | Quality: %lu", av_lis->Network[i].wlanSignalQuality);

                wd->str_val[buf_pos++] = '\n';

            }

            if(wd->str_val && buf_pos)
            {
                wd->str_val[buf_pos - 1] = 0;
            }

            WlanFreeMemory(av_lis);
        }
    }


    if(if_list)
    {
        WlanFreeMemory(if_list);
    }

#endif
    return(ret);
}

unsigned char *wifi_string(void *spv)
{
    wifi_data *wd = spv;
    return(wd->str_val);
}

void wifi_destroy(void **spv)
{
    wifi_data *wd = *spv;
    sfree((void**)&wd->str_val);
#ifdef WIN32

    if(wd->wifi_handle)
    {
        WlanCloseHandle(wd->wifi_handle, NULL);
    }

#endif
    sfree(spv);
}
