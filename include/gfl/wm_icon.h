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
void WmIcon_Create(u32 x, u32 y, u32 type, HeapID heapId);
void WmIcon_Delete(void);
// The signal strength, 0 to 3
void WmIcon_SetStrength(int level);
void WmIcon_SetScreen(BOOL top, HeapID heapId);
// Writes the icon to OAM, each frame
void WmIcon_Update(void);
void WmIcon_Show(void);
void WmIcon_Hide(void);
// Puts the icon at x and y in place of where WmIcon_Create says, unless x is 0
void WmIcon_SetPos(u16 x, u16 y);
// The 2D engine that draws the icon, 1 for the main engine and 2 for the sub engine
u32 WmIcon_GetEngine(void);

// The game's archive of the icon's graphics, and its files: two sets of characters and the palette
typedef struct {
    u32 chars[2];
    u32 pltt;
} WmIconFiles;

u32 getWifiIconNarcIdxNum(void);
void WmIconFiles_Init(WmIconFiles *files);

#endif // POKEBW2_GFL_WM_ICON_H
