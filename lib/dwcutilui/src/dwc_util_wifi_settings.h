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
// declares this file's func_ov182_021b7e6c, the pages.

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
void func_ov182_021b77dc(void);
void func_ov182_021b780c(void);
DWCUtilSettingsEdit *func_ov182_021b781c(void);
// Sets whether DHCP gives the address, the mask and the gateway, and the DNS servers, of the connection being edited
void func_ov182_021b782c(BOOL autoAddress);
void func_ov182_021b7840(BOOL autoDns);
// Takes an SSID or a WEP key into the connection being edited: a key of 10, 26 or 32 hex digits as their bytes, any
// other as its characters
void func_ov182_021b7854(const u8 *ssid);
void func_ov182_021b7874(const u8 *key);
// The addresses of the connection being edited (its IP address, subnet mask, gateway and two DNS servers) from 12
// digits, each number right-aligned in three, and back
void func_ov182_021b7990(const u8 *digits);
void func_ov182_021b79a8(const u8 *digits);
void func_ov182_021b79c0(const u8 *digits);
void func_ov182_021b79d8(const u8 *digits);
void func_ov182_021b79f0(const u8 *digits);
// Copies the SSID of the connection being edited
void func_ov182_021b7a08(u8 *ssid);
void func_ov182_021b7a24(u8 *digits);
void func_ov182_021b7a50(u8 *digits);
void func_ov182_021b7a7c(u8 *digits);
void func_ov182_021b7aac(u8 *digits);
void func_ov182_021b7adc(u8 *digits);
// The status of a connection of the console's Wi-Fi settings (DWCBmConnection.status)
u8 func_ov182_021b7b0c(int index);
// Makes the connection of the console's Wi-Fi settings the one being edited
void func_ov182_021b7b20(int index);
// Saves the connection being edited into its connection of the console's Wi-Fi settings, and writes the page
void func_ov182_021b7bb0(void);
// Saves the access point that the connection test connected to, with its SSID and WEP keys, into the connection being
// edited, and the keys that an AOSS setup got
void func_ov182_021b7c60(const DWCUtilAcResult *result);
void func_ov182_021b7d80(const AossResult *result);
// Erases a connection of the console's Wi-Fi settings, and all of them with a new Wi-Fi ID
void func_ov182_021b7e78(int index);
void func_ov182_021b7ea0(void);
// Converts 12 digits to an address
void func_ov182_021b7ff0(const u8 *digits, u8 *address);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_WIFI_SETTINGS_H
