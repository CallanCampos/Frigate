#ifndef POKEBW2_DWCUTIL_DWC_UTIL_WIFI_SETTINGS_H
#define POKEBW2_DWCUTIL_DWC_UTIL_WIFI_SETTINGS_H

#include "types.h"
#include "dwc/bm.h"
#include "dwcutil/dwc_aoss.h"
#include "dwcutil/dwcutil_ac.h"
#include "struct_decls.h"

// The connection utility's copy of the console's Wi-Fi settings (the four pages of dwc/bm.h's NTR settings), and of
// the one connection that its screens edit, which is saved back into its page and written to the NVRAM.
// The ROM does not embed this source file's name; dwc_util_wifi_settings.c is descriptive. app/download_play_sys.h
// declares this file's DWCUtilWifiSettings_GetPages, the pages.

// The copy of a connection of the console's Wi-Fi settings that the screens edit: the connection, its subnet mask as
// four bytes (the connection keeps the number of its bits), which connection it is, and whether DHCP gives the address,
// the mask and the gateway, and the DNS servers
struct DWCUtilSettingsEdit {
    DWCBmConnection connection;
    u8 netmask[4];
    u8 index;
    u8 autoAddress;
    u8 autoDns;
};

// Reads the console's Wi-Fi settings, and frees them
void DWCUtilWifiSettings_Init(void);
void DWCUtilWifiSettings_Exit(void);
DWCUtilSettingsEdit *DWCUtilWifiSettings_GetEdit(void);
// Sets whether DHCP gives the address, the mask and the gateway, and the DNS servers, of the connection being edited
void DWCUtilWifiSettings_SetAutoAddress(BOOL autoAddress);
void DWCUtilWifiSettings_SetAutoDns(BOOL autoDns);
// Takes an SSID or a WEP key into the connection being edited: a key of 10, 26 or 32 hex digits as their bytes, any
// other as its characters
void DWCUtilWifiSettings_SetSsid(const u8 *ssid);
void DWCUtilWifiSettings_SetWepKey(const u8 *key);
// The addresses of the connection being edited (its IP address, subnet mask, gateway and two DNS servers) from 12
// digits, each number right-aligned in three, and back
void DWCUtilWifiSettings_SetIp(const u8 *digits);
void DWCUtilWifiSettings_SetNetmask(const u8 *digits);
void DWCUtilWifiSettings_SetGateway(const u8 *digits);
void DWCUtilWifiSettings_SetDns1(const u8 *digits);
void DWCUtilWifiSettings_SetDns2(const u8 *digits);
// Copies the SSID of the connection being edited
void DWCUtilWifiSettings_GetSsid(u8 *ssid);
void DWCUtilWifiSettings_GetIp(u8 *digits);
void DWCUtilWifiSettings_GetNetmask(u8 *digits);
void DWCUtilWifiSettings_GetGateway(u8 *digits);
void DWCUtilWifiSettings_GetDns1(u8 *digits);
void DWCUtilWifiSettings_GetDns2(u8 *digits);
// The status of a connection of the console's Wi-Fi settings (DWCBmConnection.status)
u8 DWCUtilWifiSettings_GetStatus(int index);
// Makes the connection of the console's Wi-Fi settings the one being edited
void DWCUtilWifiSettings_EditConnection(int index);
// Saves the connection being edited into its connection of the console's Wi-Fi settings, and writes the page
void DWCUtilWifiSettings_Save(void);
// Saves the access point that the connection test connected to, with its SSID and WEP keys, into the connection being
// edited, and the keys that an AOSS setup got
void DWCUtilWifiSettings_SaveAcResult(const DWCUtilAcResult *result);
void DWCUtilWifiSettings_SaveAossResult(const AossResult *result);
// Erases a connection of the console's Wi-Fi settings, and all of them with a new Wi-Fi ID
void DWCUtilWifiSettings_Erase(int index);
void DWCUtilWifiSettings_EraseAll(void);
// Converts 12 digits to an address
void DWCUtilWifiSettings_DigitsToAddress(const u8 *digits, u8 *address);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_WIFI_SETTINGS_H
