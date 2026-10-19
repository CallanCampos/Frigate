#ifndef POKEBW2_DWCUTIL_DWC_UTIL_MAIN_H
#define POKEBW2_DWCUTIL_DWC_UTIL_MAIN_H

#include "types.h"

// The connection utility's entry point, DWC_StartUtility (dwc/utility.h), and its main loop: the screen's update
// function runs once a frame until a screen ends the utility. The screens hand each other two pairs of values, and
// read the language and the starting screen they were started with and the flags.
// The ROM does not embed this source file's name; dwc_util_main.c is descriptive.

// Sets the function that runs each frame
void func_ov182_021adfbc(void (*update)(void));
void func_ov182_021adfc8(u32 value0, u32 value1);
// Reads the values that func_ov182_021adfc8 set, into the pointers that aren't NULL
void func_ov182_021adfd4(u32 *value0, u32 *value1);
void func_ov182_021adff0(u32 value2, u32 value3);
void func_ov182_021adffc(u32 *value2, u32 *value3);
// The utility's language
u8 func_ov182_021ae018(void);
// The starting screen, the low 4 bits of DWC_StartUtility's param
u32 func_ov182_021ae024(void);
// Whether any of the flags in mask, the rest of the param, are set
BOOL func_ov182_021ae034(u32 mask);
// Ends the main loop after this frame
void func_ov182_021ae04c(void);
u8 func_ov182_021ae058(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_MAIN_H
