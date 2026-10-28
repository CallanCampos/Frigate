#ifndef POKEBW2_APP_POKEDEX_AREA_LIST_VIEW_H
#define POKEBW2_APP_POKEDEX_AREA_LIST_VIEW_H

#include "types.h"
#include "app/pokedex_common.h"
#include "app/ui/frame_list.h"
#include "app/ui/ui_scene.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/str.h"
#include "struct_decls.h"
#include "system/blink_palanm.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/printsys.h"

// pokedex_area_list_view.c: what the Pokédex's list of areas (overlay 304) shows: its BGs, actors and windows, the
// list of the areas and the marks of its rows. The ROM doesn't name this file, and the names are ours

// Not from swan: the screen's heap
#define HEAPID_POKEDEX_AREA_LIST 0xa6

#define POKEDEX_AREA_LIST_ROWS 7
// The rows the list draws: those shown, and one before and one after them
#define POKEDEX_AREA_LIST_DRAWN_ROWS (POKEDEX_AREA_LIST_ROWS + 2)
// The kinds of habitat an area has marks for
#define POKEDEX_AREA_LIST_KINDS 3
#define POKEDEX_AREA_LIST_MARK_COUNT (POKEDEX_AREA_LIST_DRAWN_ROWS * POKEDEX_AREA_LIST_KINDS)
#define POKEDEX_AREA_LIST_NAME_MAX 100

// The actors
enum {
    // The marks of the rows on the lower screen, those on the upper screen, and the OBJ windows of those
    POKEDEX_AREA_LIST_OBJ_MARK,
    POKEDEX_AREA_LIST_OBJ_MARK_SUB = 28,
    POKEDEX_AREA_LIST_OBJ_MARK_WINDOW = 56,
    // The button at the upper left that opens the area under the cursor
    POKEDEX_AREA_LIST_OBJ_DECIDE = 84,
    // The buttons of the Pokédex's lists: the upper one is shown once the national Pokédex is obtained
    POKEDEX_AREA_LIST_OBJ_MODE_UPPER,
    POKEDEX_AREA_LIST_OBJ_MODE_LOWER,
    POKEDEX_AREA_LIST_OBJ_MODE_HABITAT,
    POKEDEX_AREA_LIST_OBJ_SCROLL_BAR,
    POKEDEX_AREA_LIST_OBJ_SCROLL_RAIL,
    // The frame around the marks of the cursor's row
    POKEDEX_AREA_LIST_OBJ_MARK_FRAME,
    POKEDEX_AREA_LIST_OBJ_MODE,
    POKEDEX_AREA_LIST_OBJ_PAGE_UP,
    POKEDEX_AREA_LIST_OBJ_PAGE_DOWN,
    POKEDEX_AREA_LIST_OBJ_SHORTCUT,
    POKEDEX_AREA_LIST_OBJ_RETURN,
    POKEDEX_AREA_LIST_OBJ_EXIT,
    POKEDEX_AREA_LIST_OBJ_COUNT,
};

// The resources of the actors
enum {
    POKEDEX_AREA_LIST_RES_MAIN,
    POKEDEX_AREA_LIST_RES_SUB,
    POKEDEX_AREA_LIST_RES_TOUCHBAR,
    POKEDEX_AREA_LIST_RES_COUNT,
};

// The windows, each the whole of a screen
enum {
    POKEDEX_AREA_LIST_WIN_MAIN,
    POKEDEX_AREA_LIST_WIN_SUB,
    POKEDEX_AREA_LIST_WIN_COUNT,
};

// What a mark's actor is set to once the screen updates, 0x10 bytes
typedef struct {
    BOOL visible;
    BOOL changed;
    u16 anim;
    ClActorPos pos;
} PokedexAreaListMark;

// 0x6f4 bytes
typedef struct {
    u32 unk0;
    PaletteFade *paletteFade;
    BlinkPalAnm *blinkAnm;
    BOOL blinkPaused;
    ClActUnit *actorUnit;
    UIObjRes res[POKEDEX_AREA_LIST_RES_COUNT];
    ClActor *actors[POKEDEX_AREA_LIST_OBJ_COUNT];
    // Whether the windows' text has been sent to VRAM
    BOOL printed;
    Font *font;
    PrintWindow printWindows[POKEDEX_AREA_LIST_WIN_COUNT];
    PrintQueue *printQueue;
    BmpWin *windows[POKEDEX_AREA_LIST_WIN_COUNT];
    u32 scrollTimer;
    u8 count;
    u32 shownRows;
    FrameList *list;
    StrBuf *names[POKEDEX_AREA_LIST_NAME_MAX];
    PokedexHabitatSummary *areas;
    // What the marks of the rows drawn are set to, on the lower screen and on the upper one
    PokedexAreaListMark marks[POKEDEX_AREA_LIST_DRAWN_ROWS][POKEDEX_AREA_LIST_KINDS];
    PokedexAreaListMark subMarks[POKEDEX_AREA_LIST_DRAWN_ROWS][POKEDEX_AREA_LIST_KINDS];
    // FALSE while the list does nothing
    BOOL listBusy;
} PokedexAreaListView;

void PokedexAreaListView_Init(PokedexAreaListView *view, GameData *gameData);
// Draws the list, and returns TRUE once it is drawn
BOOL PokedexAreaListView_Draw(PokedexAreaListView *view);
void PokedexAreaListView_Exit(PokedexAreaListView *view);
void PokedexAreaListView_VBlank(PokedexAreaListView *view);
// The blend of the upper screen's list changes down the screen, by the height of the list's rows
void PokedexAreaListView_HBlank(PokedexAreaListView *view);
void PokedexAreaListView_Main(PokedexAreaListView *view);
// Runs the list, and returns what FrameList_Main does
u32 PokedexAreaListView_ListMain(PokedexAreaListView *view);
// Fades the palettes of both screens from start to end of 16 toward black
void PokedexAreaListView_StartFade(PokedexAreaListView *view, u8 start, u8 end);
u32 PokedexAreaListView_IsFading(PokedexAreaListView *view);
BOOL PokedexAreaListView_IsPrinted(PokedexAreaListView *view);
// Prints a string into a POKEDEX_AREA_LIST_WIN_* after clearing the rectangle given
void PokedexAreaListView_Print(u32 window, StrBuf *strbuf, PokedexAreaListView *view, s16 x, s16 y, u16 width,
                               u16 height, u16 color);
void PokedexAreaListView_FillWindow(u32 window, u8 color, PokedexAreaListView *view);
ClActor *PokedexAreaListView_GetActor(PokedexAreaListView *view, u32 index);
// Creates the list of the areas, with the cursor on the area of a zone if it has it
void PokedexAreaListView_InitList(PokedexAreaListView *view, u32 count, PokedexHabitatSummary *areas, u16 zone,
                                  u16 cursorPos);
void PokedexAreaListView_ExitList(PokedexAreaListView *view);
// Hides the marks that the list did not draw again after a jump
void PokedexAreaListView_HideUndrawnMarks(PokedexAreaListView *view);
int PokedexAreaListView_GetListSelected(PokedexAreaListView *view);
s16 PokedexAreaListView_GetListCursor(PokedexAreaListView *view);
int PokedexAreaListView_GetListScroll(PokedexAreaListView *view);
void PokedexAreaListView_SetListCursor(PokedexAreaListView *view, int pos);
// Sets the palette of the cursor's row
void PokedexAreaListView_SetCursorPalette(PokedexAreaListView *view, u32 palette);
PokedexHabitatSummary *PokedexAreaListView_GetArea(PokedexAreaListView *view, u32 index);
// Starts an animation of an actor
void PokedexAreaListView_SetActorAnim(PokedexAreaListView *view, u32 index, u32 anim);
// Moves the touch bar and its buttons up by a number of pixels, or else the upper screen's heading
void PokedexAreaListView_Slide(PokedexAreaListView *view, int offset, BOOL touchbar);
void PokedexAreaListView_SetBlinkPaused(PokedexAreaListView *view, BOOL paused);

#endif // POKEBW2_APP_POKEDEX_AREA_LIST_VIEW_H
