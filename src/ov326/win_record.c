#include "types.h"
#include "app/ui/ui_scene.h"
#include "app/ui/print_msg.h"
#include "app/ui/frame_list.h"
#include "app/win_record.h"
#include "app/win_record_graphic.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "field/wbt.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "pml/poke_graphic.h"
#include "save/player_info.h"
#include "save/wbt_save.h"
#include "system/app_menu_common.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/str_tool.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/0411.h"

// The Pokémon World Tournament's win record: the player, their total of wins, and a list of the wins in each
// tournament that is open. The name is ours, after the ROM's "win_record_graphic.c", which draws it

// The kinds of cell actors, each with its own array in the work
enum {
    // The scroll bar and its arrow
    WIN_RECORD_ACTOR_SCROLL,
    // The buttons: the list's two arrows, then the X and return buttons
    WIN_RECORD_ACTOR_BUTTON,
    // The player's trainer sprite
    WIN_RECORD_ACTOR_TRAINER,
};

#define WIN_RECORD_SCROLL_ACTOR_COUNT 2
#define WIN_RECORD_BUTTON_COUNT 4
#define WIN_RECORD_TRAINER_ACTOR_COUNT 1
#define WIN_RECORD_WINDOW_COUNT 5
// The rectangles of the touch screen, from the archive, and the table's end
#define WIN_RECORD_TOUCH_RECT_COUNT 5
#define WIN_RECORD_TOURNAMENT_COUNT 29
// The rows the list shows at once
#define WIN_RECORD_LIST_ROWS 6
#define WIN_RECORD_MAX_WINS 9999

// The buttons
enum {
    WIN_RECORD_BUTTON_UP,
    WIN_RECORD_BUTTON_DOWN,
    WIN_RECORD_BUTTON_X,
    WIN_RECORD_BUTTON_RETURN,
};

// The bits of the work's flags. The first four are the buttons whose pressed animation is playing
#define WIN_RECORD_FLAG_BUTTONS 0xf
// The list has no more items than its rows, so it doesn't scroll
#define WIN_RECORD_FLAG_NO_SCROLL 0x40
#define WIN_RECORD_FLAG_RETURN 0x100
#define WIN_RECORD_FLAG_X 0x200
// The last input was with the keys
#define WIN_RECORD_FLAG_KEYS 0x400
// The list has been updated once, which is all a list that doesn't scroll needs
#define WIN_RECORD_FLAG_LIST_DONE 0x800
#define WIN_RECORD_FLAG_END 0x1000
// The list has been printed
#define WIN_RECORD_FLAG_LIST_PRINTED 0x10000

// The sequence
enum {
    WIN_RECORD_SEQ_INIT = 0,
    WIN_RECORD_SEQ_MAIN = 100,
    WIN_RECORD_SEQ_DOWN = 110,
    WIN_RECORD_SEQ_UP = 120,
    WIN_RECORD_SEQ_CLOSE = 200,
    WIN_RECORD_SEQ_END = 10000,
};

// The resources of the cell actors: the touch bar's, the screen's own and the trainer's, each a palette, characters
// and cells
enum {
    WIN_RECORD_RES_UI_PLTT,
    WIN_RECORD_RES_UI_CHARS,
    WIN_RECORD_RES_UI_CELLS,
    WIN_RECORD_RES_OBJ_PLTT,
    WIN_RECORD_RES_OBJ_CHARS,
    WIN_RECORD_RES_OBJ_CELLS,
    WIN_RECORD_RES_TRAINER_PLTT,
    WIN_RECORD_RES_TRAINER_CHARS,
    WIN_RECORD_RES_TRAINER_CELLS,
    WIN_RECORD_RES_COUNT,
};

typedef struct {
    HeapID heapId;
    u32 res[WIN_RECORD_RES_COUNT];
    ClActor *buttons[WIN_RECORD_BUTTON_COUNT];
    ClActor *scrollActors[WIN_RECORD_SCROLL_ACTOR_COUNT];
    ClActor *trainer[WIN_RECORD_TRAINER_ACTOR_COUNT];
    WinRecordGraphic *graphic;
    WbtOv326Param *param;
    Font *font;
    PrintQueue *printQueue;
    MsgData *msgData[2];
    PrintWindow windows[WIN_RECORD_WINDOW_COUNT];
    WordSet *wordSet;
    int seq;
    u32 unk8C;
    u32 flags;
    // The trainer class of the player's sprite
    u32 trainerClass;
    // The scroll of the background, in 1/256 pixels
    int bgScroll;
    // The tournaments in the list, and their total of wins
    int count;
    int totalWins;
    // The wins of each tournament, in the order of sWinRecordTournaments, and the indexes into it of the list's items
    s16 wins[31];
    s16 items[31];
    FrameList *list;
    // The positions of the cell actors, and the rectangles of the buttons, as x, y, width and height
    s16 *positions;
    s16 (*rects)[4];
    TouchRect touchRects[WIN_RECORD_TOUCH_RECT_COUNT + 1];
} WinRecordWork;

// A palette, characters or a screen of the BGs to load
typedef struct {
    u8 bg;
    u8 file;
    u8 offset;
    u8 size;
} WinRecordBGRes;

// A resource of a set of cell actors to load: its file, and where it goes in the work's resources
typedef struct {
    s16 file;
    s16 index;
    u16 offset;
    u16 count;
} WinRecordObjResFile;

typedef struct {
    WinRecordObjResFile pltt;
    WinRecordObjResFile chars;
    WinRecordObjResFile cells;
    WinRecordObjResFile anims;
} WinRecordObjRes;

// Which resources a set of cell actors is drawn with, how many there are, and which array of the work holds them
typedef struct {
    s16 pltt;
    s16 chars;
    s16 cells;
    s16 count;
    s16 kind;
} WinRecordActorRes;

// A cell actor: its setup, and its position as an index into the work's positions
typedef struct {
    u8 anim;
    u8 priority;
    u8 bgPriority;
    u8 visible;
    u8 unk4;
    u8 flags;
    u8 pos;
    u8 unk7;
} WinRecordActorData;

typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
} WinRecordWindowData;

// The animations of a button, normal and pressed
typedef struct {
    s16 normal;
    s16 pressed;
    s16 unk4;
    s16 unk6;
} WinRecordButtonAnims;

static BOOL WinRecord_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WinRecord_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WinRecord_Main(GameProc *proc, u32 *state, void *param, void *work);
static void WinRecord_InitWork(WinRecordWork *wk);
static void WinRecord_Update(WinRecordWork *wk);
static void WinRecord_ExitWork(WinRecordWork *wk);
static void WinRecord_RunSeq(WinRecordWork *wk);
static void WinRecord_Seq(WinRecordWork *wk);
static void WinRecord_FlushWindows(WinRecordWork *wk);
static s16 WinRecord_GetPosition(WinRecordWork *wk, u32 index);
static void WinRecord_LoadBG(WinRecordWork *wk);
static void WinRecord_CreateActors(WinRecordWork *wk, u32 surface, const WinRecordActorData *data,
                                   const WinRecordActorRes *res);
static void WinRecord_LoadObjRes(WinRecordWork *wk, u32 vramType, const WinRecordObjRes *res);
static void WinRecord_LoadUIRes(WinRecordWork *wk, u32 vramType);
static void WinRecord_LoadTrainerRes(WinRecordWork *wk, u32 vramType, u32 trainerClass);
static void WinRecord_InitObj(WinRecordWork *wk);
static void WinRecord_ExitObj(WinRecordWork *wk);
static void WinRecord_SetActorAnim(WinRecordWork *wk, int kind, int index, int anim);
static void WinRecord_SetActorVisible(WinRecordWork *wk, int kind, int index, BOOL visible);
static BOOL WinRecord_IsActorAnimating(WinRecordWork *wk, int kind, int index);
static void WinRecord_InitWindows(WinRecordWork *wk);
static void WinRecord_Print(WinRecordWork *wk, int window, int x, int y, const StrBuf *strbuf, u32 align, u16 color);
static void WinRecord_ExitWindows(WinRecordWork *wk);
static void WinRecord_PrintMsg(WinRecordWork *wk, int window, u32 msgId, int x, int y, u32 align);
static void WinRecord_PrintTotal(WinRecordWork *wk);
static void WinRecord_InitTouch(WinRecordWork *wk);
static void WinRecord_InitWins(WinRecordWork *wk);
static void WinRecord_PressButton(WinRecordWork *wk, int button);
static void WinRecord_SeqMain(WinRecordWork *wk);
static BOOL WinRecord_KeyInput(WinRecordWork *wk);
static void WinRecord_GetTouchRect(WinRecordWork *wk, int index, TouchRect *rect);
static BOOL WinRecord_TouchInput(WinRecordWork *wk);
static void WinRecord_UpdateButtons(WinRecordWork *wk);
static BOOL WinRecord_IsTournamentShown(WinRecordWork *wk, int tournament);
static void WinRecord_InitList(WinRecordWork *wk);
static u32 WinRecord_UpdateList(WinRecordWork *wk);
static int WinRecord_ListResult(WinRecordWork *wk, u32 result);
static void WinRecord_FreeList(WinRecordWork *wk);
static void WinRecord_ListPrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void WinRecord_ListSelect(void *work, u32 index, BOOL moved);
static void WinRecord_ListScroll(void *work, s16 delta);
static void WinRecord_PrintItem(WinRecordWork *wk, u32 index, PrintWindow *window, MsgData *msgData);
static void WinRecord_PrintItemAt(WinRecordWork *wk, PrintWindow *window, PrintQueue *queue, MsgData *msgData,
                                  u32 msgId, u16 color, int y, u32 index);
static void WinRecord_PrintItemLine(WinRecordWork *wk, PrintWindow *window, PrintQueue *queue, MsgData *msgData,
                                    u32 msgId, u32 index);
static void WinRecord_UpdateScrollBar(WinRecordWork *wk);
static void WinRecord_ScrollBG(WinRecordWork *wk);
static void WinRecord_ListInput(WinRecordWork *wk, BOOL updated);

static const WinRecordActorRes sWinRecordButtonActorRes = {
    WIN_RECORD_RES_UI_PLTT,  WIN_RECORD_RES_UI_CHARS, WIN_RECORD_RES_UI_CELLS,
    WIN_RECORD_BUTTON_COUNT, WIN_RECORD_ACTOR_BUTTON,
};

static const WinRecordActorRes sWinRecordTrainerActorRes = {
    WIN_RECORD_RES_TRAINER_PLTT,    WIN_RECORD_RES_TRAINER_CHARS, WIN_RECORD_RES_TRAINER_CELLS,
    WIN_RECORD_TRAINER_ACTOR_COUNT, WIN_RECORD_ACTOR_TRAINER,
};

static const WinRecordActorRes sWinRecordScrollActorRes = {
    WIN_RECORD_RES_OBJ_PLTT,       WIN_RECORD_RES_OBJ_CHARS, WIN_RECORD_RES_OBJ_CELLS,
    WIN_RECORD_SCROLL_ACTOR_COUNT, WIN_RECORD_ACTOR_SCROLL,
};

static const WinRecordBGRes sWinRecordBGPltts[] = {
    { 4, 1, 0, 4 },
    { 0, 0, 0, 3 },
    { 0xff },
};

static const FrameListCallbacks sWinRecordListCallbacks = {
    WinRecord_ListPrint,
    WinRecord_ListSelect,
    WinRecord_ListScroll,
};

static const WinRecordBGRes sWinRecordBGScreens[] = {
    { 7, 10 }, { 6, 11 }, { 5, 12 }, { 3, 7 }, { 1, 8 }, { 0xff },
};

static const WinRecordActorData sWinRecordScrollActors[] = {
    { 0, 0x30, 2, TRUE, 0, 1, 4 },
    { 1, 0x38, 2, TRUE, 0, 1, 14 },
    { 0xff },
};

static const WinRecordBGRes sWinRecordBGChars[] = {
    { 7, 4 }, { 6, 4 }, { 5, 4 }, { 3, 3 }, { 1, 3 }, { 0xff },
};

static const WinRecordActorData sWinRecordButtonActors[] = {
    { 4, 0x30, 1, TRUE, 0, 1, 7 },
    { 5, 0x30, 1, TRUE, 0, 1, 8 },
    { 0, 0x30, 1, TRUE, 0, 1, 9 },
    { 1, 0x30, 1, TRUE, 0, 1, 10 },
    { 0xff },
};

static const WinRecordObjRes sWinRecordObjRes = {
    { 2, WIN_RECORD_RES_OBJ_PLTT, 0, 11 },
    { 5, WIN_RECORD_RES_OBJ_CHARS },
    { 6, WIN_RECORD_RES_OBJ_CELLS },
    { 13, -1 },
};

static const WinRecordButtonAnims sWinRecordButtonAnims[WIN_RECORD_BUTTON_COUNT] = {
    { 4, 12, 18, 0 },
    { 5, 13, 19, 0 },
    { 0, 8, 0, 0 },
    { 1, 9, 1, 0 },
};

static const WinRecordActorData sWinRecordTrainerActors[] = {
    { 0, 0x30, 1, TRUE, 0, 1, 13 },
    { 0xff },
};

static const WinRecordWindowData sWinRecordWindows[] = {
    { 0, 1, 1, 30, 2, 8 },  { 4, 2, 7, 16, 2, 2 },  { 4, 4, 9, 16, 2, 2 },
    { 4, 2, 12, 20, 2, 2 }, { 4, 4, 14, 16, 2, 2 }, { 0xff },
};

// The rows and buttons of a list that doesn't scroll
static const FrameListTouch sWinRecordListTouchRectsNoScroll[] = {
    { { 0x18, 0x2f, 0x08, 0xe8 }, 0 }, { { 0x30, 0x47, 0x08, 0xe8 }, 0 }, { { 0x48, 0x5f, 0x08, 0xe8 }, 0 },
    { { 0x60, 0x77, 0x08, 0xe8 }, 0 }, { { 0x78, 0x8f, 0x08, 0xe8 }, 0 }, { { 0x90, 0xa7, 0x08, 0xe8 }, 0 },
    { { 0xa8, 0xc0, 0x88, 0xa0 }, 4 }, { { 0xa8, 0xc0, 0xa8, 0xc0 }, 5 }, { { TOUCH_RECT_END } },
};

// The rows, the scroll bar and the buttons
static const FrameListTouch sWinRecordListTouchRects[] = {
    { { 0x18, 0x2f, 0x08, 0xe8 }, 0 }, { { 0x30, 0x47, 0x08, 0xe8 }, 0 },
    { { 0x48, 0x5f, 0x08, 0xe8 }, 0 }, { { 0x60, 0x77, 0x08, 0xe8 }, 0 },
    { { 0x78, 0x8f, 0x08, 0xe8 }, 0 }, { { 0x90, 0xa7, 0x08, 0xe8 }, 0 },
    { { 0x20, 0xa0, 0xf4, 0xfc }, 1 }, { { 0xa8, 0xc0, 0x88, 0xa0 }, 4 },
    { { 0xa8, 0xc0, 0xa8, 0xc0 }, 5 }, { { TOUCH_RECT_END } },
};

static const FrameListSetup sWinRecordListSetup = {
    { 2, 0xff, 1, 3, 28, 3, 1, 0, 26, 3, 2, 24, 12, 8, 6, 4, 3, 2, 16, 0 },
    7,
    1,
    0,
    6,
    0,
    sWinRecordListTouchRects,
    NULL,
    NULL,
};

// The message of each tournament's name
static const s16 sWinRecordTournamentNames[WIN_RECORD_TOURNAMENT_COUNT] = {
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
};

const GameProcFunctions WIN_RECORD_PROC_FUNCTIONS = {
    WinRecord_Init,
    WinRecord_Main,
    WinRecord_Exit,
};

// The tournaments in the order of the list, by their count in the save
static const s16 sWinRecordTournaments[WIN_RECORD_TOURNAMENT_COUNT] = {
    18, 19, 20, 21, 22, 23, 24, 17, 25, 26, 27, 28, 0, 9, 10, 11, 12, 14, 1, 3, 4, 2, 13, 6, 5, 7, 15, 16, 8,
};

static BOOL WinRecord_Init(GameProc *proc, u32 *state, void *param, void *work) {
    WinRecordWork *wk;

    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_WBT_RECORD, 0x30000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(WinRecordWork), HEAPID_WBT_RECORD);
    sys_memset(wk, 0, sizeof(WinRecordWork));
    wk->heapId = HEAPID_WBT_RECORD;
    wk->param = param;
    wk->graphic = WinRecordGraphic_Create(0, wk->heapId);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
    wk->msgData[0] = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0411, wk->heapId);
    wk->msgData[1] = NULL;
    wk->printQueue = func_02021998(wk->heapId);
    wk->wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
    WinRecord_InitWork(wk);
    GFL_WipeSet(0, 1, 1, 0, 6, 1, wk->heapId);
    return TRUE;
}

static BOOL WinRecord_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    WinRecordWork *wk = work;
    HeapID heapId;
    int i;

    if (wk->flags & WIN_RECORD_FLAG_KEYS) {
        func_0203d564(FALSE);
    } else {
        func_0203d564(TRUE);
    }
    WinRecord_ExitWork(wk);
    for (i = 0; i < 2; i++) {
        if (wk->msgData[i] != NULL) {
            // BUG: Frees the first message data for each of them. It is latent and harmless, as only the first is ever
            // loaded
#ifdef BUGFIX
            GFL_MsgDataFree(wk->msgData[i]);
#else
            GFL_MsgDataFree(wk->msgData[0]);
#endif
            wk->msgData[i] = NULL;
        }
    }
    GFL_WordSetSystemFree(wk->wordSet);
    func_02021a18(wk->printQueue);
    GFL_FontFree(wk->font);
    WinRecordGraphic_Delete(wk->graphic);
    heapId = wk->heapId;
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(heapId);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return TRUE;
}

static BOOL WinRecord_Main(GameProc *proc, u32 *state, void *param, void *work) {
    WinRecordWork *wk = work;

    switch (*state) {
    case 0:
        if (GFL_WipeIsFinished()) {
            (*state)++;
        }
        break;
    case 1:
        if (wk->seq == WIN_RECORD_SEQ_END && func_02021c0c(wk->printQueue) == TRUE) {
            (*state)++;
        }
        break;
    case 2:
        GFL_WipeSet(0, 0, 0, 0, 6, 1, wk->heapId);
        (*state)++;
        break;
    case 3:
        if (GFL_WipeIsFinished()) {
            return TRUE;
        }
        break;
    }
    WinRecord_Update(wk);
    WinRecord_FlushWindows(wk);
    WinRecordGraphic_Main(wk->graphic);
    WinRecordGraphic_Begin3D(wk->graphic);
    WinRecordGraphic_End3D(wk->graphic);
    return FALSE;
}

static void WinRecord_InitWork(WinRecordWork *wk) {
    int i;

    wk->seq = WIN_RECORD_SEQ_INIT;
    wk->unk8C = wk->param->unk0;
    wk->flags = 0;
    wk->trainerClass = 0;
    wk->count = 0;
    wk->bgScroll = 0;
    wk->totalWins = 0;
    for (i = 0; i < 31; i++) {
        wk->wins[i] = i;
        wk->items[i] = 0;
    }
    wk->positions = GFL_ArcSysReadHeapNew(ARCID_WIN_RECORD, 15, wk->heapId);
    wk->rects = GFL_ArcSysReadHeapNew(ARCID_WIN_RECORD, 14, wk->heapId);
    WinRecord_InitWins(wk);
    WinRecord_LoadBG(wk);
    WinRecord_InitWindows(wk);
    WinRecord_InitObj(wk);
    WinRecord_InitTouch(wk);
    WinRecord_InitList(wk);
}

static void WinRecord_Update(WinRecordWork *wk) {
    WinRecord_RunSeq(wk);
    WinRecord_ScrollBG(wk);
}

static void WinRecord_ExitWork(WinRecordWork *wk) {
    if (wk->flags & WIN_RECORD_FLAG_X) {
        if (wk->param->unk4 != NULL) {
            *wk->param->unk4 = TRUE;
        }
    } else {
        if (wk->param->unk4 != NULL) {
            *wk->param->unk4 = FALSE;
        }
    }
    GFL_HeapFree(wk->positions);
    GFL_HeapFree(wk->rects);
    WinRecord_FreeList(wk);
    WinRecord_ExitWindows(wk);
    WinRecord_ExitObj(wk);
}

static void WinRecord_RunSeq(WinRecordWork *wk) {
    WinRecord_Seq(wk);
}

static void WinRecord_Seq(WinRecordWork *wk) {
    switch (wk->seq) {
    case WIN_RECORD_SEQ_INIT:
        if (!(wk->flags & WIN_RECORD_FLAG_LIST_PRINTED)) {
            if (FrameList_Draw(wk->list) == FALSE) {
                wk->seq = WIN_RECORD_SEQ_MAIN;
                wk->flags |= WIN_RECORD_FLAG_LIST_PRINTED;
                WinRecord_ListInput(wk, FALSE);
            }
        } else {
            wk->seq = WIN_RECORD_SEQ_MAIN;
            WinRecord_ListInput(wk, FALSE);
        }
        break;
    case WIN_RECORD_SEQ_MAIN:
        WinRecord_SeqMain(wk);
        break;
    case WIN_RECORD_SEQ_DOWN:
        if (WinRecord_IsActorAnimating(wk, WIN_RECORD_ACTOR_BUTTON, WIN_RECORD_BUTTON_DOWN) == FALSE) {
            WinRecord_SetActorAnim(wk, WIN_RECORD_ACTOR_BUTTON, WIN_RECORD_BUTTON_DOWN, 5);
            wk->flags &= ~(1 << WIN_RECORD_BUTTON_DOWN);
            wk->seq = WIN_RECORD_SEQ_MAIN;
        }
        break;
    case WIN_RECORD_SEQ_UP:
        if (WinRecord_IsActorAnimating(wk, WIN_RECORD_ACTOR_BUTTON, WIN_RECORD_BUTTON_UP) == FALSE) {
            WinRecord_SetActorAnim(wk, WIN_RECORD_ACTOR_BUTTON, WIN_RECORD_BUTTON_UP, 4);
            wk->flags &= ~(1 << WIN_RECORD_BUTTON_UP);
            wk->seq = WIN_RECORD_SEQ_MAIN;
        }
        break;
    case WIN_RECORD_SEQ_CLOSE:
        WinRecord_UpdateButtons(wk);
        if (wk->flags & WIN_RECORD_FLAG_END) {
            wk->seq = WIN_RECORD_SEQ_END;
        }
        break;
    case WIN_RECORD_SEQ_END:
        break;
    }
}

static void WinRecord_FlushWindows(WinRecordWork *wk) {
    int i;

    func_02021a3c(wk->printQueue);
    for (i = 0; i < WIN_RECORD_WINDOW_COUNT; i++) {
        PrintWindow_Flush(&wk->windows[i], wk->printQueue);
    }
}

static s16 WinRecord_GetPosition(WinRecordWork *wk, u32 index) {
    if (index >= 30) {
        return 0;
    }
    return wk->positions[index];
}

static void WinRecord_LoadBG(WinRecordWork *wk) {
    int i;
    const WinRecordBGRes *pltts = sWinRecordBGPltts;
    const WinRecordBGRes *chars = sWinRecordBGChars;
    const WinRecordBGRes *screens = sWinRecordBGScreens;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_WIN_RECORD, wk->heapId);

    for (i = 0; i < 8; i++) {
        if (pltts->bg == 0xff) {
            break;
        }
        GFL_G2DIOLoadArcNCLRDefault(arc, pltts->file, pltts->bg, pltts->offset * 32, pltts->size * 32, wk->heapId);
        pltts++;
    }
    for (i = 0; i < 8; i++) {
        if (chars->bg == 0xff) {
            break;
        }
        GFL_BGSysLoadArcNCGRStatic(arc, chars->file, chars->bg, chars->offset, 0, FALSE, wk->heapId);
        chars++;
    }
    for (i = 0; i < 8; i++) {
        if (screens->bg == 0xff) {
            break;
        }
        loadBGScrToVramByFileNoReserveNegAlign(arc, screens->file, screens->bg, screens->offset, 0, FALSE, wk->heapId);
        screens++;
    }
    GFL_ArcToolFree(arc);
}

static void WinRecord_CreateActors(WinRecordWork *wk, u32 surface, const WinRecordActorData *data,
                                   const WinRecordActorRes *res) {
    int i;
    ClActUnit *unit = WinRecordGraphic_GetClActUnit(wk->graphic);
    ClActorSetup setup;
    ClActor **actors;
    s16 pltt, chars, cells, count;
    u32 pos;
    s16 x, y;
    BOOL visible, autoAnim;

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    pltt = res->pltt;
    chars = res->chars;
    cells = res->cells;
    count = res->count;
    if (res->kind == WIN_RECORD_ACTOR_SCROLL) {
        actors = wk->scrollActors;
    } else if (res->kind == WIN_RECORD_ACTOR_BUTTON) {
        actors = wk->buttons;
    } else {
        actors = wk->trainer;
    }
    for (i = 0; i < count; i++) {
        if (data->anim == 0xff) {
            break;
        }
        pos = data->pos * 2;
        x = WinRecord_GetPosition(wk, pos);
        y = WinRecord_GetPosition(wk, pos + 1);
        setup.x = x;
        setup.y = y;
        setup.sequence = data->anim;
        setup.priority = data->priority;
        setup.bgPriority = data->bgPriority;
        actors[i] = func_0204c040(unit, wk->res[chars], wk->res[pltt], wk->res[cells], &setup, surface, wk->heapId);
        visible = FALSE;
        if (data->visible == TRUE) {
            visible = TRUE;
        }
        func_0204c124(actors[i], visible);
        autoAnim = FALSE;
        if (data->flags & 1) {
            autoAnim = TRUE;
        }
        func_0204c520(actors[i], autoAnim);
        if (data->flags & 2) {
            func_0204c2b0(actors[i], 1, TRUE);
        }
        if (data->flags & 4) {
            func_0204c2b0(actors[i], 0, TRUE);
        }
        data++;
    }
}

static void WinRecord_LoadObjRes(WinRecordWork *wk, u32 vramType, const WinRecordObjRes *res) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_WIN_RECORD, wk->heapId);
    ClActUnit *unit = WinRecordGraphic_GetClActUnit(wk->graphic);

    wk->res[res->pltt.index] =
        func_0204bbb8(arc, res->pltt.file, vramType, res->pltt.offset, 0, res->pltt.count, wk->heapId);
    wk->res[res->chars.index] = func_0204b81c(arc, res->chars.file, FALSE, vramType, wk->heapId);
    wk->res[res->cells.index] = func_0204bde0(arc, res->cells.file, res->anims.file, wk->heapId);
    GFL_ArcToolFree(arc);
}

static void WinRecord_LoadUIRes(WinRecordWork *wk, u32 vramType) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), wk->heapId);
    ClActUnit *unit = WinRecordGraphic_GetClActUnit(wk->graphic);
    u32 pltt = func_0202d810();
    u32 chars = func_0202d814();
    u32 cells = func_0202d818(2);
    u32 anims = func_0202d81c(2);

    wk->res[WIN_RECORD_RES_UI_PLTT] = func_0204bbb8(arc, pltt, vramType, 0x160, 0, 4, wk->heapId);
    wk->res[WIN_RECORD_RES_UI_CHARS] = func_0204b81c(arc, chars, FALSE, vramType, wk->heapId);
    wk->res[WIN_RECORD_RES_UI_CELLS] = func_0204bde0(arc, cells, anims, wk->heapId);
    GFL_ArcToolFree(arc);
}

static void WinRecord_LoadTrainerRes(WinRecordWork *wk, u32 vramType, u32 trainerClass) {
    ArcTool *arc = MakeTrGraArcHandle(wk->heapId);

    wk->res[WIN_RECORD_RES_TRAINER_PLTT] = TrGra_LoadClActPalette(arc, trainerClass, vramType, 0, wk->heapId);
    wk->res[WIN_RECORD_RES_TRAINER_CHARS] = TrGra_LoadClActChars(arc, trainerClass, vramType, wk->heapId);
    wk->res[WIN_RECORD_RES_TRAINER_CELLS] = TrGra_LoadClActCellAnims(trainerClass, 0, vramType, wk->heapId);
    GFL_ArcToolFree(arc);
}

static void WinRecord_InitObj(WinRecordWork *wk) {
    ClActUnit *unit = WinRecordGraphic_GetClActUnit(wk->graphic);

    WinRecord_LoadObjRes(wk, 0, &sWinRecordObjRes);
    WinRecord_LoadUIRes(wk, 0);
    WinRecord_LoadTrainerRes(wk, 1, wk->trainerClass);
    WinRecord_CreateActors(wk, 0, sWinRecordScrollActors, &sWinRecordScrollActorRes);
    WinRecord_CreateActors(wk, 0, sWinRecordButtonActors, &sWinRecordButtonActorRes);
    WinRecord_CreateActors(wk, 1, sWinRecordTrainerActors, &sWinRecordTrainerActorRes);
    func_02042ba8(TRUE, wk->heapId);
}

static void WinRecord_ExitObj(WinRecordWork *wk) {
    int i;

    for (i = 0; i < WIN_RECORD_SCROLL_ACTOR_COUNT; i++) {
        func_0204c108(wk->scrollActors[i]);
    }
    for (i = 0; i < WIN_RECORD_BUTTON_COUNT; i++) {
        func_0204c108(wk->buttons[i]);
    }
    func_0204c108(wk->trainer[0]);
    func_0204bcd0(wk->res[WIN_RECORD_RES_OBJ_PLTT]);
    func_0204b98c(wk->res[WIN_RECORD_RES_OBJ_CHARS]);
    func_0204be64(wk->res[WIN_RECORD_RES_OBJ_CELLS]);
    func_0204bcd0(wk->res[WIN_RECORD_RES_UI_PLTT]);
    func_0204b98c(wk->res[WIN_RECORD_RES_UI_CHARS]);
    func_0204be64(wk->res[WIN_RECORD_RES_UI_CELLS]);
    func_0204bcd0(wk->res[WIN_RECORD_RES_TRAINER_PLTT]);
    func_0204b98c(wk->res[WIN_RECORD_RES_TRAINER_CHARS]);
    func_0204be64(wk->res[WIN_RECORD_RES_TRAINER_CELLS]);
}

static void WinRecord_SetActorAnim(WinRecordWork *wk, int kind, int index, int anim) {
    if (kind == WIN_RECORD_ACTOR_SCROLL) {
        if (index < WIN_RECORD_SCROLL_ACTOR_COUNT) {
            func_0204c488(wk->scrollActors[index], anim);
            func_0204c520(wk->scrollActors[index], TRUE);
        }
    } else if (kind == WIN_RECORD_ACTOR_BUTTON) {
        if (index < WIN_RECORD_BUTTON_COUNT) {
            func_0204c488(wk->buttons[index], anim);
            func_0204c520(wk->buttons[index], TRUE);
        }
    } else if (kind == WIN_RECORD_ACTOR_TRAINER) {
        if (index < WIN_RECORD_TRAINER_ACTOR_COUNT) {
            func_0204c488(wk->trainer[index], anim);
            func_0204c520(wk->trainer[index], TRUE);
        }
    }
}

static void WinRecord_SetActorVisible(WinRecordWork *wk, int kind, int index, BOOL visible) {
    if (kind == WIN_RECORD_ACTOR_SCROLL) {
        if (index < WIN_RECORD_SCROLL_ACTOR_COUNT) {
            func_0204c124(wk->scrollActors[index], visible);
        }
    } else if (kind == WIN_RECORD_ACTOR_BUTTON) {
        if (index < WIN_RECORD_BUTTON_COUNT) {
            func_0204c124(wk->buttons[index], visible);
        }
    } else if (kind == WIN_RECORD_ACTOR_TRAINER) {
        if (index < WIN_RECORD_TRAINER_ACTOR_COUNT) {
            func_0204c124(wk->trainer[index], visible);
        }
    }
}

static BOOL WinRecord_IsActorAnimating(WinRecordWork *wk, int kind, int index) {
    BOOL animating = TRUE;

    if (kind == WIN_RECORD_ACTOR_SCROLL) {
        if (index >= WIN_RECORD_SCROLL_ACTOR_COUNT) {
            return FALSE;
        }
        if (func_0204c560(wk->scrollActors[index]) == FALSE) {
            animating = FALSE;
        }
    } else if (kind == WIN_RECORD_ACTOR_BUTTON) {
        if (index >= WIN_RECORD_BUTTON_COUNT) {
            return FALSE;
        }
        if (func_0204c560(wk->buttons[index]) == FALSE) {
            animating = FALSE;
        }
    } else if (kind == WIN_RECORD_ACTOR_TRAINER) {
        if (index >= WIN_RECORD_TRAINER_ACTOR_COUNT) {
            return FALSE;
        }
        if (func_0204c560(wk->trainer[index]) == FALSE) {
            animating = FALSE;
        }
    }
    return animating;
}

static void WinRecord_InitWindows(WinRecordWork *wk) {
    int i;
    const WinRecordWindowData *data = sWinRecordWindows;
    ArcTool *arc;

    for (i = 0; i < WIN_RECORD_WINDOW_COUNT; i++) {
        if (data->bg == 0xff) {
            break;
        }
        wk->windows[i].window =
            BmpWin_CreateDynamic(data->bg, data->x, data->y, data->width, data->height, data->palette, TRUE);
        BmpWin_FlushMap(wk->windows[i].window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(wk->windows[i].window));
        data++;
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_FONT, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 6, 4, 0x100, 0x20, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 6, 0, 0x100, 0x20, wk->heapId);
    GFL_ArcToolFree(arc);
    WinRecord_PrintMsg(wk, 0, 1, 0, 0, 0);
    WinRecord_PrintTotal(wk);
}

static void WinRecord_Print(WinRecordWork *wk, int window, int x, int y, const StrBuf *strbuf, u32 align, u16 color) {
    PrintStrAligned(&wk->windows[window], wk->printQueue, x, y, strbuf, wk->font, color, align);
}

static void WinRecord_ExitWindows(WinRecordWork *wk) {
    int i;

    for (i = 0; i < WIN_RECORD_WINDOW_COUNT; i++) {
        BmpWin_Free(wk->windows[i].window);
    }
}

static void WinRecord_PrintMsg(WinRecordWork *wk, int window, u32 msgId, int x, int y, u32 align) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData[0], msgId);

    WinRecord_Print(wk, window, x, y, strbuf, align, 0x3c40);
    GFL_StrBufFree(strbuf);
}

static void WinRecord_PrintTotal(WinRecordWork *wk) {
    // Unused, and only here for the layout of .rodata. As u16s it reads 1, 2, 0x22, 0x21 and 0x20, which may be the
    // messages the record prints
    static const u32 unused[] = { 0x00020001, 0x00210022, 0x00000020 };
    StrBuf *strbuf;
    StrBuf *name;
    StrBuf *format;
    StrBuf *number;

    strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData[0], Bank0411_Text_Name);
    WinRecord_Print(wk, 1, 0, 0, strbuf, 0, 0x39e0);
    GFL_StrBufFree(strbuf);

    name = GFL_StrBufCreate(64, wk->heapId);
    textCopy(wk->param->playerInfo->name, name);
    func_0202437c(wk->wordSet, 0, name, 0, 1, 2);
    strbuf = GFL_StrBufCreate(64, wk->heapId);
    format = GFL_MsgDataLoadStrbufNew(wk->msgData[0], Bank0411_Text_Empty_3);
    GFL_WordSetFormatStrbuf(wk->wordSet, strbuf, format);
    WinRecord_Print(wk, 2, 0, 0, strbuf, 0, 0x39e0);
    GFL_StrBufFree(format);
    GFL_StrBufFree(name);
    GFL_StrBufFree(strbuf);

    strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData[0], Bank0411_Text_TotalWins);
    WinRecord_Print(wk, 3, 0, 0, strbuf, 0, 0x39e0);
    GFL_StrBufFree(strbuf);

    number = GFL_StrBufCreate(64, wk->heapId);
    GFL_WordSetFormatNumber(number, wk->totalWins, 4, 1, TRUE);
    func_0202437c(wk->wordSet, 0, number, 0, 1, 2);
    strbuf = GFL_StrBufCreate(64, wk->heapId);
    format = GFL_MsgDataLoadStrbufNew(wk->msgData[0], Bank0411_Text_Empty_2);
    GFL_WordSetFormatStrbuf(wk->wordSet, strbuf, format);
    WinRecord_Print(wk, 4, 0, 0, strbuf, 0, 0x39e0);
    GFL_StrBufFree(format);
    GFL_StrBufFree(number);
    GFL_StrBufFree(strbuf);
}

static void WinRecord_InitTouch(WinRecordWork *wk) {
    s16 i;
    TouchRect *rect = wk->touchRects;

    for (i = 0; i < WIN_RECORD_TOUCH_RECT_COUNT; i++) {
        WinRecord_GetTouchRect(wk, i, rect);
        rect++;
    }
    rect->top = TOUCH_RECT_END;
    rect->bottom = 0;
    rect->left = 0;
    rect->right = 0;
    if (wk->flags & WIN_RECORD_FLAG_NO_SCROLL) {
        WinRecord_SetActorVisible(wk, WIN_RECORD_ACTOR_SCROLL, 0, FALSE);
        WinRecord_SetActorVisible(wk, WIN_RECORD_ACTOR_BUTTON, WIN_RECORD_BUTTON_UP, FALSE);
        WinRecord_SetActorVisible(wk, WIN_RECORD_ACTOR_BUTTON, WIN_RECORD_BUTTON_DOWN, FALSE);
    }
}

static void WinRecord_InitWins(WinRecordWork *wk) {
    int i;
    int count;
    int total;
    u16 wins;
    u32 gender = getTrainerGender(wk->param->playerInfo);

    wk->trainerClass = 0;
    if (gender == GENDER_FEMALE) {
        wk->trainerClass = 1;
    }
    for (i = 0; i < WIN_RECORD_TOURNAMENT_COUNT; i++) {
        wins = func_0200feac(wk->param->save, sWinRecordTournaments[i]);
        if (wins >= WIN_RECORD_MAX_WINS) {
            wins = WIN_RECORD_MAX_WINS;
        }
        wk->wins[i] = wins;
    }
    count = 0;
    for (i = 0; i < WIN_RECORD_TOURNAMENT_COUNT; i++) {
        if (WinRecord_IsTournamentShown(wk, sWinRecordTournaments[i]) == TRUE) {
            wk->items[count] = i;
            count++;
        }
    }
    wk->count = count;
    if (count <= WIN_RECORD_LIST_ROWS) {
        wk->flags |= WIN_RECORD_FLAG_NO_SCROLL;
    }
    total = 0;
    for (i = 0; i < wk->count; i++) {
        total += wk->wins[wk->items[i]];
    }
    if (total >= WIN_RECORD_MAX_WINS) {
        total = WIN_RECORD_MAX_WINS;
    }
    wk->totalWins = total;
}

static void WinRecord_PressButton(WinRecordWork *wk, int button) {
    WinRecord_SetActorAnim(wk, WIN_RECORD_ACTOR_BUTTON, button, sWinRecordButtonAnims[button].pressed);
    wk->flags |= 1 << button;
}

static void WinRecord_SeqMain(WinRecordWork *wk) {
    BOOL done = FALSE;

    WinRecord_UpdateButtons(wk);
    if (WinRecord_ListResult(wk, WinRecord_UpdateList(wk))) {
        done = TRUE;
    }
    if (done == FALSE && WinRecord_TouchInput(wk) == FALSE) {
        WinRecord_KeyInput(wk);
    }
    WinRecord_UpdateScrollBar(wk);
}

static BOOL WinRecord_KeyInput(WinRecordWork *wk) {
    BOOL pressed = FALSE;
    BOOL result = FALSE;

    if (wk->flags & WIN_RECORD_FLAG_END) {
        return FALSE;
    }
    if (wk->flags & WIN_RECORD_FLAG_BUTTONS) {
        return FALSE;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) {
        WinRecord_PressButton(wk, WIN_RECORD_BUTTON_X);
        pressed = TRUE;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
    } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        WinRecord_PressButton(wk, WIN_RECORD_BUTTON_RETURN);
        pressed = TRUE;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
    }
    if (pressed == TRUE) {
        wk->seq = WIN_RECORD_SEQ_CLOSE;
        result = TRUE;
        wk->flags |= WIN_RECORD_FLAG_KEYS;
    }
    return result;
}

static void WinRecord_GetTouchRect(WinRecordWork *wk, int index, TouchRect *rect) {
    s16 i;
    s16 bounds[4];

    for (i = 0; i < 4; i++) {
        bounds[i] = wk->rects[index][i];
    }
    bounds[2] += bounds[0];
    bounds[3] += bounds[1];
    for (i = 0; i < 4; i++) {
        if (bounds[i] < 0) {
            bounds[i] = 0;
        }
        if (bounds[i] > 255) {
            bounds[i] = 255;
        }
    }
    rect->left = bounds[0];
    rect->top = bounds[1];
    rect->right = bounds[2];
    rect->bottom = bounds[3];
}

static BOOL WinRecord_TouchInput(WinRecordWork *wk) {
    int pressed = 0;
    int touched = func_0203da0c(wk->touchRects);
    BOOL result = FALSE;

    if (wk->flags & WIN_RECORD_FLAG_END) {
        return FALSE;
    }
    if (wk->flags & WIN_RECORD_FLAG_BUTTONS) {
        return FALSE;
    }
    if (touched == WIN_RECORD_BUTTON_X) {
        WinRecord_PressButton(wk, WIN_RECORD_BUTTON_X);
        pressed = 2;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
    } else if (touched == WIN_RECORD_BUTTON_RETURN) {
        WinRecord_PressButton(wk, WIN_RECORD_BUTTON_RETURN);
        pressed = 2;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
    }
    if (pressed) {
        result = TRUE;
        wk->seq = WIN_RECORD_SEQ_CLOSE;
    }
    return result;
}

static void WinRecord_UpdateButtons(WinRecordWork *wk) {
    BOOL pressed = FALSE;
    s16 button = 0;

    if (wk->flags & (1 << WIN_RECORD_BUTTON_X)) {
        button = WIN_RECORD_BUTTON_X;
        pressed = TRUE;
    } else if (wk->flags & (1 << WIN_RECORD_BUTTON_RETURN)) {
        button = WIN_RECORD_BUTTON_RETURN;
        pressed = TRUE;
    }
    if (pressed == TRUE && WinRecord_IsActorAnimating(wk, WIN_RECORD_ACTOR_BUTTON, button) == FALSE) {
        WinRecord_SetActorAnim(wk, WIN_RECORD_ACTOR_BUTTON, button, sWinRecordButtonAnims[button].normal);
        wk->flags &= ~(1 << button);
        if (button == WIN_RECORD_BUTTON_X || button == WIN_RECORD_BUTTON_RETURN) {
            wk->flags |= WIN_RECORD_FLAG_END;
            if (button == WIN_RECORD_BUTTON_X) {
                wk->flags |= WIN_RECORD_FLAG_X;
            } else if (button == WIN_RECORD_BUTTON_RETURN) {
                wk->flags |= WIN_RECORD_FLAG_RETURN;
            }
        }
    }
}

static BOOL WinRecord_IsTournamentShown(WinRecordWork *wk, int tournament) {
    BOOL shown = FALSE;

    if (wk->param->open[tournament] == TRUE) {
        shown = TRUE;
    }
    // The save's count 18 is listed even when its tournament isn't open
    if (tournament == 18) {
        shown = TRUE;
    }
    return shown;
}

static void WinRecord_InitList(WinRecordWork *wk) {
    FrameListSetup setup = sWinRecordListSetup;
    ArcTool *arc;
    int i;

    setup.work = wk;
    WinRecord_FreeList(wk);
    arc = GFL_ArcSysCreateFileHandle(ARCID_WIN_RECORD, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 3, 2, 0, 0, FALSE, wk->heapId);
    setup.callbacks = &sWinRecordListCallbacks;
    setup.count = wk->count;
    setup.cursorPos = 0;
    setup.scroll = 0;
    if (wk->count <= WIN_RECORD_LIST_ROWS) {
        setup.touch = sWinRecordListTouchRectsNoScroll;
    }
    wk->list = FrameList_Create(&setup, wk->heapId);
    FrameList_LoadScreen(wk->list, arc, 9, FALSE, 0);
    FrameList_LoadCursorPalette(wk->list, arc, 0, 2, 2);
    GFL_ArcToolFree(arc);
    for (i = 0; i < setup.count; i++) {
        FrameList_AddItem(wk->list, 0, sWinRecordTournamentNames[wk->items[i]]);
    }
    wk->flags &= ~WIN_RECORD_FLAG_LIST_PRINTED;
    if (FrameList_Draw(wk->list) == FALSE) {
        wk->flags |= WIN_RECORD_FLAG_LIST_PRINTED;
    }
    FrameList_SetShownRows(wk->list, 7);
}

static u32 WinRecord_UpdateList(WinRecordWork *wk) {
    u32 result = FRAMELIST_NONE;

    if (wk->flags & WIN_RECORD_FLAG_LIST_DONE) {
        return result;
    }
    WinRecord_ListInput(wk, TRUE);
    if (wk->list != NULL) {
        result = FrameList_Main(wk->list);
    }
    if (wk->flags & WIN_RECORD_FLAG_NO_SCROLL) {
        wk->flags |= WIN_RECORD_FLAG_LIST_DONE;
    }
    return result;
}

static int WinRecord_ListResult(WinRecordWork *wk, u32 result) {
    int handled = FALSE;

    switch (result) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        handled = 2;
        break;
    case -7:
    case -5:
        WinRecord_PressButton(wk, WIN_RECORD_BUTTON_UP);
        handled = TRUE;
        wk->seq = WIN_RECORD_SEQ_UP;
        break;
    case -6:
    case -4:
        WinRecord_PressButton(wk, WIN_RECORD_BUTTON_DOWN);
        handled = TRUE;
        wk->seq = WIN_RECORD_SEQ_DOWN;
        break;
    case -3:
    case -2:
        handled = TRUE;
        break;
    case -11:
    case -10:
        handled = TRUE;
        break;
    }
    if (handled == TRUE) {
        FrameList_SetShownRows(wk->list, 7);
    }
    return handled;
}

static void WinRecord_FreeList(WinRecordWork *wk) {
    if (wk->list != NULL) {
        FrameList_Free(wk->list);
        wk->list = NULL;
    }
}

static void WinRecord_ListPrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    WinRecordWork *wk = work;

    WinRecord_PrintItem(wk, index, window, wk->msgData[0]);
}

static void WinRecord_ListSelect(void *work, u32 index, BOOL moved) {
}

static void WinRecord_ListScroll(void *work, s16 delta) {
}

static void WinRecord_PrintItem(WinRecordWork *wk, u32 index, PrintWindow *window, MsgData *msgData) {
    PrintQueue *queue = FrameList_GetPrintQueue(wk->list);

    WinRecord_PrintItemLine(wk, window, queue, msgData, FrameList_GetValue(wk->list, index), index);
}

static void WinRecord_PrintItemAt(WinRecordWork *wk, PrintWindow *window, PrintQueue *queue, MsgData *msgData,
                                  u32 msgId, u16 color, int y, u32 index) {
    // The x of the tournament's name and of its wins
    static const u16 columns[] = { 0, 144 };
    StrBuf *strbuf;
    StrBuf *number;
    StrBuf *format;

    strbuf = GFL_MsgDataLoadStrbufNew(msgData, msgId);
    PrintWindow_Print(window, queue, columns[0], y, strbuf, wk->font, color);
    GFL_StrBufFree(strbuf);
    number = GFL_StrBufCreate(64, wk->heapId);
    GFL_WordSetFormatNumber(number, wk->wins[wk->items[index]], 4, 1, TRUE);
    func_0202437c(wk->wordSet, 0, number, 0, 1, 2);
    strbuf = GFL_StrBufCreate(64, wk->heapId);
    format = GFL_MsgDataLoadStrbufNew(wk->msgData[0], Bank0411_Text_Empty_2);
    GFL_WordSetFormatStrbuf(wk->wordSet, strbuf, format);
    PrintWindow_Print(window, queue, columns[1], y, strbuf, wk->font, color);
    GFL_StrBufFree(number);
    GFL_StrBufFree(format);
    GFL_StrBufFree(strbuf);
}

static void WinRecord_PrintItemLine(WinRecordWork *wk, PrintWindow *window, PrintQueue *queue, MsgData *msgData,
                                    u32 msgId, u32 index) {
    WinRecord_PrintItemAt(wk, window, queue, msgData, msgId, 0x39e0, 4, index);
}

static void WinRecord_UpdateScrollBar(WinRecordWork *wk) {
    ClActorPos pos;

    if (!(wk->flags & WIN_RECORD_FLAG_NO_SCROLL)) {
        func_0204c178(wk->scrollActors[0], &pos, 0);
        pos.y = FrameList_ClampBarPos(wk->list, pos.y);
        if (pos.y < 40) {
            pos.y = 40;
        } else if (pos.y > 152) {
            pos.y = 152;
        }
        func_0204c140(wk->scrollActors[0], &pos, 0);
    }
}

static void WinRecord_ScrollBG(WinRecordWork *wk) {
    // The speed of the scroll, and where it wraps around
    static const u16 scroll[] = { 0x40, 0x2000 };
    int y;

    wk->bgScroll += scroll[0];
    if (wk->bgScroll >= scroll[1]) {
        wk->bgScroll = 0;
    }
    y = -(u8)(wk->bgScroll >> 8);
    GFL_BGSysMoveBGReq(3, BG_MOVE_SET_Y, y);
    GFL_BGSysMoveBGReq(7, BG_MOVE_SET_Y, y);
}

static void WinRecord_ListInput(WinRecordWork *wk, BOOL updated) {
    BOOL touch = func_0203d554();
    int pos;

    if (wk->flags & WIN_RECORD_FLAG_NO_SCROLL) {
        if (updated == FALSE) {
            if (GCTX_HIDGetHeldKeys() & (PAD_KEY_LEFT | PAD_KEY_UP)) {
                FrameList_SetCursor(wk->list, 0);
            } else if (GCTX_HIDGetHeldKeys() & (PAD_KEY_RIGHT | PAD_KEY_DOWN)) {
                pos = wk->count - 1;
                if (pos < 0) {
                    pos = 0;
                } else if (pos >= WIN_RECORD_LIST_ROWS) {
                    pos = WIN_RECORD_LIST_ROWS - 1;
                }
                FrameList_SetCursor(wk->list, pos);
            }
        }
    } else {
        if (touch == TRUE) {
            func_0203d564(FALSE);
        }
        if (updated == FALSE) {
            if (GCTX_HIDGetHeldKeys() & (PAD_KEY_LEFT | PAD_KEY_UP)) {
                FrameList_SetCursor(wk->list, 0);
            } else if (GCTX_HIDGetHeldKeys() & (PAD_KEY_RIGHT | PAD_KEY_DOWN)) {
                FrameList_SetCursor(wk->list, WIN_RECORD_LIST_ROWS - 1);
            }
        } else {
            if (GCTX_HIDGetPressedKeys() & PAD_KEY_UP) {
                FrameList_SetCursor(wk->list, 0);
            } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_DOWN) {
                FrameList_SetCursor(wk->list, WIN_RECORD_LIST_ROWS - 1);
            }
        }
    }
}
