#ifndef POKEBW2_APP_UI_TOUCHBAR_H
#define POKEBW2_APP_UI_TOUCHBAR_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

// touchbar.c: The bar of icons at the bottom of the lower screen, such as the return button
typedef struct TouchBar TouchBar;
typedef struct TouchBarIcon TouchBarIcon;

// An icon of the bar: one of the bar's own, or from TOUCHBAR_ICON_CUSTOM on, one drawn from the caller's
// resources and animations. Pressing the key does what touching it does
#define TOUCHBAR_ICON_CUSTOM 7

typedef struct {
    s32 icon;
    ClActorPos pos;
    u16 charRes;
    u16 plttRes;
    u16 cellRes;
    // The animations of the icon at rest, inactive, pressed and, for an icon that toggles, toggled
    u16 anims[4];
    u16 unk16;
    u32 key;
    u32 se;
} TouchBarItem;

typedef struct {
    TouchBarItem *items;
    u32 count;
    ClActUnit *unit;
    u32 bg;
    u32 bgPalette;
    u32 objPalette;
    u32 vramType;
    BOOL unk1C;
} TouchBarSetup;

// An icon of the bar: its sprite, its state and what to do when it is touched, which depends on its kind
struct TouchBarIcon {
    ClActor *actor;
    BOOL active;
    u32 anim;
    void (*onTouch)(TouchBarIcon *icon);
    void (*setActive)(TouchBarIcon *icon, BOOL active);
    u32 kind;
    TouchBarItem item;
};

// The resources the bar loads: its icons' palette, characters, and cells and animations, and its BG
typedef struct {
    u32 res[3];
    u32 bg;
    u32 bgChars;
} TouchBarGfx;

struct TouchBar {
    s32 selected;
    u32 count;
    u32 state;
    TouchBarGfx gfx;
    TouchBarIcon icons[];
};

TouchBar *TouchBar_Create(const TouchBarSetup *setup, HeapID heapId);
void TouchBar_Free(TouchBar *bar);
void TouchBar_Main(TouchBar *bar);
// The icon whose touch animation has ended, or -1 when none
u32 TouchBar_GetDecided(TouchBar *bar);
// The icon being touched, or -1 when none
u32 TouchBar_GetTouched(TouchBar *bar);
// Whether an icon is being touched
BOOL TouchBar_IsTouching(TouchBar *bar);
void TouchBar_SetActive(TouchBar *bar, BOOL active);
void TouchBar_SetVisible(TouchBar *bar, BOOL visible);
void TouchBar_SetBgPriority(TouchBar *bar, u8 bgPriority);
void TouchBar_SetPriority(TouchBar *bar, u8 priority);
void TouchBar_SetIconActive(TouchBar *bar, u32 icon, BOOL a2);
void TouchBar_SetIconVisible(TouchBar *bar, u32 icon, BOOL a2);
void TouchBar_SetIconSe(TouchBar *bar, u32 icon, u32 se);
void TouchBar_SetIconKey(TouchBar *bar, u32 icon, u32 key);
void TouchBar_SetIconBgPriority(TouchBar *bar, u32 icon, u8 bgPriority);
void TouchBar_SetIconPressed(TouchBar *bar, u32 icon, BOOL pressed);
BOOL TouchBar_IsIconPressed(TouchBar *bar, u32 icon);
void TouchBar_Reset(TouchBar *bar, u32 a1);

#endif // POKEBW2_APP_UI_TOUCHBAR_H
