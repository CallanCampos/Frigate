#ifndef POKEBW2_DWCUTIL_DWC_UTIL_WIFI_SETTINGS_H
#define POKEBW2_DWCUTIL_DWC_UTIL_WIFI_SETTINGS_H

#include "types.h"
#include "dwc/bm.h"
#include "dwcutil/aoss.h"
#include "dwcutil/atermset.h"
#include "struct_decls.h"

// The connection utility's copy of the console's Wi-Fi settings (the four pages of dwc/bm.h's NTR settings), and of
// the one connection that its screens edit, which is saved back into its page and written to the NVRAM.
// The ROM does not embed this source file's name; dwc_util_wifi_settings.c is descriptive. dwcutil/dwc_moveid.h
// declares this file's DWCi_SETTINGlGetMemMap, the pages.

// The copy of a connection of the console's Wi-Fi settings that the screens edit: the connection, its subnet mask as
// four bytes (the connection keeps the number of its bits), which connection it is, and whether DHCP gives the address,
// the mask and the gateway, and the DNS servers
struct DWCUtilSettingsEdit {
    DWCBMApInfo connection;
    u8 netmask[4];
    u8 index;
    u8 autoAddress;
    u8 autoDns;
};

// Reads the console's Wi-Fi settings, and frees them
void DWCi_SETTIGNlInit(void);
void DWCi_SETTIGNlEnd(void);
DWCUtilSettingsEdit *DWCUtilWifiSettings_GetEdit(void);
// Sets whether DHCP gives the address, the mask and the gateway, and the DNS servers, of the connection being edited
void DWCi_SETTINGlSetIpAuto(BOOL autoAddress);
void DWCi_SETTINGlSetDnsAuto(BOOL autoDns);
// Takes an SSID or a WEP key into the connection being edited: a key of 10, 26 or 32 hex digits as their bytes, any
// other as its characters
void DWCi_SETTINGlSetSsid(const u8 *ssid);
void DWCi_SETTINGlSetWep(const u8 *key);
// The addresses of the connection being edited (its IP address, subnet mask, gateway and two DNS servers) from 12
// digits, each number right-aligned in three, and back
void DWCUtilWifiSettings_SetIp(const u8 *digits);
void DWCUtilWifiSettings_SetNetmask(const u8 *digits);
void DWCi_SETTINGlSetGateway(const u8 *digits);
void DWCi_SETTINGlSetDnsP(const u8 *digits);
void DWCi_SETTINGlSetDnsS(const u8 *digits);
// Copies the SSID of the connection being edited
void DWCi_SETTINGlGetSsid(u8 *ssid);
void DWCi_SETTINGlGetIp(u8 *digits);
void DWCi_SETTINGlGetNetMask(u8 *digits);
void DWCi_SETTINGlGetGateway(u8 *digits);
void DWCi_SETTINGlGetDnsP(u8 *digits);
void DWCi_SETTINGlGetDnsS(u8 *digits);
// The status of a connection of the console's Wi-Fi settings (DWCBMApInfo.status)
u8 DWCi_SETTINGlGetSetType(int index);
// Makes the connection of the console's Wi-Fi settings the one being edited
void DWCi_SETTINGlSetSetting(int index);
// Saves the connection being edited into its connection of the console's Wi-Fi settings, and writes the page
void DWCi_SETTINGlSet(void);
// Saves the access point that the connection test connected to, with its SSID and WEP keys, into the connection being
// edited, and the keys that an AOSS setup got
void DWCi_SETTINGlSetRaku(const ATERMApConfigParam *result);
void DWCi_SETTINGlSetAoss(const AOSS_SECURE_INFO *result);
// Erases a connection of the console's Wi-Fi settings, and all of them with a new Wi-Fi ID
void DWCi_SETTINGlErasePage(int index);
void DWCi_SETTINGlEraseAll(void);
// Converts 12 digits to an address
void DWCi_SETTINGlConvAddress(const u8 *digits, u8 *address);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_WIFI_SETTINGS_H
