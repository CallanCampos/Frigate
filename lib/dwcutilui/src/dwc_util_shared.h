#ifndef POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H
#define POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H

#include "types.h"
#include "dwc/bm.h"
#include "dwc_util_cell.h"
#include "dwc_util_display.h"
#include "dwc_util_heap.h"
#include "dwc_util_list.h"
#include "dwc_util_oam.h"
#include "dwc_util_oam_buf.h"
#include "dwc_util_task.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "dwc_util_vram_alloc.h"
#include "dwcutil/dwc_aoss.h"
#include "dwcutil/dwcutil_ac.h"
#include "nitro/gx.h"
#include "struct_decls.h"

// Entry points in later, still unrecovered files of the DWC settings utility.

// The rows of buttons of the two layouts; func_ov182_021b849c starts the task that moves them away,
// func_ov182_021b854c is whether that task runs and func_ov182_021b87b0 whether they are idle
void func_ov182_021b8154(int variant);
void func_ov182_021b84c0(int variant);
void func_ov182_021b849c(void);
BOOL func_ov182_021b854c(void);
BOOL func_ov182_021b87b0(void);

// The utility's sound: its start and its end, around the main loop
void func_ov182_021b8050(void);
void func_ov182_021b80d4(void);
// Starts the sound effect, from the utility's sound archive
void func_ov182_021b80f4(int seq);
// Stops the music
void func_ov182_021b8138(void);

// Loads the sub screen's BG 1 of one of the layouts at the next VBlank
void func_ov182_021b87d0(int layout);

// A dialog over the main screen with the message (as DWCUtilScreen_Start shows it) and the buttons of a layout.
// func_ov182_021b8f14 is the button chosen, -2 until one is; func_ov182_021b8eec closes the dialog, and
// func_ov182_021b8f24 is TRUE until it has closed
void func_ov182_021b8cf0(int msgIndex, int layout, int flag, int slot, int value);
void func_ov182_021b8eec(void);
int func_ov182_021b8f14(void);
BOOL func_ov182_021b8f24(void);

// The search for the Nintendo Wi-Fi USB Connector and its results (its file is not written yet)
enum {
    DWC_UTIL_USB_SCAN_NONE,
    DWC_UTIL_USB_SCAN_FOUND,
    DWC_UTIL_USB_SCAN_SEVERAL,
};
typedef void (*DWCUtilUsbScanCallback)(int result);
BOOL func_ov182_021b8834(DWCUtilUsbScanCallback callback);
BOOL func_ov182_021b895c(void);
void func_ov182_021b89e8(DWCUtilUsbScanCallback callback);

// The volume and the pitch of the sound effect (their file is not written yet)
void func_ov182_021b810c(int volume);
void func_ov182_021b8120(u32 trackMask, int pitch);

// The whole touch screen, a rectangle as in dwc_util_display.c: .rodata between dwc_util_text.c's and
// dwc_util_task.c's that no file claims yet
extern const u16 data_ov182_021c059c[4];

#endif // POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H
