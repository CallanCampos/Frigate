#ifndef POKEBW2_DWCUTIL_DWC_UTIL_MAIN_H
#define POKEBW2_DWCUTIL_DWC_UTIL_MAIN_H

#include "types.h"

// The connection utility's entry point, DWC_StartUtility (dwc/utility.h), and its main loop: the screen's update
// function runs once a frame until a screen ends the utility. The screens hand each other two pairs of values, and
// read the language and the starting screen they were started with and the flags.
// The ROM does not embed this source file's name; dwc_util_main.c is descriptive.

// Sets the function that runs each frame
void DWCUtilMain_SetUpdate(void (*update)(void));
void DWCUtilMain_SetLayout(u32 value0, u32 value1);
// Reads the values that DWCUtilMain_SetLayout set, into the pointers that aren't NULL
void DWCUtilMain_GetLayout(u32 *value0, u32 *value1);
void DWCUtilMain_SetScreenArgs(int value2, int value3);
void DWCUtilMain_GetScreenArgs(int *value2, int *value3);
// The utility's language
u8 DWCUtilMain_GetLanguage(void);
// The starting screen, the low 4 bits of DWC_StartUtility's param
u32 DWCUtilMain_GetStartScreen(void);
// Whether any of the flags in mask, the rest of the param, are set
BOOL DWCUtilMain_HasFlag(u32 mask);
// Ends the main loop after this frame
void DWCUtilMain_Finish(void);
u8 func_ov182_021ae058(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_MAIN_H
