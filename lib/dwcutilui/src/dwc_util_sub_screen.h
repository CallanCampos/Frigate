#ifndef POKEBW2_DWCUTIL_DWC_UTIL_SUB_SCREEN_H
#define POKEBW2_DWCUTIL_DWC_UTIL_SUB_SCREEN_H

#include "types.h"

// The connection utility's sub screen: its BG 1 for each of the three steps of a setup, the options and the main menu,
// and the row of five objects of the steps, which slides in from the left one object after the other and out again.
// The ROM does not embed this source file's name; dwc_util_sub_screen.c is descriptive.

// The screens of the sub screen's BG 1; the first three have the row of objects
enum {
    DWC_UTIL_SUB_SCREEN_STEP1,
    DWC_UTIL_SUB_SCREEN_STEP2,
    DWC_UTIL_SUB_SCREEN_STEP3,
    DWC_UTIL_SUB_SCREEN_OPTION,
    DWC_UTIL_SUB_SCREEN_TOP,
};

// Slides the row of a step in, and shows the row of another step in its place
void func_ov182_021b8154(int step);
void func_ov182_021b84c0(int step);
// Slides the row out and frees it; func_ov182_021b854c is whether a slide runs, func_ov182_021b87b0 whether no row is
// sliding out
void func_ov182_021b849c(void);
BOOL func_ov182_021b854c(void);
BOOL func_ov182_021b87b0(void);
// Loads a screen as the sub screen's BG 1 at the next VBlank
void func_ov182_021b87d0(int screen);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_SUB_SCREEN_H
