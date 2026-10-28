#ifndef POKEBW2_APP_POKEDEX_LIST_MAIN_H
#define POKEBW2_APP_POKEDEX_LIST_MAIN_H

#include "types.h"
#include "app/pokedex_list.h"
#include "app/ui/frame_list.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "struct_decls.h"
#include "system/printsys.h"

// pokedex_list_main.c: the Pokédex's list: its work, its BGs and graphics, its fades, the frame of the touch bar that
// slides in and the list of the species. The ROM doesn't name this file; pokedex_list_main.c is a guess after the
// search screen's pokedex_search_main.c. The names are ours

// Not from swan: the list's heap
#define HEAPID_POKEDEX_LIST 0x6b

// How many species the list can hold, and the rows it shows
#define POKEDEX_LIST_SPECIES_COUNT 649
#define POKEDEX_LIST_ROW_COUNT 7

// The icons of the rows that a screen can show while the list scrolls
#define POKEDEX_LIST_ICON_COUNT 16

// A list item's value: the species, and above it whether it is seen or caught
#define POKEDEX_LIST_ITEM_SPECIES_MASK 0x0fffffff
#define POKEDEX_LIST_ITEM_STATE_MASK 0xf0000000
#define POKEDEX_LIST_ITEM_STATE_SHIFT 28
#define POKEDEX_LIST_ITEM_SEEN (1 << POKEDEX_LIST_ITEM_STATE_SHIFT)
#define POKEDEX_LIST_ITEM_CAUGHT (2 << POKEDEX_LIST_ITEM_STATE_SHIFT)

// The actors
enum {
    POKEDEX_LIST_OBJ_SCROLL_BAR,
    POKEDEX_LIST_OBJ_SCROLL_RAIL,
    // Shown with the picture of a species
    POKEDEX_LIST_OBJ_MON_BASE,
    // The buttons of the Pokédexes; without the National Pokédex the second is Unova's
    POKEDEX_LIST_OBJ_UNOVA,
    POKEDEX_LIST_OBJ_NATIONAL,
    POKEDEX_LIST_OBJ_HABITAT,
    POKEDEX_LIST_OBJ_RETURN,
    POKEDEX_LIST_OBJ_EXIT,
    POKEDEX_LIST_OBJ_SHORTCUT,
    POKEDEX_LIST_OBJ_PAGE_DOWN,
    POKEDEX_LIST_OBJ_PAGE_UP,
    POKEDEX_LIST_OBJ_SELECT,
    POKEDEX_LIST_OBJ_START,
    // The icons of the rows: on the touch screen, on the top screen, and the windows of the top screen's
    POKEDEX_LIST_OBJ_ICON,
    POKEDEX_LIST_OBJ_ICON_SUB = POKEDEX_LIST_OBJ_ICON + POKEDEX_LIST_ICON_COUNT,
    POKEDEX_LIST_OBJ_ICON_WINDOW = POKEDEX_LIST_OBJ_ICON_SUB + POKEDEX_LIST_ICON_COUNT,
    // The picture of the species under the cursor: two, so that one loads while the other shows
    POKEDEX_LIST_OBJ_MON = POKEDEX_LIST_OBJ_ICON_WINDOW + POKEDEX_LIST_ICON_COUNT,
    POKEDEX_LIST_OBJ_COUNT = POKEDEX_LIST_OBJ_MON + 2,
};

#define POKEDEX_LIST_CHAR_RES_COUNT 36
#define POKEDEX_LIST_PALETTE_RES_COUNT 6
#define POKEDEX_LIST_CELL_RES_COUNT 6

// The windows
enum {
    POKEDEX_LIST_WIN_TITLE,
    POKEDEX_LIST_WIN_SEEN,
    POKEDEX_LIST_WIN_CAUGHT,
    POKEDEX_LIST_WIN_UNOVA,
    POKEDEX_LIST_WIN_NATIONAL,
    POKEDEX_LIST_WIN_HABITAT,
    POKEDEX_LIST_WIN_COUNT,
};

// The work, 0xc70 bytes
typedef struct {
    PokedexListParam *param;
    TCB *vblankTask;
    TCB *hblankTask;
    PaletteFade *paletteFade;
    int seq;
    int subSeq;
    // The sequences after a button's animation, and after a fade
    int buttonNextSeq;
    int fadeNextSeq;
    PrintWindow windows[POKEDEX_LIST_WIN_COUNT];
    Font *font;
    MsgData *msgData;
    WordSet *wordSet;
    StrBuf *strbuf;
    PrintQueue *printQueue;
    // The names of the list's items
    StrBuf *names[POKEDEX_LIST_SPECIES_COUNT];
    ClActUnit *actorUnit;
    ClActor *actors[POKEDEX_LIST_OBJ_COUNT];
    u32 charRes[POKEDEX_LIST_CHAR_RES_COUNT];
    u32 paletteRes[POKEDEX_LIST_PALETTE_RES_COUNT];
    u32 cellRes[POKEDEX_LIST_CELL_RES_COUNT];
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
} PokedexListWork;

void PokedexListMain_InitVBlank(PokedexListWork *wk);
void PokedexListMain_ExitVBlank(PokedexListWork *wk);
void PokedexListMain_InitHBlank(PokedexListWork *wk);
void PokedexListMain_ExitHBlank(PokedexListWork *wk);
void PokedexListMain_InitVRAM(void);
const BGSysVRAMConfig *PokedexListMain_GetVRAMConfig(void);
void PokedexListMain_InitBG(void);
void PokedexListMain_ExitBG(void);
void PokedexListMain_LoadGraphics(void);
void PokedexListMain_InitPaletteFade(PokedexListWork *wk);
void PokedexListMain_ExitPaletteFade(PokedexListWork *wk);
// Fades everything but the touch bar and its buttons, which slide instead
void PokedexListMain_StartFade(PokedexListWork *wk, u8 start, u8 end);
void PokedexListMain_SetBlend(void);
void PokedexListMain_InitWindow(void);
void PokedexListMain_ExitWindow(void);
void PokedexListMain_InitMsg(PokedexListWork *wk);
void PokedexListMain_ExitMsg(PokedexListWork *wk);
void PokedexListMain_LoadRegionalNumbers(PokedexListWork *wk);
void PokedexListMain_FreeRegionalNumbers(PokedexListWork *wk);
// The touch bars of both screens and the buttons, off the screens at first, slide by speed pixels a frame
void PokedexListMain_InitFramePos(PokedexListWork *wk);
void PokedexListMain_StartFrameMove(PokedexListWork *wk, s16 speed);
BOOL PokedexListMain_UpdateFrameMove(PokedexListWork *wk);
// The list of the species, with the cursor on the species that the Pokédex was last on
void PokedexListMain_InitList(PokedexListWork *wk);
void PokedexListMain_ExitList(PokedexListWork *wk);

#endif // POKEBW2_APP_POKEDEX_LIST_MAIN_H
