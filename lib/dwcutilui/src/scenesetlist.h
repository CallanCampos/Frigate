#ifndef POKEBW2_DWCUTIL_SCENESETLIST_H
#define POKEBW2_DWCUTIL_SCENESETLIST_H

#include "types.h"
#include "nitro/gx.h"

// The connection utility's settings of the connection being edited: a list that scrolls, of its SSID, its WEP key,
// whether DHCP gives the address, the address, the subnet mask, the gateway, whether DHCP gives the DNS servers and
// the two DNS servers, with buttons above and below it. A row opens its keyboard or keypad, and the buttons run the
// connection test with the settings, save them, or go back.
// The file is the DWC utility library's scenesetlist.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code
// to that library's objects (layout/symbols_sdk.txt) shows it.
void DWCUtilAddressSettings_Init(void);
// Where the settings were opened from: 0 from the first screen of the connection settings, 1 from the choice of the
// setup of a connection
void DWCi_SetListReturn(int from);

#endif // POKEBW2_DWCUTIL_SCENESETLIST_H
