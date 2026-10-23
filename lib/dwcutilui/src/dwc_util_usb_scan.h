#ifndef POKEBW2_DWCUTIL_DWC_UTIL_USB_SCAN_H
#define POKEBW2_DWCUTIL_DWC_UTIL_USB_SCAN_H

#include "types.h"

// The results of the search for the Nintendo Wi-Fi USB Connector: none found, a connector that takes the console, and
// connectors of which some take it and some don't
enum {
    DWC_UTIL_USB_SCAN_NONE,
    DWC_UTIL_USB_SCAN_FOUND,
    DWC_UTIL_USB_SCAN_SEVERAL,
};

typedef void (*DWCUtilUsbScanCallback)(int result);

// The connection utility's search for the Nintendo Wi-Fi USB Connector: a scan for the access points whose SSID starts
// with NWCUSBAP, which calls the callback with a result. DWCi_USBAPlBegin starts it (FALSE when it couldn't),
// DWCi_USBAPlEnd ends it (FALSE when the wireless couldn't be stopped), and DWCi_USBAPlSetCallback changes the
// callback; with none, a connector found is kept for the next callback.
// The ROM does not embed this source file's name; dwc_util_usb_scan.c is descriptive.
BOOL DWCi_USBAPlBegin(DWCUtilUsbScanCallback callback);
BOOL DWCi_USBAPlEnd(void);
void DWCi_USBAPlSetCallback(DWCUtilUsbScanCallback callback);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_USB_SCAN_H
