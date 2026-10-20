#ifndef POKEBW2_DWCUTIL_DWC_UTIL_USB_INTRO_H
#define POKEBW2_DWCUTIL_DWC_UTIL_USB_INTRO_H

#include "types.h"

// The connection utility's screen before the setup of the Nintendo Wi-Fi USB Connector, after the first screen of the
// connection settings' wide button: yes goes on to the setup (dwc_util_usb_setup.h), no back to the first screen.
// The ROM does not embed this source file's name; dwc_util_usb_intro.c is descriptive.
void DWCUtilUsbIntro_Init(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_USB_INTRO_H
