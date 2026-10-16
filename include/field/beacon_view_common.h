#ifndef POKEBW2_FIELD_BEACON_VIEW_COMMON_H
#define POKEBW2_FIELD_BEACON_VIEW_COMMON_H

#include "types.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "struct_decls.h"
#include "system/app_taskmenu.h"
#include "system/bmp_oam.h"
#include "system/printsys.h"

// What the beacon view's touch screens share, overlay 83, named after the ROM's "beacon_view_common.c": a row of up to
// three buttons, text printed into a window or a bitmap OAM through the print queue, and the sub screen's BG sliding
// in and out. Overlays 84 and 85, the beacon view, call these. swan has no names for this overlay; the names are ours

// A row of two or three buttons at the bottom of the screen, of one of three layouts. Their labels come from system
// message file 12
#define BEACON_VIEW_MENU_BUTTON_MAX 3
#define BEACON_VIEW_MENU_STR_MAX 4

typedef struct {
    AppTaskMenuWin *win;
    AppTaskMenuItem item;
} BeaconViewMenuButton;

typedef struct {
    HeapID heapId;
    AppTaskMenuRes *res;
    MsgData *msgData;
    u8 type;
    u8 buttonCount;
    StrBuf *strs[BEACON_VIEW_MENU_STR_MAX];
    BeaconViewMenuButton buttons[BEACON_VIEW_MENU_BUTTON_MAX];
} BeaconViewMenu;

// A window whose text goes through the print queue. The window, the font, the queue and the task manager belong to
// the caller
typedef struct {
    BmpWin *window;
    GFLBitmap *bitmap;
    Font *font;
    PrintQueue *queue;
    TCBExManager *tcbManager;
    PrintWindow printWindow;
} BeaconViewWin;

// A bitmap shown as OAM, whose text goes through the print queue
typedef struct {
    BmpOamActor *actor;
    GFLBitmap *bitmap;
    Font *font;
    PrintQueue *queue;
    TCBExManager *tcbManager;
} BeaconViewOam;

void BeaconView_PlaySE(u32 se);
void BeaconView_SetActorPos(ClActor *actor, s16 x, s16 y);
// Sets the actor's animation sequence and starts it over
void BeaconView_SetActorAnim(ClActor *actor, u16 sequence);
// How long a frame may spend printing: 2000 or 500 OS_GetTick() ticks
void BeaconView_SetPrintTimeLimit(PrintQueue *queue, BOOL longer);

BeaconViewMenu *BeaconViewMenu_Create(u8 bg, u8 palette, Font *font, PrintQueue *queue, HeapID heapId);
void BeaconViewMenu_Free(BeaconViewMenu *menu);
// Puts up the buttons of one of the three layouts
void BeaconViewMenu_Open(BeaconViewMenu *menu, u8 type);
AppTaskMenuWin *BeaconViewMenu_GetButton(BeaconViewMenu *menu, int index);
// The button touched this frame, or -1. The last button flashes when touched; the others only play the sound
int BeaconViewMenu_Update(BeaconViewMenu *menu);
// Updates a flashing button; once it has finished, frees every button and returns TRUE
BOOL BeaconViewMenu_WaitFlash(BeaconViewMenu *menu, int index);
void BeaconViewMenu_Close(BeaconViewMenu *menu);

// Counts *timer down, and returns TRUE once it runs out or a drawn pixel of the BG is touched
BOOL BeaconView_WaitTimerOrTouch(u8 *timer, u8 bg);
// 22 for a beacon of a type of 60 or more (the Funfest missions'), otherwise its type plus 21
u32 func_ov083_021eab38(const GameBeacon *beacon);

void BeaconViewOam_Init(BeaconViewOam *oam, BmpOamActorSetup *setup, BmpOamSys *sys, PrintQueue *queue, Font *font,
                        TCBExManager *tcbManager);
void BeaconViewOam_Free(BeaconViewOam *oam);

// Prints a string into the window, at once if `now` and otherwise through the print queue, with a task that copies
// the window to VRAM once it is printed. The window is first filled with `fill` if `clear`. *pending, if given,
// counts the tasks still running
void BeaconViewWin_PrintEx(BeaconViewWin *win, const StrBuf *strbuf, s16 x, s16 y, u8 fill, u16 color, int *pending,
                           BOOL now, BOOL clear);
void BeaconViewWin_Print(BeaconViewWin *win, const StrBuf *strbuf, s16 x, s16 y, u8 fill, u16 color, int *pending,
                         BOOL now);
// BeaconViewWin_Print for a bitmap OAM, at its top left corner over color 0
void BeaconViewOam_Print(BeaconViewOam *oam, const StrBuf *strbuf, u16 color, int *pending, BOOL now);

// Scrolls the BG's y from 0 to 64 over four frames, or back from 64 to 0 if `back`. *pending counts the tasks still
// running
void BeaconView_StartBGSlide(TCBExManager *tcbManager, u8 bg, u8 back, int *pending);

#endif // POKEBW2_FIELD_BEACON_VIEW_COMMON_H
