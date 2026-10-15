#ifndef POKEBW2_DWCUTIL_DWC_UTIL_UI_OBJECT_H
#define POKEBW2_DWCUTIL_DWC_UTIL_UI_OBJECT_H

#include "types.h"

// The settings utility's row of selectable objects, one per Wi-Fi option
// variant. The caller shows them with DWCUtilUiObject_Init, reads the selection
// back through DWCUtilUiObject_GetState, and hides them with
// DWCUtilUiObject_Exit while DWCUtilUiObject_IsIdle reports the exit.
void DWCUtilUiObject_Init(int variant);
void DWCUtilUiObject_Exit(void);
int DWCUtilUiObject_GetState(void);
void DWCUtilUiObject_SetStateIfIdle(int state);
void DWCUtilUiObject_SetState(int state);
BOOL DWCUtilUiObject_IsIdle(void);
void DWCUtilUiObject_Enable(void);
void DWCUtilUiObject_Disable(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_UI_OBJECT_H
