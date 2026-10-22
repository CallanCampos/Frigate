#ifndef POKEBW2_DWCUTIL_DWC_UTIL_UI_OBJECT_H
#define POKEBW2_DWCUTIL_DWC_UTIL_UI_OBJECT_H

#include "types.h"

// The settings utility's row of selectable objects, one per Wi-Fi option
// variant. The caller shows them with DWCi_BTNlInit, reads the selection
// back through DWCi_BTNlGet, and hides them with
// DWCi_BTNlEnd while DWCi_BTNlIsEnd reports the exit.
void DWCi_BTNlInit(int variant);
void DWCi_BTNlEnd(void);
int DWCi_BTNlGet(void);
void DWCi_BTNlSet(int state);
void DWCi_BTNlSetForce(int state);
BOOL DWCi_BTNlIsEnd(void);
void DWCi_BTNlEnable(void);
void DWCi_BTNlDisable(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_UI_OBJECT_H
