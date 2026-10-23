#ifndef POKEBW2_APP_ZKNLIST_MAIN_H
#define POKEBW2_APP_ZKNLIST_MAIN_H

#include "types.h"
#include "app/ui/frame_list.h"
#include "app/zukanlist.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "struct_decls.h"
#include "system/printsys.h"

// zknlist_main.c: the Pokédex's list: its work, its BGs and graphics, its fades, the frame of the touch bar that
// slides in and the list of the species. The ROM doesn't name this file; zknlist_main.c is a guess after the search
// screen's zknsearch_main.c. The names are ours

// Not from swan: the list's heap
#define HEAPID_ZUKAN_LIST 0x6b

// How many species the list can hold, and the rows it shows
#define ZKNLIST_SPECIES_COUNT 649
#define ZKNLIST_ROW_COUNT 7

// The icons of the rows that a screen can show while the list scrolls
#define ZKNLIST_ICON_COUNT 16

// A list item's value: the species, and above it whether it is seen or caught
#define ZKNLIST_ITEM_SPECIES_MASK 0x0fffffff
#define ZKNLIST_ITEM_STATE_MASK 0xf0000000
#define ZKNLIST_ITEM_STATE_SHIFT 28
#define ZKNLIST_ITEM_SEEN (1 << ZKNLIST_ITEM_STATE_SHIFT)
#define ZKNLIST_ITEM_CAUGHT (2 << ZKNLIST_ITEM_STATE_SHIFT)

// The actors
enum {
    ZKNLIST_OBJ_SCROLL_BAR,
    ZKNLIST_OBJ_SCROLL_RAIL,
    // Shown with the picture of a species
    ZKNLIST_OBJ_MON_BASE,
    // The buttons of the Pokédexes; without the National Pokédex the second is Unova's
    ZKNLIST_OBJ_UNOVA,
    ZKNLIST_OBJ_NATIONAL,
    ZKNLIST_OBJ_HABITAT,
    ZKNLIST_OBJ_RETURN,
    ZKNLIST_OBJ_EXIT,
    ZKNLIST_OBJ_SHORTCUT,
    ZKNLIST_OBJ_PAGE_DOWN,
    ZKNLIST_OBJ_PAGE_UP,
    ZKNLIST_OBJ_SELECT,
    ZKNLIST_OBJ_START,
    // The icons of the rows: on the touch screen, on the top screen, and the windows of the top screen's
    ZKNLIST_OBJ_ICON,
    ZKNLIST_OBJ_ICON_SUB = ZKNLIST_OBJ_ICON + ZKNLIST_ICON_COUNT,
    ZKNLIST_OBJ_ICON_WINDOW = ZKNLIST_OBJ_ICON_SUB + ZKNLIST_ICON_COUNT,
    // The picture of the species under the cursor: two, so that one loads while the other shows
    ZKNLIST_OBJ_MON = ZKNLIST_OBJ_ICON_WINDOW + ZKNLIST_ICON_COUNT,
    ZKNLIST_OBJ_COUNT = ZKNLIST_OBJ_MON + 2,
};

#define ZKNLIST_CHAR_RES_COUNT 36
#define ZKNLIST_PALETTE_RES_COUNT 6
#define ZKNLIST_CELL_RES_COUNT 6

// The windows
enum {
    ZKNLIST_WIN_TITLE,
    ZKNLIST_WIN_SEEN,
    ZKNLIST_WIN_CAUGHT,
    ZKNLIST_WIN_UNOVA,
    ZKNLIST_WIN_NATIONAL,
    ZKNLIST_WIN_HABITAT,
    ZKNLIST_WIN_COUNT,
};

// The work, 0xc70 bytes
typedef struct {
    ZukanListParam *param;
    TCB *vblankTask;
    TCB *hblankTask;
    PaletteFade *paletteFade;
    int seq;
    int subSeq;
    // The sequences after a button's animation, and after a fade
    int buttonNextSeq;
    int fadeNextSeq;
    PrintWindow windows[ZKNLIST_WIN_COUNT];
    Font *font;
    MsgData *msgData;
    WordSet *wordSet;
    StrBuf *strbuf;
    PrintQueue *printQueue;
    // The names of the list's items
    StrBuf *names[ZKNLIST_SPECIES_COUNT];
    ClActUnit *actorUnit;
    ClActor *actors[ZKNLIST_OBJ_COUNT];
    u32 charRes[ZKNLIST_CHAR_RES_COUNT];
    u32 paletteRes[ZKNLIST_PALETTE_RES_COUNT];
    u32 cellRes[ZKNLIST_CELL_RES_COUNT];
    // Which of the two pictures the next species loads into
    u16 monIndex : 1;
    // The actor whose animation the sequence waits for
    u16 buttonId : 15;
    // The flashing of the chosen row
    u8 decideSeq;
    u8 decideTimer;
    // The icons shown on each screen, a bit each
    u16 shownIcons;
    u16 shownIconsSub;
    FrameList *frameList;
    int initSeq;
    u32 listRows;
    // The number of each species in the regional Pokédex
    u16 *regionalNumbers;
    int scrollTimer;
    // The touch bar's slide: the pixels left and the pixels a frame
    u16 frameMoveLeft;
    s16 frameMoveSpeed;
    BlinkPalAnm *blinkAnm;
    BOOL blinkStopped;
} ZknListWork;

void ZknListMain_InitVBlank(ZknListWork *wk);
void ZknListMain_ExitVBlank(ZknListWork *wk);
void ZknListMain_InitHBlank(ZknListWork *wk);
void ZknListMain_ExitHBlank(ZknListWork *wk);
void ZknListMain_InitVRAM(void);
const BGSysVRAMConfig *ZknListMain_GetVRAMConfig(void);
void ZknListMain_InitBG(void);
void ZknListMain_ExitBG(void);
void ZknListMain_LoadGraphics(void);
void ZknListMain_InitPaletteFade(ZknListWork *wk);
void ZknListMain_ExitPaletteFade(ZknListWork *wk);
// Fades everything but the touch bar and its buttons, which slide instead
void ZknListMain_StartFade(ZknListWork *wk, u8 start, u8 end);
void ZknListMain_SetBlend(void);
void ZknListMain_InitWindow(void);
void ZknListMain_ExitWindow(void);
void ZknListMain_InitMsg(ZknListWork *wk);
void ZknListMain_ExitMsg(ZknListWork *wk);
void ZknListMain_LoadRegionalNumbers(ZknListWork *wk);
void ZknListMain_FreeRegionalNumbers(ZknListWork *wk);
// The touch bars of both screens and the buttons, off the screens at first, slide by speed pixels a frame
void ZknListMain_InitFramePos(ZknListWork *wk);
void ZknListMain_StartFrameMove(ZknListWork *wk, s16 speed);
BOOL ZknListMain_UpdateFrameMove(ZknListWork *wk);
// The list of the species, with the cursor on the species that the Pokédex was last on
void ZknListMain_InitList(ZknListWork *wk);
void ZknListMain_ExitList(ZknListWork *wk);

#endif // POKEBW2_APP_ZKNLIST_MAIN_H
