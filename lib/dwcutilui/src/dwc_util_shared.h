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

// The copy of a connection of the console's Wi-Fi settings that the screens edit: the connection, its subnet mask as
// four bytes (the connection keeps the number of its bits), which connection it is, and whether DHCP gives the address,
// the mask and the gateway, and the DNS servers
struct DWCUtilSettingsEdit {
    DWCBmConnection connection;
    u8 unk_ef;
    u8 netmask[4];
    u8 index;
    u8 autoAddress;
    u8 autoDns;
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
// Stops the music
void func_ov182_021b8138(void);

// Reads the console's Wi-Fi settings, and frees them
void func_ov182_021b77dc(void);
void func_ov182_021b780c(void);
// Erases the connections of the console's Wi-Fi settings and makes a new Wi-Fi ID
void func_ov182_021b7ea0(void);
// Saves the connection being edited into its connection of the console's Wi-Fi settings
void func_ov182_021b7bb0(void);
// Sets whether DHCP gives the address, the mask and the gateway, and the DNS servers, of the connection being edited
void func_ov182_021b782c(BOOL autoAddress);
void func_ov182_021b7840(BOOL autoDns);

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

// An access point that the scan found
struct DWCUtilApEntry {
    u8 ssid[32];
    u8 bssid[6];
    u16 linkLevel;
    // 0 for none, 1 for WEP, 2 for a security that the utility can't set
    u8 security;
};

// The scan for access points: allocates it, starts it (TRUE when it started), and stops it (TRUE once it has stopped)
void func_ov182_021b74d4(void);
BOOL func_ov182_021b751c(void);
BOOL func_ov182_021b75d0(void);
// Stops the scan for access points and frees it, and the access points it found and their count
void func_ov182_021b74f4(void);
int func_ov182_021b7630(DWCUtilApEntry **entries);
// Copies the SSID of the connection being edited, and takes an SSID or a WEP key into it
void func_ov182_021b7a08(u8 *ssid);
void func_ov182_021b7854(const u8 *ssid);
void func_ov182_021b7874(const u8 *key);
// The addresses of the connection being edited (its IP address, subnet mask, gateway and two DNS servers) as 12
// digits, each number right-aligned in three, and back
void func_ov182_021b7a24(u8 *digits);
void func_ov182_021b7a50(u8 *digits);
void func_ov182_021b7a7c(u8 *digits);
void func_ov182_021b7aac(u8 *digits);
void func_ov182_021b7adc(u8 *digits);
void func_ov182_021b7990(const u8 *digits);
void func_ov182_021b79a8(const u8 *digits);
void func_ov182_021b79c0(const u8 *digits);
void func_ov182_021b79d8(const u8 *digits);
void func_ov182_021b79f0(const u8 *digits);
// Converts 12 digits to an address
void func_ov182_021b7ff0(const u8 *digits, u8 *address);
// Makes the connection of the console's Wi-Fi settings the one being edited
void func_ov182_021b7b20(int index);

// A copy of a screen of BG 2 of the main screen, uploaded at the next VBlank once marked
void func_ov182_021b6f38(const void *screen);
void func_ov182_021b6f78(void);
void func_ov182_021b6f98(void);
// Copies rows of a screen of 32 entries a row into that copy, from an offset in the copy on, and loads a palette as the
// main screen's BG palettes at the next VBlank
void func_ov182_021b6fac(const u16 *src, int offset, int width, int rows);
void func_ov182_021b7060(const void *palette);

// The scroll bar of a list: its handle (kind 1 or 2, none for 0) travels range pixels down from x, y. Its event is 1
// to 3 when the handle is grabbed, dragged and let go, 4 and 6 when an arrow is pressed and 5 and 7 when it is let go
void func_ov182_021b70a0(int kind, int range, int x, int y, int pos);
void func_ov182_021b711c(void);
u8 func_ov182_021b7144(void);
u8 func_ov182_021b7150(void);
void func_ov182_021b715c(int pos);
// Unlocks and locks the scroll bar
void func_ov182_021b7164(void);
void func_ov182_021b7174(void);

// The first screen of the connection settings
void func_ov182_021b536c(void);
// The choice of the setup of a connection
void func_ov182_021b5bb8(void);
// The message before the connection test
void func_ov182_021b6588(void);
// The connection test with auto-connect (its file is not written yet)
void func_ov182_021b6104(void);

// The whole touch screen, a rectangle as in dwc_util_display.c: .rodata between dwc_util_text.c's and
// dwc_util_task.c's that no file claims yet
extern const u16 data_ov182_021c059c[4];

#endif // POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H
