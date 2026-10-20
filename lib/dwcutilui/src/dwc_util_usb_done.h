#ifndef POKEBW2_DWCUTIL_DWC_UTIL_USB_DONE_H
#define POKEBW2_DWCUTIL_DWC_UTIL_USB_DONE_H

#include "types.h"

// The connection utility's message after the setup of the Nintendo Wi-Fi USB Connector (dwc_util_usb_setup.h), which
// stays for 120 frames, before the message of the connection test with auto-connect.
// The ROM does not embed this source file's name; dwc_util_usb_done.c is descriptive.
void DWCUtilUsbDone_Init(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_USB_DONE_H
