#ifndef POKEBW2_DWCUTIL_DWC_UTIL_USB_SETUP_H
#define POKEBW2_DWCUTIL_DWC_UTIL_USB_SETUP_H

#include "types.h"

// The connection utility's setup of the Nintendo Wi-Fi USB Connector: it shows the console's nickname and searches for
// the connector (dwc_util_usb_scan.h) until the connector takes the console, which goes on to the message of
// dwc_util_usb_done.h. When none is found, it goes back to the screen before (dwc_util_usb_intro.h), and B, or closing
// the lid, back to the first screen of the connection settings.
// The ROM does not embed this source file's name; dwc_util_usb_setup.c is descriptive.
void DWCUtilUsbSetup_Init(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_USB_SETUP_H
