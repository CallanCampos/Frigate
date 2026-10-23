#ifndef POKEBW2_SYSTEM_WIPE_H
#define POKEBW2_SYSTEM_WIPE_H

#include "types.h"
#include "struct_decls.h"

// GFL_WipeSet, GFL_WipeIsFinished, killBrightnessEitherEngine, setBrightnessForEngine and wipe.c's WIPE_FUNCTIONS
// are swan's names (https://github.com/ds-pokemon-hacking/swan, GPL-3.0); the others are ours

// Screen wipes (wipe.c): a pattern that covers or uncovers each screen over a number of steps, in black or white

// Which screens wipe, and in which order
enum {
    WIPE_MODE_BOTH,
    WIPE_MODE_MAIN_FIRST,
    WIPE_MODE_SUB_FIRST,
    WIPE_MODE_MAIN,
    WIPE_MODE_SUB,
};

// The patterns: each even one covers the screen and the odd one after it uncovers it
enum {
    WIPE_TYPE_FADE_OUT,
    WIPE_TYPE_FADE_IN,
};

// The colors a wipe covers with; WIPE_COLOR_LAST is the color the last wipe covered with
#define WIPE_COLOR_BLACK 0
#define WIPE_COLOR_WHITE 0x7fff
#define WIPE_COLOR_LAST 0xffff

// A wipe of one screen
struct WipeScreen {
    u32 type;
    s32 division;
    s32 sync;
    s32 seq;
    s32 screen;
    void *work;
    WipeWnd *wnd;
    WipeHBlank *hblank;
    u32 heapId;
    u16 color;
    // Whether the wipe leaves the screen covered
    BOOL endCovered;
    // Whether the wipe is a master brightness fade
    BOOL useBrightness;
};

typedef void (*WipeHBlankFunc)(void *work);

// The H-blank functions of the wipes of both screens
struct WipeHBlank {
    void *work[2];
    WipeHBlankFunc func[2];
    BOOL active[2];
};

// Starts a wipe of the screens mode picks, with the pattern typeMain or typeSub over division steps of sync frames
void GFL_WipeSet(int mode, int typeMain, int typeSub, u16 color, s32 division, s32 sync, u32 heapId);
// Runs a frame of the wipe
void GFL_WipeMain(void);
BOOL GFL_WipeIsFinished(void);
// Ends the wipe at once
void GFL_WipeForceEnd(void);
void Wipe_HideWindows(int screen);
void killBrightnessEitherEngine(int screen);
// Covers a screen or both with the master brightness, white for WIPE_COLOR_WHITE and black otherwise
void Wipe_SetScreenCovered(int screen, u16 color);
void Wipe_SetCovered(u16 color);
void Wipe_SetBackdropColor(u16 color);
void setBrightnessForEngine(int screen, int brightness);
// Adds or removes a wipe's H-blank function at the next VBlank
void WipeHBlank_AddAtVBlank(WipeHBlank *hblank, void *work, WipeHBlankFunc func, int slot, u32 heapId);
void WipeHBlank_RemoveAtVBlank(WipeHBlank *hblank, int slot, u32 heapId);

#endif // POKEBW2_SYSTEM_WIPE_H
