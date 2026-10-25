#ifndef POKEBW2_DWCUTIL_DWC_UTIL_MAIN_H
#define POKEBW2_DWCUTIL_DWC_UTIL_MAIN_H

#include "types.h"

// The connection utility's entry point, DWC_StartUtility (dwc/utility.h), and its main loop: the screen's update
// function runs once a frame until a screen ends the utility. The screens hand each other two pairs of values, and
// read the language and the starting screen they were started with and the flags.
// The file is the DWC utility library's main.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to that
// library's objects (layout/symbols_sdk.txt) shows it; it keeps a name of ours, since dsd takes each file name once and
// src/system/main.c has main.c.

// Sets the function that runs each frame
void DWCi_ChangeScene(void (*update)(void));
void DWCi_SetParam_(u32 value0, u32 value1);
// Reads the values that DWCi_SetParam_ set, into the pointers that aren't NULL
void DWCi_GetParam_(u32 *value0, u32 *value1);
void DWCi_SetParam2_(int value2, int value3);
void DWCi_GetParam2_(int *value2, int *value3);
// The utility's language
u8 DWCi_Language(void);
// The starting screen, the low 4 bits of DWC_StartUtility's param
u32 DWCi_Entry(void);
// Whether any of the flags in mask, the rest of the param, are set
BOOL DWCi_Flag(u32 mask);
// Ends the main loop after this frame
void DWCi_EndUtility(void);
u8 kDWCi_Language(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_MAIN_H
