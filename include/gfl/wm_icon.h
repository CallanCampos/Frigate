#ifndef POKEBW2_GFL_WM_ICON_H
#define POKEBW2_GFL_WM_ICON_H

#include "types.h"
#include "gfl/heap.h"

// The wireless signal icon in a corner of a screen, drawn with the first two OBJ of OAM

// Which screen the icon is on
enum {
    WM_ICON_SCREEN_TOP = 1,
    WM_ICON_SCREEN_BOTTOM,
};

// Shows the icon, or shows it again with new graphics when it is already shown
void func_0203e76c(u32 x, u32 y, u32 type, HeapID heapId);
void func_0203e7dc(void);
// The signal strength, 0 to 3
void func_0203e7f8(int level);
void func_0203e810(BOOL top, HeapID heapId);
// Writes the icon to OAM, each frame
void func_0203e838(void);
void func_0203e84c(void);
void func_0203e860(void);
// Puts the icon at x and y in place of where func_0203e76c says, unless x is 0
void func_0203e874(u16 x, u16 y);
// The 2D engine that draws the icon, 1 for the main engine and 2 for the sub engine
u32 func_0203e8b0(void);

// The game's archive of the icon's graphics, and its files: two sets of characters and the palette
typedef struct {
    u32 chars[2];
    u32 pltt;
} WmIconFiles;

u32 getWifiIconNarcIdxNum(void);
void func_020116b0(WmIconFiles *files);

#endif // POKEBW2_GFL_WM_ICON_H
