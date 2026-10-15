#ifndef POKEBW2_APP_ZUKAN_AREA_LIST_VIEW_H
#define POKEBW2_APP_ZUKAN_AREA_LIST_VIEW_H

#include "types.h"
#include "app/ui/frame_list.h"
#include "app/ui/ui_scene.h"
#include "app/zukan_common.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/str.h"
#include "struct_decls.h"
#include "system/blink_palanm.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/printsys.h"

// zukan_area_list_view.c: what the Pokédex's list of areas (overlay 304) shows: its BGs, actors and windows, the
// list of the areas and the marks of its rows. The ROM doesn't name this file, and the names are ours

// Not from swan: the screen's heap
#define HEAPID_ZUKAN_AREA_LIST 0xa6

#define ZUKAN_AREA_LIST_ROWS 7
// The rows the list draws: those shown, and one before and one after them
#define ZUKAN_AREA_LIST_DRAWN_ROWS (ZUKAN_AREA_LIST_ROWS + 2)
// The kinds of habitat an area has marks for
#define ZUKAN_AREA_LIST_KINDS 3
#define ZUKAN_AREA_LIST_MARK_COUNT (ZUKAN_AREA_LIST_DRAWN_ROWS * ZUKAN_AREA_LIST_KINDS)
#define ZUKAN_AREA_LIST_NAME_MAX 100

// The actors
enum {
    // The marks of the rows on the lower screen, those on the upper screen, and the OBJ windows of those
    ZUKAN_AREA_LIST_OBJ_MARK,
    ZUKAN_AREA_LIST_OBJ_MARK_SUB = 28,
    ZUKAN_AREA_LIST_OBJ_MARK_WINDOW = 56,
    // The button at the upper left that opens the area under the cursor
    ZUKAN_AREA_LIST_OBJ_DECIDE = 84,
    // The buttons of the Pokédex's lists: the upper one is shown once the national Pokédex is obtained
    ZUKAN_AREA_LIST_OBJ_MODE_UPPER,
    ZUKAN_AREA_LIST_OBJ_MODE_LOWER,
    ZUKAN_AREA_LIST_OBJ_MODE_HABITAT,
    ZUKAN_AREA_LIST_OBJ_SCROLL_BAR,
    ZUKAN_AREA_LIST_OBJ_SCROLL_RAIL,
    // The frame around the marks of the cursor's row
    ZUKAN_AREA_LIST_OBJ_MARK_FRAME,
    ZUKAN_AREA_LIST_OBJ_MODE,
    ZUKAN_AREA_LIST_OBJ_PAGE_UP,
    ZUKAN_AREA_LIST_OBJ_PAGE_DOWN,
    ZUKAN_AREA_LIST_OBJ_SHORTCUT,
    ZUKAN_AREA_LIST_OBJ_RETURN,
    ZUKAN_AREA_LIST_OBJ_EXIT,
    ZUKAN_AREA_LIST_OBJ_COUNT,
};

// The resources of the actors
enum {
    ZUKAN_AREA_LIST_RES_MAIN,
    ZUKAN_AREA_LIST_RES_SUB,
    ZUKAN_AREA_LIST_RES_TOUCHBAR,
    ZUKAN_AREA_LIST_RES_COUNT,
};

// The windows, each the whole of a screen
enum {
    ZUKAN_AREA_LIST_WIN_MAIN,
    ZUKAN_AREA_LIST_WIN_SUB,
    ZUKAN_AREA_LIST_WIN_COUNT,
};

// What a mark's actor is set to once the screen updates, 0x10 bytes
typedef struct {
    BOOL visible;
    BOOL changed;
    u16 anim;
    ClActorPos pos;
} ZukanAreaListMark;

// 0x6f4 bytes
typedef struct {
    u32 unk0;
    PaletteFade *paletteFade;
    BlinkPalAnm *blinkAnm;
    BOOL blinkPaused;
    ClActUnit *actorUnit;
    UIObjRes res[ZUKAN_AREA_LIST_RES_COUNT];
    ClActor *actors[ZUKAN_AREA_LIST_OBJ_COUNT];
    // Whether the windows' text has been sent to VRAM
    BOOL printed;
    Font *font;
    PrintWindow printWindows[ZUKAN_AREA_LIST_WIN_COUNT];
    PrintQueue *printQueue;
    BmpWin *windows[ZUKAN_AREA_LIST_WIN_COUNT];
    u32 scrollTimer;
    u8 count;
    u32 shownRows;
    FrameList *list;
    StrBuf *names[ZUKAN_AREA_LIST_NAME_MAX];
    ZukanHabitatSummary *areas;
    // What the marks of the rows drawn are set to, on the lower screen and on the upper one
    ZukanAreaListMark marks[ZUKAN_AREA_LIST_DRAWN_ROWS][ZUKAN_AREA_LIST_KINDS];
    ZukanAreaListMark subMarks[ZUKAN_AREA_LIST_DRAWN_ROWS][ZUKAN_AREA_LIST_KINDS];
    // FALSE while the list does nothing
    BOOL listBusy;
} ZukanAreaListView;

void ZukanAreaListView_Init(ZukanAreaListView *view, GameData *gameData);
// Draws the list, and returns TRUE once it is drawn
BOOL ZukanAreaListView_Draw(ZukanAreaListView *view);
void ZukanAreaListView_Exit(ZukanAreaListView *view);
void ZukanAreaListView_VBlank(ZukanAreaListView *view);
// The blend of the upper screen's list changes down the screen, by the height of the list's rows
void ZukanAreaListView_HBlank(ZukanAreaListView *view);
void ZukanAreaListView_Main(ZukanAreaListView *view);
// Runs the list, and returns what FrameList_Main does
u32 ZukanAreaListView_ListMain(ZukanAreaListView *view);
// Fades the palettes of both screens from start to end of 16 toward black
void ZukanAreaListView_StartFade(ZukanAreaListView *view, u8 start, u8 end);
u32 ZukanAreaListView_IsFading(ZukanAreaListView *view);
BOOL ZukanAreaListView_IsPrinted(ZukanAreaListView *view);
// Prints a string into a ZUKAN_AREA_LIST_WIN_* after clearing the rectangle given
void ZukanAreaListView_Print(u32 window, StrBuf *strbuf, ZukanAreaListView *view, s16 x, s16 y, u16 width, u16 height,
                             u16 color);
void ZukanAreaListView_FillWindow(u32 window, u8 color, ZukanAreaListView *view);
ClActor *ZukanAreaListView_GetActor(ZukanAreaListView *view, u32 index);
// Creates the list of the areas, with the cursor on the area of a zone if it has it
void ZukanAreaListView_InitList(ZukanAreaListView *view, u32 count, ZukanHabitatSummary *areas, u16 zone,
                                u16 cursorPos);
void ZukanAreaListView_ExitList(ZukanAreaListView *view);
// Hides the marks that the list did not draw again after a jump
void ZukanAreaListView_HideUndrawnMarks(ZukanAreaListView *view);
int ZukanAreaListView_GetListSelected(ZukanAreaListView *view);
s16 ZukanAreaListView_GetListCursor(ZukanAreaListView *view);
int ZukanAreaListView_GetListScroll(ZukanAreaListView *view);
void ZukanAreaListView_SetListCursor(ZukanAreaListView *view, int pos);
// Sets the palette of the cursor's row
void ZukanAreaListView_SetCursorPalette(ZukanAreaListView *view, u32 palette);
ZukanHabitatSummary *ZukanAreaListView_GetArea(ZukanAreaListView *view, u32 index);
// Starts an animation of an actor
void ZukanAreaListView_SetActorAnim(ZukanAreaListView *view, u32 index, u32 anim);
// Moves the touch bar and its buttons up by a number of pixels, or else the upper screen's heading
void ZukanAreaListView_Slide(ZukanAreaListView *view, int offset, BOOL touchbar);
void ZukanAreaListView_SetBlinkPaused(ZukanAreaListView *view, BOOL paused);

#endif // POKEBW2_APP_ZUKAN_AREA_LIST_VIEW_H
