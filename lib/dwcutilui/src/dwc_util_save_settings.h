#ifndef POKEBW2_DWCUTIL_DWC_UTIL_SAVE_SETTINGS_H
#define POKEBW2_DWCUTIL_DWC_UTIL_SAVE_SETTINGS_H

#include "types.h"

// The connection utility's save of a connection set up by hand: it saves the connection being edited into the
// console's Wi-Fi settings, shows the message of the save, and goes back to the first screen of the connection
// settings.
// The ROM does not embed this source file's name; dwc_util_save_settings.c is descriptive.
void DWCUtilSaveSettings_Init(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_SAVE_SETTINGS_H
