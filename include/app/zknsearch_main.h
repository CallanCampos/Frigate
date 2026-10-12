#ifndef POKEBW2_APP_ZKNSEARCH_MAIN_H
#define POKEBW2_APP_ZKNSEARCH_MAIN_H

#include "types.h"
#include "app/ui/frame_list.h"
#include "app/zukan_search_engine.h"
#include "app/zukansearch.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "struct_decls.h"
#include "system/printsys.h"

// zknsearch_main.c: the Pokédex's search screen: its work, its BGs and graphics, its fades and the frame of the
// touch bar that slides in. The names are ours

// Not from swan: the search screen's heap
#define HEAPID_ZUKAN_SEARCH 0x7b

// The shortcut of the search screen that the Y button registers
#define SHORTCUT_POKEDEX_SEARCH 19

#define ZKNSEARCH_NONE 0xff

#define ZKNSEARCH_SHAPE_COUNT 14
#define ZKNSEARCH_TYPE_COUNT 17

// The actors
enum {
    ZKNSEARCH_OBJ_SCROLL_BAR,
    ZKNSEARCH_OBJ_SCROLL_RAIL,
    ZKNSEARCH_OBJ_RETURN,
    ZKNSEARCH_OBJ_EXIT,
    ZKNSEARCH_OBJ_SHORTCUT,
    ZKNSEARCH_OBJ_PAGE_DOWN,
    ZKNSEARCH_OBJ_PAGE_UP,
    ZKNSEARCH_OBJ_SHAPE,
    ZKNSEARCH_OBJ_LOADING_BAR,
    // The marks of the list's chosen items: two on the first BG's rows, and two on the second's, with their windows
    ZKNSEARCH_OBJ_MARK,
    ZKNSEARCH_OBJ_MARK_SUB = ZKNSEARCH_OBJ_MARK + 2,
    ZKNSEARCH_OBJ_MARK_WINDOW = ZKNSEARCH_OBJ_MARK_SUB + 2,
    // The icons of the shapes, in the same three kinds
    ZKNSEARCH_OBJ_SHAPE_ICON = ZKNSEARCH_OBJ_MARK_WINDOW + 2,
    ZKNSEARCH_OBJ_SHAPE_ICON_SUB = ZKNSEARCH_OBJ_SHAPE_ICON + ZKNSEARCH_SHAPE_COUNT,
    ZKNSEARCH_OBJ_SHAPE_ICON_WINDOW = ZKNSEARCH_OBJ_SHAPE_ICON_SUB + ZKNSEARCH_SHAPE_COUNT,
    ZKNSEARCH_OBJ_COUNT = ZKNSEARCH_OBJ_SHAPE_ICON_WINDOW + ZKNSEARCH_SHAPE_COUNT,
};

#define ZKNSEARCH_CHAR_RES_COUNT 6
#define ZKNSEARCH_PALETTE_RES_COUNT 6
#define ZKNSEARCH_CELL_RES_COUNT 4

// The windows
enum {
    ZKNSEARCH_WIN_MODE,
    ZKNSEARCH_WIN_INFO,
    ZKNSEARCH_WIN_ORDER_LABEL,
    ZKNSEARCH_WIN_INITIAL_LABEL,
    ZKNSEARCH_WIN_TYPE_LABEL,
    ZKNSEARCH_WIN_COLOR_LABEL,
    ZKNSEARCH_WIN_SHAPE_LABEL,
    ZKNSEARCH_WIN_ORDER,
    ZKNSEARCH_WIN_INITIAL,
    ZKNSEARCH_WIN_TYPE,
    ZKNSEARCH_WIN_COLOR,
    ZKNSEARCH_WIN_START,
    // On the top screen while a list is open: the criterion's name and its values
    ZKNSEARCH_WIN_TOP_LABEL,
    ZKNSEARCH_WIN_TOP_VALUE,
    ZKNSEARCH_WIN_TOP_VALUE2,
    ZKNSEARCH_WIN_MESSAGE,
    ZKNSEARCH_WIN_COUNT,
};

// The pages: the main page's is ZKNSEARCH_NONE before the first, else the criterion it came back from
enum {
    ZKNSEARCH_PAGE_ORDER,
    ZKNSEARCH_PAGE_INITIAL,
    ZKNSEARCH_PAGE_TYPE,
    ZKNSEARCH_PAGE_COLOR,
    ZKNSEARCH_PAGE_SHAPE,
};

// What the VBlank task does with the BGs of the lists (bgRequest)
#define ZKNSEARCH_BG_REQUEST_ON 1
#define ZKNSEARCH_BG_REQUEST_OFF 2

#define ZKNSEARCH_ITEM_NAME_COUNT 50

// The work, 0x2d4 bytes
typedef struct {
    ZukanSearchParam *param;
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
    ClActor *actors[ZKNSEARCH_OBJ_COUNT];
    u32 charRes[ZKNSEARCH_CHAR_RES_COUNT];
    u32 paletteRes[ZKNSEARCH_PALETTE_RES_COUNT];
    u32 cellRes[ZKNSEARCH_CELL_RES_COUNT];
    PrintWindow windows[ZKNSEARCH_WIN_COUNT];
    Font *font;
    MsgData *msgData;
    WordSet *wordSet;
    StrBuf *strbuf;
    PrintQueue *printQueue;
    FrameList *frameList;
    StrBuf *itemNames[ZKNSEARCH_ITEM_NAME_COUNT];
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
    ZukanSearchEngine *engine;
    BOOL searchDone;
} ZknSearchWork;

void ZknSearchMain_InitVBlank(ZknSearchWork *wk);
void ZknSearchMain_ExitVBlank(ZknSearchWork *wk);
void ZknSearchMain_InitHBlank(ZknSearchWork *wk);
void ZknSearchMain_ExitHBlank(ZknSearchWork *wk);
void ZknSearchMain_InitVRAM(void);
const BGSysVRAMConfig *ZknSearchMain_GetVRAMConfig(void);
void ZknSearchMain_InitBG(void);
void ZknSearchMain_ExitBG(void);
// Put the main page's frame over its text, or a list's text over its frame
void ZknSearchMain_SetMainBGPriority(void);
void ZknSearchMain_SetListBGPriority(void);
void ZknSearchMain_LoadGraphics(void);
void ZknSearchMain_InitPaletteFade(ZknSearchWork *wk);
void ZknSearchMain_ExitPaletteFade(ZknSearchWork *wk);
void ZknSearchMain_StartFade(ZknSearchWork *wk, u8 start, u8 end);
void ZknSearchMain_SetBlend(BOOL enabled);
void ZknSearchMain_InitWindow(void);
void ZknSearchMain_ExitWindow(void);
void ZknSearchMain_InitMsg(ZknSearchWork *wk);
void ZknSearchMain_ExitMsg(ZknSearchWork *wk);
void ZknSearchMain_InitBlinkAnm(ZknSearchWork *wk);
void ZknSearchMain_ExitBlinkAnm(ZknSearchWork *wk);
void ZknSearchMain_InitBGWinFrame(ZknSearchWork *wk);
void ZknSearchMain_ExitBGWinFrame(ZknSearchWork *wk);
void ZknSearchMain_LoadMainScreen(ZknSearchWork *wk);
void ZknSearchMain_LoadOrderScreen(ZknSearchWork *wk);
void ZknSearchMain_LoadInitialScreen(ZknSearchWork *wk);
void ZknSearchMain_LoadTypeScreen(ZknSearchWork *wk);
void ZknSearchMain_LoadColorScreen(ZknSearchWork *wk);
void ZknSearchMain_LoadShapeScreen(ZknSearchWork *wk);
void ZknSearchMain_LoadLoadingScreen(ZknSearchWork *wk);
void ZknSearchMain_FreeLoadingScreen(ZknSearchWork *wk);
// Show and hide the search's progress bar and its message
void ZknSearchMain_ShowLoading(ZknSearchWork *wk);
void ZknSearchMain_HideLoading(ZknSearchWork *wk);
void ZknSearchMain_RequestBGOn(ZknSearchWork *wk);
// Hides the page's BGs and clears their screens
void ZknSearchMain_ClearPage(ZknSearchWork *wk);
// The touch bar and its buttons, below the screen at first, slide by speed pixels a frame
void ZknSearchMain_InitFramePos(ZknSearchWork *wk);
void ZknSearchMain_StartFrameMove(ZknSearchWork *wk, s16 speed);
BOOL ZknSearchMain_UpdateFrameMove(ZknSearchWork *wk);
// A type from its position in the list, its position, and the message of the type at a position or of a type
u32 ZknSearchMain_ListPosToType(ZknSearchWork *wk, u32 pos);
u32 ZknSearchMain_TypeToListPos(ZknSearchWork *wk, u32 type);
u32 ZknSearchMain_GetListPosTypeMsg(ZknSearchWork *wk, u32 pos);
u32 ZknSearchMain_GetTypeMsg(ZknSearchWork *wk, u32 type);
// The criteria's shape is its position in the list while the screen runs
void ZknSearchMain_ShapeToListPos(ZknSearchWork *wk);
void ZknSearchMain_ListPosToShape(ZknSearchWork *wk);

#endif // POKEBW2_APP_ZKNSEARCH_MAIN_H
