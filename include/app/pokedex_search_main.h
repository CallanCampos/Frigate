#ifndef POKEBW2_APP_POKEDEX_SEARCH_MAIN_H
#define POKEBW2_APP_POKEDEX_SEARCH_MAIN_H

#include "types.h"
#include "app/pokedex_search.h"
#include "app/pokedex_search_engine.h"
#include "app/ui/frame_list.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "struct_decls.h"
#include "system/printsys.h"

// pokedex_search_main.c: the Pokédex's search screen: its work, its BGs and graphics, its fades and the frame of the
// touch bar that slides in. The names are ours

// Not from swan: the search screen's heap
#define HEAPID_POKEDEX_SEARCH 0x7b

#define POKEDEX_SEARCH_NONE 0xff

#define POKEDEX_SEARCH_SHAPE_COUNT 14
#define POKEDEX_SEARCH_TYPE_COUNT 17

// The actors
enum {
    POKEDEX_SEARCH_OBJ_SCROLL_BAR,
    POKEDEX_SEARCH_OBJ_SCROLL_RAIL,
    POKEDEX_SEARCH_OBJ_RETURN,
    POKEDEX_SEARCH_OBJ_EXIT,
    POKEDEX_SEARCH_OBJ_SHORTCUT,
    POKEDEX_SEARCH_OBJ_PAGE_DOWN,
    POKEDEX_SEARCH_OBJ_PAGE_UP,
    POKEDEX_SEARCH_OBJ_SHAPE,
    POKEDEX_SEARCH_OBJ_LOADING_BAR,
    // The marks of the list's chosen items: two on the first BG's rows, and two on the second's, with their windows
    POKEDEX_SEARCH_OBJ_MARK,
    POKEDEX_SEARCH_OBJ_MARK_SUB = POKEDEX_SEARCH_OBJ_MARK + 2,
    POKEDEX_SEARCH_OBJ_MARK_WINDOW = POKEDEX_SEARCH_OBJ_MARK_SUB + 2,
    // The icons of the shapes, in the same three kinds
    POKEDEX_SEARCH_OBJ_SHAPE_ICON = POKEDEX_SEARCH_OBJ_MARK_WINDOW + 2,
    POKEDEX_SEARCH_OBJ_SHAPE_ICON_SUB = POKEDEX_SEARCH_OBJ_SHAPE_ICON + POKEDEX_SEARCH_SHAPE_COUNT,
    POKEDEX_SEARCH_OBJ_SHAPE_ICON_WINDOW = POKEDEX_SEARCH_OBJ_SHAPE_ICON_SUB + POKEDEX_SEARCH_SHAPE_COUNT,
    POKEDEX_SEARCH_OBJ_COUNT = POKEDEX_SEARCH_OBJ_SHAPE_ICON_WINDOW + POKEDEX_SEARCH_SHAPE_COUNT,
};

#define POKEDEX_SEARCH_CHAR_RES_COUNT 6
#define POKEDEX_SEARCH_PALETTE_RES_COUNT 6
#define POKEDEX_SEARCH_CELL_RES_COUNT 4

// The windows
enum {
    POKEDEX_SEARCH_WIN_MODE,
    POKEDEX_SEARCH_WIN_INFO,
    POKEDEX_SEARCH_WIN_ORDER_LABEL,
    POKEDEX_SEARCH_WIN_INITIAL_LABEL,
    POKEDEX_SEARCH_WIN_TYPE_LABEL,
    POKEDEX_SEARCH_WIN_COLOR_LABEL,
    POKEDEX_SEARCH_WIN_SHAPE_LABEL,
    POKEDEX_SEARCH_WIN_ORDER,
    POKEDEX_SEARCH_WIN_INITIAL,
    POKEDEX_SEARCH_WIN_TYPE,
    POKEDEX_SEARCH_WIN_COLOR,
    POKEDEX_SEARCH_WIN_START,
    // On the top screen while a list is open: the criterion's name and its values
    POKEDEX_SEARCH_WIN_TOP_LABEL,
    POKEDEX_SEARCH_WIN_TOP_VALUE,
    POKEDEX_SEARCH_WIN_TOP_VALUE2,
    POKEDEX_SEARCH_WIN_MESSAGE,
    POKEDEX_SEARCH_WIN_COUNT,
};

// The pages: the main page's is POKEDEX_SEARCH_NONE before the first, else the criterion it came back from
enum {
    POKEDEX_SEARCH_PAGE_ORDER,
    POKEDEX_SEARCH_PAGE_INITIAL,
    POKEDEX_SEARCH_PAGE_TYPE,
    POKEDEX_SEARCH_PAGE_COLOR,
    POKEDEX_SEARCH_PAGE_SHAPE,
};

// What the VBlank task does with the BGs of the lists (bgRequest)
#define POKEDEX_SEARCH_BG_REQUEST_ON 1
#define POKEDEX_SEARCH_BG_REQUEST_OFF 2

#define POKEDEX_SEARCH_ITEM_NAME_COUNT 50

// The work, 0x2d4 bytes
typedef struct {
    PokedexSearchParam *param;
    TCB *vblankTask;
    TCB *hblankTask;
    PaletteFade *paletteFade;
    int seq;
    int subSeq;
    // The sequences after the page's end or the list's scroll, and after a button's animation
    int nextSeq;
    int buttonNextSeq;
    BGWinFrame *bgWinFrame;
    ClActUnit *actorUnit;
    ClActor *actors[POKEDEX_SEARCH_OBJ_COUNT];
    u32 charRes[POKEDEX_SEARCH_CHAR_RES_COUNT];
    u32 paletteRes[POKEDEX_SEARCH_PALETTE_RES_COUNT];
    u32 cellRes[POKEDEX_SEARCH_CELL_RES_COUNT];
    PrintWindow windows[POKEDEX_SEARCH_WIN_COUNT];
    Font *font;
    MsgData *msgData;
    WordSet *wordSet;
    StrBuf *strbuf;
    PrintQueue *printQueue;
    FrameList *frameList;
    StrBuf *itemNames[POKEDEX_SEARCH_ITEM_NAME_COUNT];
    u32 listRows;
    CursorMove *cursorMove;
    BlinkPalAnm *blinkAnm;
    u8 bgRequest;
    u8 page;
    u8 initSeq;
    u8 timer;
    // The screen of the search's progress bar
    u16 *loadingScreen;
    // A button's animation: of the cursor's palette (0) or of an actor (1), which one, and its progress
    u8 buttonIsActor;
    u8 buttonId;
    u8 buttonSeq;
    u8 buttonTimer;
    int scrollTimer;
    // The touch bar's slide: the pixels left and the pixels a frame
    u16 frameMoveLeft;
    s16 frameMoveSpeed;
    PokedexSearchEngine *engine;
    BOOL searchDone;
} PokedexSearchWork;

void PokedexSearchMain_InitVBlank(PokedexSearchWork *wk);
void PokedexSearchMain_ExitVBlank(PokedexSearchWork *wk);
void PokedexSearchMain_InitHBlank(PokedexSearchWork *wk);
void PokedexSearchMain_ExitHBlank(PokedexSearchWork *wk);
void PokedexSearchMain_InitVRAM(void);
const BGSysVRAMConfig *PokedexSearchMain_GetVRAMConfig(void);
void PokedexSearchMain_InitBG(void);
void PokedexSearchMain_ExitBG(void);
// Put the main page's frame over its text, or a list's text over its frame
void PokedexSearchMain_SetMainBGPriority(void);
void PokedexSearchMain_SetListBGPriority(void);
void PokedexSearchMain_LoadGraphics(void);
void PokedexSearchMain_InitPaletteFade(PokedexSearchWork *wk);
void PokedexSearchMain_ExitPaletteFade(PokedexSearchWork *wk);
void PokedexSearchMain_StartFade(PokedexSearchWork *wk, u8 start, u8 end);
void PokedexSearchMain_SetBlend(BOOL enabled);
void PokedexSearchMain_InitWindow(void);
void PokedexSearchMain_ExitWindow(void);
void PokedexSearchMain_InitMsg(PokedexSearchWork *wk);
void PokedexSearchMain_ExitMsg(PokedexSearchWork *wk);
void PokedexSearchMain_InitBlinkAnm(PokedexSearchWork *wk);
void PokedexSearchMain_ExitBlinkAnm(PokedexSearchWork *wk);
void PokedexSearchMain_InitBGWinFrame(PokedexSearchWork *wk);
void PokedexSearchMain_ExitBGWinFrame(PokedexSearchWork *wk);
void PokedexSearchMain_LoadMainScreen(PokedexSearchWork *wk);
void PokedexSearchMain_LoadOrderScreen(PokedexSearchWork *wk);
void PokedexSearchMain_LoadInitialScreen(PokedexSearchWork *wk);
void PokedexSearchMain_LoadTypeScreen(PokedexSearchWork *wk);
void PokedexSearchMain_LoadColorScreen(PokedexSearchWork *wk);
void PokedexSearchMain_LoadShapeScreen(PokedexSearchWork *wk);
void PokedexSearchMain_LoadLoadingScreen(PokedexSearchWork *wk);
void PokedexSearchMain_FreeLoadingScreen(PokedexSearchWork *wk);
// Show and hide the search's progress bar and its message
void PokedexSearchMain_ShowLoading(PokedexSearchWork *wk);
void PokedexSearchMain_HideLoading(PokedexSearchWork *wk);
void PokedexSearchMain_RequestBGOn(PokedexSearchWork *wk);
// Hides the page's BGs and clears their screens
void PokedexSearchMain_ClearPage(PokedexSearchWork *wk);
// The touch bar and its buttons, below the screen at first, slide by speed pixels a frame
void PokedexSearchMain_InitFramePos(PokedexSearchWork *wk);
void PokedexSearchMain_StartFrameMove(PokedexSearchWork *wk, s16 speed);
BOOL PokedexSearchMain_UpdateFrameMove(PokedexSearchWork *wk);
// A type from its position in the list, its position, and the message of the type at a position or of a type
u32 PokedexSearchMain_ListPosToType(PokedexSearchWork *wk, u32 pos);
u32 PokedexSearchMain_TypeToListPos(PokedexSearchWork *wk, u32 type);
u32 PokedexSearchMain_GetListPosTypeMsg(PokedexSearchWork *wk, u32 pos);
u32 PokedexSearchMain_GetTypeMsg(PokedexSearchWork *wk, u32 type);
// The criteria's shape is its position in the list while the screen runs
void PokedexSearchMain_ShapeToListPos(PokedexSearchWork *wk);
void PokedexSearchMain_ListPosToShape(PokedexSearchWork *wk);

#endif // POKEBW2_APP_POKEDEX_SEARCH_MAIN_H
