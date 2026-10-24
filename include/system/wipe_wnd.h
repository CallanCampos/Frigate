#ifndef POKEBW2_SYSTEM_WIPE_WND_H
#define POKEBW2_SYSTEM_WIPE_WND_H

#include "types.h"
#include "nitro/gx.h"
#include "struct_decls.h"

// The windows the screen wipes draw with (wipe_wnd.c, a guessed name): window register writes, made at once or at the
// next VBlank

typedef struct {
    u32 visible;
    u32 screen;
} WipeWndDisp;

typedef struct {
    u32 planes;
    BOOL effect;
    u32 window;
    u32 screen;
} WipeWndIn;

typedef struct {
    u32 planes;
    BOOL effect;
    u32 screen;
} WipeWndOut;

typedef struct {
    s16 x1;
    s16 y1;
    s16 x2;
    s16 y2;
    u32 window;
    u32 screen;
} WipeWndPos;

// The window writes waiting for the VBlank, per screen and window
struct WipeWnd {
    WipeWndDisp disp[2];
    WipeWndIn in[2][2];
    u8 unk50[0x18];
    WipeWndOut out[2];
    WipeWndPos pos[2][2];
};

// Sets which windows a screen shows
void WipeWnd_SetVisible(u32 visible, u32 screen);
// The planes inside a window and outside the windows, and a window's rectangle, at once
void WipeWnd_SetInsidePlane(u32 planes, BOOL effect, u32 window, u32 screen);
void WipeWnd_SetOutsidePlane(u32 planes, BOOL effect, u32 screen);
void WipeWnd_SetPosition(int x1, int y1, int x2, int y2, u32 window, u32 screen);
// The same at the next VBlank
void WipeWnd_SetVisibleAtVBlank(WipeWnd *wnd, u32 visible, u32 screen);
void WipeWnd_SetInsidePlaneAtVBlank(WipeWnd *wnd, u32 planes, BOOL effect, u32 window, u32 screen);
void WipeWnd_SetOutsidePlaneAtVBlank(WipeWnd *wnd, u32 planes, BOOL effect, u32 screen);
void WipeWnd_SetPositionAtVBlank(WipeWnd *wnd, int x1, int y1, int x2, int y2, u32 window, u32 screen);

// The planes inside a window and outside the windows, read back from the window registers. The original link dropped
// both from wipe_wnd.c, since only wipe_sub.c's dead-stripped plane swap called them; wipe_wnd.c has them back as
// dead-stripped reconstructions (config/deadstrip.toml), and tools/build/check_deadstrip.py fails on a call from
// anything that stays
GXWndPlane WipeWnd_GetInsidePlane(u32 window, u32 screen);
GXWndPlane WipeWnd_GetOutsidePlane(u32 screen);

#endif // POKEBW2_SYSTEM_WIPE_WND_H
