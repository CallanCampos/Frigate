#ifndef POKEBW2_DWCUTIL_DWC_UTIL_ADDRESS_SETTINGS_H
#define POKEBW2_DWCUTIL_DWC_UTIL_ADDRESS_SETTINGS_H

#include "types.h"
#include "nitro/gx.h"

// The connection utility's settings of the connection being edited: a list that scrolls, of its SSID, its WEP key,
// whether DHCP gives the address, the address, the subnet mask, the gateway, whether DHCP gives the DNS servers and
// the two DNS servers, with buttons above and below it. A row opens its keyboard or keypad, and the buttons run the
// connection test with the settings, save them, or go back.
// The ROM does not embed this source file's name; dwc_util_address_settings.c is descriptive.
void DWCUtilAddressSettings_Init(void);
// Where the settings were opened from: 0 from the first screen of the connection settings, 1 from the choice of the
// setup of a connection
void DWCUtilAddressSettings_SetFrom(int from);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_ADDRESS_SETTINGS_H
