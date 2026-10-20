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

// The whole touch screen, a rectangle as in dwc_util_display.c: .rodata between dwc_util_text.c's and
// dwc_util_task.c's that no file claims yet
extern const u16 data_ov182_021c059c[4];

#endif // POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H
