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

// The copy of a connection of the console's Wi-Fi settings that the screens edit, and which connection it is
struct DWCUtilSettingsEdit {
    DWCBmConnection connection;
    u8 unk_ef[5];
    u8 index;
};

DWCUtilSettingsEdit *func_ov182_021b781c(void);
// Takes the WEP keys that the AOSS setup got into the connection being edited
void func_ov182_021b7d80(const AossResult *result);
// Takes the SSID and the WEP mode of the access point that the connection test connected to
void func_ov182_021b7c60(const DWCUtilAcResult *result);

// The rows of buttons of the two layouts
void func_ov182_021b8154(int variant);
void func_ov182_021b84c0(int variant);

// The utility's sound: its start and its end, around the main loop
void func_ov182_021b8050(void);
void func_ov182_021b80d4(void);
// Starts the sound effect, from the utility's sound archive
void func_ov182_021b80f4(int seq);

// Reads the console's Wi-Fi settings, and frees them
void func_ov182_021b77dc(void);
void func_ov182_021b780c(void);
// Erases the connections of the console's Wi-Fi settings and makes a new Wi-Fi ID
void func_ov182_021b7ea0(void);

// Copies row src of the palette to BG palette row dest of the main screen at the next VBlank
void func_ov182_021b7014(const void *palette, int src, int dest);
// Loads the sub screen's BG 1 of one of the layouts at the next VBlank
void func_ov182_021b87d0(int layout);

// A dialog over the main screen with the message (as DWCUtilScreen_Start shows it) and the buttons of a layout.
// func_ov182_021b8f14 is the button chosen, -2 until one is; func_ov182_021b8eec closes the dialog, and
// func_ov182_021b8f24 is TRUE until it has closed
void func_ov182_021b8cf0(int msgIndex, int layout, int flag, int slot, int value);
void func_ov182_021b8eec(void);
int func_ov182_021b8f14(void);
BOOL func_ov182_021b8f24(void);

// The first screen of the connection settings
void func_ov182_021b536c(void);
// The confirmation before the move of the settings to another console
void func_ov182_021afba4(void);

// The whole touch screen, a rectangle as in dwc_util_display.c: .rodata between dwc_util_text.c's and
// dwc_util_task.c's that no file claims yet
extern const u16 data_ov182_021c059c[4];

#endif // POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H
