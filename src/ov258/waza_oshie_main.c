#include "types.h"
#include "app/p_status.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "nnsys/g2d.h"
#include "pml/move_reminder.h"
#include "pml/poke_graphic.h"
#include "pml/poke_party.h"
#include "pml/waza.h"
#include "system/app_keycursor.h"
#include "system/app_menu_common.h"
#include "system/app_taskmenu.h"
#include "system/bmp_menuwork.h"
#include "system/bmp_winframe.h"
#include "system/cursor_move.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/shinka_demo_btl_main.h"

// The move reminder's screen, which the move tutors' scripts show too: the Pokémon and its moves on the top screen,
// and on the touch screen the list of moves to pick from with the picked move's data. Replacing a move goes through
// the summary screen, which ends this screen's graphics and builds them again on return. The ROM has no name for this
// file; "waza_oshie_main.c" is a guess from "waza_oshie.c", the file of overlay 12 that allocates its parameter

#define HEAPID_WAZA_OSHIE 0x4f
// The screen's graphics
#define ARCID_WAZA_OSHIE 125

#define WINDOW_COUNT 16
#define ACTOR_COUNT 14
#define RESOURCE_COUNT 9
#define TYPE_ICON_COUNT 8
#define LIST_ROWS 4
#define MOVE_END 0xffff
// What the info windows are drawn for when the cursor's row has no move
#define NO_MOVE 0xfffffffe

// The windows
#define WINDOW_POWER_LABEL 2
#define WINDOW_ACCURACY_LABEL 3
#define WINDOW_POWER 4
#define WINDOW_ACCURACY 5
#define WINDOW_DESCRIPTION 6
#define WINDOW_MESSAGE 7
#define WINDOW_TEACH 9
#define WINDOW_LIST 11
#define WINDOW_TITLE 13
#define WINDOW_KNOWN_MOVES 14
#define WINDOW_POKEMON 15

// The actors
#define ACTOR_ARROW_DOWN 0
#define ACTOR_ARROW_UP 1
#define ACTOR_CURSOR 2
#define ACTOR_CATEGORY 3
#define ACTOR_LIST_TYPE 4
#define ACTOR_KNOWN_TYPE 8
#define ACTOR_POKEMON 12
#define ACTOR_RETURN 13

// The cursor's positions: the list's rows, then
#define POS_ARROW_DOWN 4
#define POS_ARROW_UP 5
#define POS_RETURN 6
#define POS_TEACH 7
#define POS_QUIT 8

// The buttons shown once a move is picked by touch
#define BUTTON_TEACH 0
#define BUTTON_QUIT 1

// The sequences of the process
enum {
    SEQ_FADE_WAIT,
    SEQ_SELECT,
    SEQ_MESSAGE,
    SEQ_YES_NO_INIT,
    SEQ_YES_NO,
    SEQ_SET_MOVE,
    SEQ_FORGOT,
    SEQ_STATUS_RESULT,
    SEQ_FADE_OUT,
    SEQ_END,
    SEQ_STATUS_FADE_OUT,
    SEQ_STATUS_CALL,
    SEQ_STATUS_RETURN,
    SEQ_DECIDE_WAIT,
    SEQ_QUIT,
    SEQ_MOVE_CHOSEN,
};

// The messages, which index a row of sWazaOshieMessages
enum {
    MSG_SELECT,
    MSG_LEARN,
    MSG_QUIT,
    MSG_LEARNED,
    MSG_REPLACE,
    // "1, 2, and... Ta-da!"
    MSG_FORGET,
    MSG_FORGOT,
    MSG_GIVE_UP,
    MSG_NOT_LEARNED,
    MSG_PAID,
    MSG_FORGET_CONFIRM,
    MESSAGE_COUNT,
};

// The questions, which index sWazaOshieYesNoFuncs
enum {
    YES_NO_LEARN,
    YES_NO_QUIT,
    YES_NO_REPLACE,
    YES_NO_GIVE_UP,
    YES_NO_FORGET,
};

// What SEQ_DECIDE_WAIT waits for
enum {
    DECIDE_ACTOR,
    DECIDE_BUTTON,
};

// How WazaOshie_PrintText places the text in its window
enum {
    ALIGN_LEFT,
    ALIGN_RIGHT,
    ALIGN_CENTER,
};

typedef struct {
    u32 type;
    BOOL dirty;
} WazaOshieTypeIcon;

// What SEQ_DECIDE_WAIT waits for: an actor's animation or a button's flash, and the sequence that follows
typedef struct {
    u8 unk00[6];
    u16 index;
    u16 seq;
    u16 type;
} WazaOshieDecide;

typedef struct {
    MoveReminderProcessData *param;
    BmpWin *windows[WINDOW_COUNT];
    MsgData *msgData;
    WordSet *wordSet;
    StrBuf *strbuf;
    StrBuf *ppLabel;
    StrBuf *ppFormat;
    ListMenuOption *moveList;
    u32 unk5C;
    Font *font;
    TCBExManager *tcbManager;
    TCB *vblankTcb;
    PrintStream *stream;
    u32 chars[RESOURCE_COUNT];
    u32 palettes[RESOURCE_COUNT];
    u32 cellAnims[RESOURCE_COUNT];
    u32 unkDC[RESOURCE_COUNT];
    ClActUnit *unit;
    ClActor *actors[ACTOR_COUNT];
    ArcTool *commonArc;
    // The type icons whose characters the next VBlank loads
    WazaOshieTypeIcon typeIcons[TYPE_ICON_COUNT];
    PStatusParam status;
    u8 unk1A8[8];
    u32 nextSeq;
    u32 unk1B4;
    u8 moveCount;
    u8 unk1B9;
    u8 yesNo;
    u8 unk1BB;
    u8 unk1BC;
    void *screenBuffer;
    NNSG2dScreenData *screen;
    CursorMove *cursor;
    u16 prevPos;
    u16 buttonsShown;
    WazaOshieDecide decide;
    AppTaskMenuRes *menuRes;
    AppTaskMenu *menu;
    AppTaskMenuItem buttonItems[2];
    AppTaskMenuItem yesNoItems[2];
    u32 unk214;
    PrintQueue *printQueue;
    KeyCursor *keyCursor;
    AppTaskMenuWin *buttons[2];
} WazaOshieWork;

typedef struct {
    u32 msg;
    u32 type;
} WazaOshieMenuItem;

typedef u32 (*WazaOshieYesNoFunc)(WazaOshieWork *work);

typedef struct {
    WazaOshieYesNoFunc yes;
    WazaOshieYesNoFunc no;
} WazaOshieYesNoFuncs;

typedef struct {
    u32 bg;
    u32 x;
    u32 y;
    u32 width;
    u32 height;
    u32 palette;
    u32 charOffset;
} WazaOshieWindowData;

typedef struct {
    s16 x;
    s16 y;
    u16 unk04;
    u16 sequence;
    u32 priority;
    u32 unk0C;
    u32 surface;
    // Indices of the work's resources
    u32 chars;
    u32 palette;
    u32 cellAnims;
    u32 unk20;
    u32 bgPriority;
    u32 unk28;
} WazaOshieActorData;

static BOOL WazaOshieProc_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WazaOshieProc_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WazaOshieProc_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void WazaOshie_InitMenu(WazaOshieWork *work);
static void WazaOshie_FreeMenu(WazaOshieWork *work);
static void WazaOshie_UpdateButtons(WazaOshieWork *work);
static void WazaOshie_InitScreen(WazaOshieWork *work);
static void WazaOshie_FreeScreen(WazaOshieWork *work);
static void WazaOshie_VBlank(TCB *tcb, void *data);
static void WazaOshie_SetVRAMBanks(void);
static void WazaOshie_InitBG(void);
static void WazaOshie_FreeBG(void);
static void WazaOshie_LoadBG(WazaOshieWork *work, ArcTool *arc);
static void WazaOshie_FreeScreenBuffer(WazaOshieWork *work);
static void WazaOshie_InitWindows(WazaOshieWork *work);
static void WazaOshie_FreeWindows(WazaOshieWork *work);
static void WazaOshie_InitMsg(WazaOshieWork *work);
static void WazaOshie_FreeMsg(WazaOshieWork *work);
static u32 WazaOshie_SeqFadeWait(WazaOshieWork *work);
static u32 WazaOshie_SeqSelect(WazaOshieWork *work);
static u32 WazaOshie_SeqMessage(WazaOshieWork *work);
static u32 WazaOshie_SeqYesNoInit(WazaOshieWork *work);
static u32 WazaOshie_SeqYesNo(WazaOshieWork *work);
static u32 WazaOshie_SeqSetMove(WazaOshieWork *work);
static u32 WazaOshie_SeqForgot(WazaOshieWork *work);
static u32 WazaOshie_SeqStatusResult(WazaOshieWork *work);
static u32 WazaOshie_SeqFadeOut(WazaOshieWork *work);
static void WazaOshie_DrawCursorMoveInfo(WazaOshieWork *work);
static void WazaOshie_PrintText(WazaOshieWork *work, u32 window, Font *font, u16 color, u32 align, u8 y);
static void WazaOshie_FormatNumber(WazaOshieWork *work, u32 msg, u32 number, u32 digits, u8 pad);
static void WazaOshie_DrawText(WazaOshieWork *work);
static u32 WazaOshie_CountMoves(WazaOshieWork *work);
static void WazaOshie_DrawListRow(WazaOshieWork *work, u8 scroll, u8 row);
static void WazaOshie_DrawList(WazaOshieWork *work);
static void WazaOshie_InitList(WazaOshieWork *work);
static void WazaOshie_FreeList(WazaOshieWork *work);
static void WazaOshie_DrawMoveInfo(WazaOshieWork *work, u32 move);
static void WazaOshie_FormatMessage(WazaOshieWork *work, u32 msg);
static u16 WazaOshie_GetCursorMove(WazaOshieWork *work);
static u16 WazaOshie_GetReplacedMove(WazaOshieWork *work);
static void WazaOshie_PrintMessage(WazaOshieWork *work, u32 msg);
static void WazaOshie_ClearMessage(WazaOshieWork *work);
static BOOL WazaOshie_StreamCallback(u32 event);
static u8 WazaOshie_CountKnownMoves(WazaOshieWork *work);
static u32 WazaOshie_LearnYes(WazaOshieWork *work);
static u32 WazaOshie_LearnNo(WazaOshieWork *work);
static u32 WazaOshie_QuitYes(WazaOshieWork *work);
static u32 WazaOshie_QuitNo(WazaOshieWork *work);
static u32 WazaOshie_ReplaceYes(WazaOshieWork *work);
static u32 WazaOshie_ReplaceNo(WazaOshieWork *work);
static u32 WazaOshie_GiveUpYes(WazaOshieWork *work);
static u32 WazaOshie_GiveUpNo(WazaOshieWork *work);
static u32 WazaOshie_ForgetYes(WazaOshieWork *work);
static u32 WazaOshie_ForgetNo(WazaOshieWork *work);
static void WazaOshie_InitActorSys(WazaOshieWork *work);
static void WazaOshie_FreeActors(WazaOshieWork *work);
static void WazaOshie_LoadActorResources(WazaOshieWork *work, ArcTool *arc);
static void WazaOshie_ReplaceChars(ArcTool *arc, u32 chars, u32 fileId, BOOL compressed, HeapID heapId);
static void WazaOshie_SetKnownMoveType(WazaOshieWork *work, u16 move, u16 index);
static void WazaOshie_LoadTypeIcons(WazaOshieWork *work);
static void WazaOshie_UpdateListTypes(WazaOshieWork *work);
static void WazaOshie_SetCategory(WazaOshieWork *work, u16 move);
static void WazaOshie_InitActors(WazaOshieWork *work, ArcTool *arc);
static void WazaOshie_SetCursorActor(WazaOshieWork *work, u8 pos, u32 unused);
static u32 WazaOshie_SeqStatusFadeOut(WazaOshieWork *work);
static u32 WazaOshie_SeqStatusCall(WazaOshieWork *work);
static u32 WazaOshie_SeqStatusReturn(WazaOshieWork *work);
static void WazaOshie_InitCursor(WazaOshieWork *work);
static void WazaOshie_FreeCursor(WazaOshieWork *work);
static void WazaOshie_CursorDummy(void *data, int pos, int prevPos);
static void WazaOshie_CursorMove(void *data, int pos, int prevPos);
static void WazaOshie_CursorTouch(void *data, int pos, int prevPos);
static void WazaOshie_ShowButtons(WazaOshieWork *work, BOOL show);
static BOOL WazaOshie_AreButtonsShown(WazaOshieWork *work);
static void WazaOshie_UpdateArrowPositions(WazaOshieWork *work);
static void WazaOshie_AnimateArrows(WazaOshieWork *work, int direction);
static void WazaOshie_InitArrows(WazaOshieWork *work);
static u32 WazaOshie_DecideActor(WazaOshieWork *work, u16 seq);
static u32 WazaOshie_DecideTeach(WazaOshieWork *work, u16 seq);
static u32 WazaOshie_DecideQuit(WazaOshieWork *work, u16 seq);
static u32 WazaOshie_SeqDecideWait(WazaOshieWork *work);
static u32 WazaOshie_SeqQuit(WazaOshieWork *work);
static u32 WazaOshie_SeqMoveChosen(WazaOshieWork *work);
static void WazaOshie_SetDimmed(BOOL dimmed);

// BmpWin_ClearScreenNow with the screen copied at the next VBlank
static inline void WazaOshie_ClearWindow(BmpWin *window) {
    BmpWin_ClearScreen(window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
}

const GameProcFunctions WazaOshieProcFunctions = { WazaOshieProc_Init, WazaOshieProc_Main, WazaOshieProc_Exit };

// The tables are declared in the order that gives the ROM's .rodata, with those of the actors and the cursor ahead
// of the functions that read them

// clang-format off
static const BGSysVRAMConfig sWazaOshieVRAMConfig = {
    GX_VRAM_BG_128_B,
    GX_VRAM_BGEXTPLTT_NONE,
    GX_VRAM_SUB_BG_128_C,
    GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_64_E,
    GX_VRAM_OBJEXTPLTT_NONE,
    GX_VRAM_SUB_OBJ_128_D,
    GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_A,
    GX_VRAM_TEXPLTT_0_F,
    GX_OBJVRAMMODE_CHAR_1D_64K,
    GX_OBJVRAMMODE_CHAR_1D_64K,
};

// clang-format on

static const WazaOshieMenuItem sWazaOshieButtonItems[2] = {
    { ShinkaDemoBtlMain_Text_Teach, 0 },
    { ShinkaDemoBtlMain_Text_Quit, APP_TASKMENU_ITEM_RETURN },
};

static const WazaOshieMenuItem sWazaOshieYesNoItems[2] = {
    { ShinkaDemoBtlMain_Text_Yes, 0 },
    { ShinkaDemoBtlMain_Text_No, 0 },
};

// clang-format off
static const WazaOshieWindowData sWazaOshieWindows[WINDOW_COUNT] = {
    { 2, 0, 25, 1, 1, 0, 0 },      { 6, 0, 25, 1, 1, 0, 0 },       { 6, 8, 6, 6, 2, 15, 43 },
    { 6, 19, 6, 8, 2, 15, 55 },    { 6, 14, 6, 3, 2, 15, 71 },     { 6, 27, 6, 3, 2, 15, 77 },
    { 6, 1, 0, 30, 6, 15, 83 },    { 4, 2, 19, 27, 4, 14, 263 },   { 6, 1, 0, 30, 3, 15, 371 },
    { 6, 17, 21, 6, 3, 15, 461 },  { 6, 25, 21, 6, 3, 15, 479 },   { 6, 8, 9, 21, 11, 15, 497 },
    { 4, 23, 13, 7, 4, 14, 728 },  { 2, 1, 0, 20, 3, 15, 1 },      { 2, 7, 5, 11, 16, 15, 61 },
    { 2, 21, 4, 9, 8, 15, 237 },
};

// clang-format on

// The messages, by the parameter's mode
static const u32 sWazaOshieMessages[2][MESSAGE_COUNT] = {
    {
        ShinkaDemoBtlMain_Text_Empty,
        ShinkaDemoBtlMain_Text_Empty_2,
        ShinkaDemoBtlMain_Text_Empty_3,
        ShinkaDemoBtlMain_Text_Learned,
        ShinkaDemoBtlMain_Text_WantsLearnMoveHowever,
        ShinkaDemoBtlMain_Text_12TaDa,
        ShinkaDemoBtlMain_Text_LearnedHowUse,
        ShinkaDemoBtlMain_Text_GiveUpLearningMove,
        ShinkaDemoBtlMain_Text_DidNotLearn,
        ShinkaDemoBtlMain_Text_Empty_4,
        ShinkaDemoBtlMain_Text_Empty_5,
    },
    {
        ShinkaDemoBtlMain_Text_WhichMoveShouldRemember,
        ShinkaDemoBtlMain_Text_ShouldMoveRemembered,
        ShinkaDemoBtlMain_Text_GiveUpTryingMake,
        ShinkaDemoBtlMain_Text_RememberedMove,
        ShinkaDemoBtlMain_Text_WantsRememberMoveBut,
        ShinkaDemoBtlMain_Text_12TaDa_2,
        ShinkaDemoBtlMain_Text_RememberedMove_2,
        ShinkaDemoBtlMain_Text_GiveUpMakingPokemon,
        ShinkaDemoBtlMain_Text_DidNotRememberMove,
        ShinkaDemoBtlMain_Text_HandedOverOneHeart,
        ShinkaDemoBtlMain_Text_OkForgetMove,
    },
};

// clang-format off
static const WazaOshieYesNoFuncs sWazaOshieYesNoFuncs[5] = {
    { WazaOshie_LearnYes, WazaOshie_LearnNo },
    { WazaOshie_QuitYes, WazaOshie_QuitNo },
    { WazaOshie_ReplaceYes, WazaOshie_ReplaceNo },
    { WazaOshie_GiveUpYes, WazaOshie_GiveUpNo },
    { WazaOshie_ForgetYes, WazaOshie_ForgetNo },
};

// clang-format on

static const ClActSysSetup sWazaOshieActorSysSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 9, 5, 6, 32, 16, 16 };

static BOOL WazaOshieProc_Init(GameProc *proc, u32 *state, void *param, void *work) {
    WazaOshieWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_WAZA_OSHIE, 0x50000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(WazaOshieWork), HEAPID_WAZA_OSHIE);
    sys_memset(wk, 0, sizeof(WazaOshieWork));
    wk->param = param;
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_WAZA_OSHIE);
    wk->tcbManager = GFL_TCBExMgrCreate(HEAPID_WAZA_OSHIE, HEAPID_WAZA_OSHIE, 32, 32);
    WazaOshie_InitScreen(wk);
    WazaOshie_SetCursorActor(wk, wk->param->cursorPos, 3);
    wk->nextSeq = SEQ_SELECT;
    func_02042ba8(TRUE, HEAPID_WAZA_OSHIE);
    return TRUE;
}

static BOOL WazaOshieProc_Main(GameProc *proc, u32 *state, void *param, void *work) {
    WazaOshieWork *wk = work;

    switch (*state) {
    case SEQ_FADE_WAIT:
        *state = WazaOshie_SeqFadeWait(wk);
        break;
    case SEQ_SELECT:
        *state = WazaOshie_SeqSelect(wk);
        break;
    case SEQ_MESSAGE:
        *state = WazaOshie_SeqMessage(wk);
        break;
    case SEQ_YES_NO_INIT:
        *state = WazaOshie_SeqYesNoInit(wk);
        break;
    case SEQ_YES_NO:
        *state = WazaOshie_SeqYesNo(wk);
        break;
    case SEQ_SET_MOVE:
        *state = WazaOshie_SeqSetMove(wk);
        break;
    case SEQ_FORGOT:
        *state = WazaOshie_SeqForgot(wk);
        break;
    case SEQ_STATUS_RESULT:
        *state = WazaOshie_SeqStatusResult(wk);
        break;
    case SEQ_FADE_OUT:
        *state = WazaOshie_SeqFadeOut(wk);
        break;
    case SEQ_END:
        return TRUE;
    case SEQ_STATUS_FADE_OUT:
        *state = WazaOshie_SeqStatusFadeOut(wk);
        break;
    case SEQ_STATUS_CALL:
        *state = WazaOshie_SeqStatusCall(wk);
        return FALSE;
    case SEQ_STATUS_RETURN:
        *state = WazaOshie_SeqStatusReturn(wk);
        return FALSE;
    case SEQ_DECIDE_WAIT:
        *state = WazaOshie_SeqDecideWait(wk);
        break;
    case SEQ_QUIT:
        *state = WazaOshie_SeqQuit(wk);
        break;
    case SEQ_MOVE_CHOSEN:
        *state = WazaOshie_SeqMoveChosen(wk);
        break;
    }
    WazaOshie_UpdateButtons(wk);
    func_0204b794();
    GFL_TCBExMgrUpdate(wk->tcbManager);
    func_02021a3c(wk->printQueue);
    return FALSE;
}

static BOOL WazaOshieProc_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    WazaOshieWork *wk = work;

    WazaOshie_FreeScreen(wk);
    GFL_TCBExMgrFree(wk->tcbManager);
    GFL_FontFree(wk->font);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_WAZA_OSHIE);
    return TRUE;
}

static void WazaOshie_InitMenu(WazaOshieWork *work) {
    int i;

    work->printQueue = func_02021998(HEAPID_WAZA_OSHIE);
    work->menuRes = AppTaskMenuRes_Create(4, 10, work->font, work->printQueue, HEAPID_WAZA_OSHIE);
    for (i = 0; i < 2; i++) {
        work->buttonItems[i].str = GFL_MsgDataLoadStrbufNew(work->msgData, sWazaOshieButtonItems[i].msg);
        work->buttonItems[i].color = PRINT_COLOR(14, 15, 3);
        work->buttonItems[i].type = sWazaOshieButtonItems[i].type;
    }
    for (i = 0; i < 2; i++) {
        work->yesNoItems[i].str = GFL_MsgDataLoadStrbufNew(work->msgData, sWazaOshieYesNoItems[i].msg);
        work->yesNoItems[i].color = PRINT_COLOR(14, 15, 3);
        work->yesNoItems[i].type = sWazaOshieYesNoItems[i].type;
    }
    work->buttons[BUTTON_TEACH] = NULL;
    work->buttons[BUTTON_QUIT] = NULL;
}

static void WazaOshie_FreeMenu(WazaOshieWork *work) {
    WazaOshie_ShowButtons(work, FALSE);
    GFL_StrBufFree(work->buttonItems[0].str);
    GFL_StrBufFree(work->buttonItems[1].str);
    GFL_StrBufFree(work->yesNoItems[0].str);
    GFL_StrBufFree(work->yesNoItems[1].str);
    AppTaskMenuRes_Free(work->menuRes);
    func_02021a18(work->printQueue);
}

static void WazaOshie_UpdateButtons(WazaOshieWork *work) {
    if (work->buttons[BUTTON_TEACH] != NULL) {
        AppTaskMenuWin_Update(work->buttons[BUTTON_TEACH]);
        AppTaskMenuWin_Update(work->buttons[BUTTON_QUIT]);
    }
}

static void WazaOshie_InitScreen(WazaOshieWork *work) {
    ArcTool *arc;

    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GX_SetVisiblePlane(0);
    GXS_SetVisiblePlane(0);
    arc = GFL_ArcSysCreateFileHandle(ARCID_WAZA_OSHIE, HEAPID_WAZA_OSHIE);
    WazaOshie_SetVRAMBanks();
    WazaOshie_InitBG();
    WazaOshie_LoadBG(work, arc);
    WazaOshie_InitActors(work, arc);
    WazaOshie_InitMsg(work);
    WazaOshie_InitWindows(work);
    WazaOshie_InitList(work);
    WazaOshie_DrawCursorMoveInfo(work);
    WazaOshie_DrawText(work);
    WazaOshie_InitCursor(work);
    WazaOshie_InitArrows(work);
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, HEAPID_WAZA_OSHIE);
    GFL_BGSysSetDisplayLayout(1);
    work->vblankTcb = GFL_VBlankTCBAdd(WazaOshie_VBlank, work, 16);
    GFL_ArcToolFree(arc);
    WazaOshie_InitMenu(work);
}

static void WazaOshie_FreeScreen(WazaOshieWork *work) {
    WazaOshie_FreeMenu(work);
    WazaOshie_FreeCursor(work);
    WazaOshie_FreeList(work);
    WazaOshie_FreeWindows(work);
    WazaOshie_FreeScreenBuffer(work);
    WazaOshie_FreeBG();
    WazaOshie_FreeMsg(work);
    WazaOshie_FreeActors(work);
    GFL_TCBRemove(work->vblankTcb);
}

static void WazaOshie_VBlank(TCB *tcb, void *data) {
    WazaOshie_LoadTypeIcons(data);
    GFL_BGSysUpdate();
    func_0204b7c8();
    OS_SetIrqCheckFlag(OS_IE_V_BLANK);
}

static void WazaOshie_SetVRAMBanks(void) {
    GFL_BGSysSetVRAMBanks(&sWazaOshieVRAMConfig);
}

static void WazaOshie_InitBG(void) {
    GFL_BGSysCreate(HEAPID_WAZA_OSHIE);
    BmpWin_InitAllocator(HEAPID_WAZA_OSHIE);
    {
        BGSysLCDConfig config = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };

        GFL_BGSysSetLCDConfig(&config);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xf800),
                          GX_BG_CHARBASE(0x00000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          2,
                          0,
                          FALSE };

        GFL_BGSysCreateBG(2, &setup, 0);
        GFL_BGSysClearScr(2);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xf000),
                          GX_BG_CHARBASE(0x08000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          3,
                          0,
                          FALSE };

        GFL_BGSysCreateBG(3, &setup, 0);
        GFL_BGSysClearScr(3);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xf800),
                          GX_BG_CHARBASE(0x00000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          0,
                          0,
                          FALSE };

        GFL_BGSysCreateBG(4, &setup, 0);
        GFL_BGSysClearScr(4);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xf000),
                          GX_BG_CHARBASE(0x18000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          2,
                          0,
                          FALSE };

        GFL_BGSysCreateBG(6, &setup, 0);
        GFL_BGSysClearScr(6);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xc000),
                          GX_BG_CHARBASE(0x08000),
                          0x4000,
                          GX_BG_EXTPLTT_01,
                          2,
                          0,
                          FALSE };

        GFL_BGSysCreateBG(5, &setup, 0);
        GFL_BGSysClearScr(5);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x1000,
                          0,
                          BGRES_512x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xe000),
                          GX_BG_CHARBASE(0x10000),
                          0x2000,
                          GX_BG_EXTPLTT_01,
                          3,
                          0,
                          FALSE };

        GFL_BGSysCreateBG(7, &setup, 0);
        GFL_BGSysClearScr(7);
    }
    GFL_BGSysSetBGEnabled(2, TRUE);
    GFL_BGSysSetBGEnabled(3, TRUE);
    GFL_BGSysSetBGEnabled(4, TRUE);
    GFL_BGSysSetBGEnabled(5, TRUE);
    GFL_BGSysSetBGEnabled(6, TRUE);
    GFL_BGSysSetBGEnabled(7, TRUE);
    GFL_BGSysClearCharCore(2, 0x20, 0, HEAPID_WAZA_OSHIE);
    GFL_BGSysClearCharCore(4, 0x20, 0, HEAPID_WAZA_OSHIE);
    GFL_BGSysClearCharCore(5, 0x20, 0, HEAPID_WAZA_OSHIE);
    GFL_BGSysClearCharCore(2, 0x20, 0, HEAPID_WAZA_OSHIE);
    GFL_BGSysClearCharCore(4, 0x20, 0, HEAPID_WAZA_OSHIE);
    G2_BlendNone();
    G2S_BlendNone();
}

static void WazaOshie_FreeBG(void) {
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ, FALSE);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(7);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysReleaseBG(3);
    GFL_BGSysReleaseBG(2);
    GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
    GFL_BGSysFree();
}

static void WazaOshie_LoadBG(WazaOshieWork *work, ArcTool *arc) {
    GFL_BGSysLoadArcNCGRStatic(arc, 1, 3, 0, 0, FALSE, HEAPID_WAZA_OSHIE);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 4, 3, 0, 0x800, FALSE, HEAPID_WAZA_OSHIE);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, 0, 0, 0, HEAPID_WAZA_OSHIE);
    GFL_BGSysLoadArcNCGRStatic(arc, 1, 7, 0, 0, FALSE, HEAPID_WAZA_OSHIE);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 2, 7, 0, 0x800, FALSE, HEAPID_WAZA_OSHIE);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, 4, 0, 0, HEAPID_WAZA_OSHIE);
    work->screenBuffer = GFL_G2DIOReadNSCRArc(arc, 3, FALSE, &work->screen, HEAPID_WAZA_OSHIE);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 14 * 0x20, 0x20, HEAPID_WAZA_OSHIE);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 15 * 0x20, 0x20, HEAPID_WAZA_OSHIE);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 15 * 0x20, 0x20, HEAPID_WAZA_OSHIE);
    LoadSysMsgBox(4, 1, 13, 0, HEAPID_WAZA_OSHIE);
}

static void WazaOshie_FreeScreenBuffer(WazaOshieWork *work) {
    GFL_HeapFree(work->screenBuffer);
}

static void WazaOshie_InitWindows(WazaOshieWork *work) {
    u32 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        work->windows[i] = BmpWin_CreateDynamic(sWazaOshieWindows[i].bg, sWazaOshieWindows[i].x, sWazaOshieWindows[i].y,
                                                sWazaOshieWindows[i].width, sWazaOshieWindows[i].height,
                                                sWazaOshieWindows[i].palette, 1);
        GFL_BitmapFill(BmpWin_GetBitmap(work->windows[i]), 0);
    }
    WazaOshie_ClearWindow(work->windows[WINDOW_TEACH]);
    work->keyCursor = KeyCursor_Create(15, TRUE, TRUE, HEAPID_WAZA_OSHIE);
}

static void WazaOshie_FreeWindows(WazaOshieWork *work) {
    u32 i;

    KeyCursor_Free(work->keyCursor);
    for (i = 0; i < WINDOW_COUNT; i++) {
        BmpWin_Free(work->windows[i]);
    }
    BmpWin_FreeAllocator();
}

static void WazaOshie_InitMsg(WazaOshieWork *work) {
    work->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_SHINKA_DEMO_BTL_MAIN, HEAPID_WAZA_OSHIE);
    work->wordSet = GFL_WordSetSystemCreateDefault(HEAPID_WAZA_OSHIE);
    work->strbuf = GFL_StrBufCreate(0x100, HEAPID_WAZA_OSHIE);
    work->ppLabel = GFL_MsgDataLoadStrbufNew(work->msgData, ShinkaDemoBtlMain_Text_Pp);
    work->ppFormat = GFL_MsgDataLoadStrbufNew(work->msgData, ShinkaDemoBtlMain_Text_Empty_12);
}

static void WazaOshie_FreeMsg(WazaOshieWork *work) {
    GFL_MsgDataFree(work->msgData);
    GFL_WordSetSystemFree(work->wordSet);
    GFL_StrBufFree(work->strbuf);
    GFL_StrBufFree(work->ppLabel);
    GFL_StrBufFree(work->ppFormat);
}

static u32 WazaOshie_SeqFadeWait(WazaOshieWork *work) {
    if (GFL_WipeIsFinished() == TRUE) {
        return work->nextSeq;
    }
    return SEQ_FADE_WAIT;
}

static u32 WazaOshie_SeqSelect(WazaOshieWork *work) {
    u32 pos;

    work->prevPos = CursorMove_GetPos(work->cursor);
    pos = CursorMove_Update(work->cursor);
    switch (pos) {
    case 0:
    case 1:
    case 2:
    case 3:
        if (!func_0203da48()) {
            if (work->param->scroll + pos < work->moveCount) {
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
                return WazaOshie_SeqMoveChosen(work);
            }
        } else {
            if (work->param->scroll + pos < work->moveCount) {
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
            }
        }
        break;
    case POS_RETURN:
    case POS_QUIT:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (WazaOshie_AreButtonsShown(work)) {
            return WazaOshie_DecideQuit(work, SEQ_QUIT);
        }
        return WazaOshie_DecideActor(work, SEQ_QUIT);
    case CURSOR_MOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        WazaOshie_ShowButtons(work, FALSE);
        return WazaOshie_DecideActor(work, SEQ_QUIT);
    case POS_TEACH:
        if (WazaOshie_AreButtonsShown(work)) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            return WazaOshie_DecideTeach(work, SEQ_MOVE_CHOSEN);
        }
        break;
    case CURSOR_MOVE_EDGE_UP:
        if (work->param->scroll != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            work->param->scroll--;
            WazaOshie_DrawMoveInfo(work, WazaOshie_GetCursorMove(work));
            WazaOshie_DrawList(work);
            WazaOshie_UpdateArrowPositions(work);
            WazaOshie_AnimateArrows(work, -1);
        }
        break;
    case CURSOR_MOVE_NONE:
    case CURSOR_MOVE_MOVED:
    case CURSOR_MOVE_CURSOR_ON:
        break;
    }
    return SEQ_SELECT;
}

static u32 WazaOshie_SeqMessage(WazaOshieWork *work) {
    u32 state = func_020223b4(work->stream);

    KeyCursor_Update(work->keyCursor, work->stream, work->windows[WINDOW_MESSAGE]);
    if (state == PRINT_STREAM_RUNNING) {
        if (GCTX_HIDGetHeldKeys() & PAD_BUTTON_A) {
            func_020223e0(work->stream, 0);
        }
    } else if (state == PRINT_STREAM_DONE) {
        func_020223cc(work->stream);
        return work->nextSeq;
    } else if (state == PRINT_STREAM_PAUSED) {
        if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da48()) {
            func_020223bc(work->stream);
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
        }
    }
    return SEQ_MESSAGE;
}

static u32 WazaOshie_SeqYesNoInit(WazaOshieWork *work) {
    AppTaskMenuInit init;

    init.heapId = HEAPID_WAZA_OSHIE;
    init.itemCount = 2;
    init.items = work->yesNoItems;
    init.posType = APP_TASKMENU_POS_TOP_LEFT;
    init.x = 21;
    init.y = 8;
    init.width = 10;
    init.height = 3;
    work->menu = AppTaskMenu_Create(&init, work->menuRes);
    WazaOshie_SetDimmed(TRUE);
    return SEQ_YES_NO;
}

static u32 WazaOshie_SeqYesNo(WazaOshieWork *work) {
    u32 seq = SEQ_YES_NO;

    if (AppTaskMenu_IsFlashFinished(work->menu)) {
        if (AppTaskMenu_GetCursorPos(work->menu) == 0) {
            seq = sWazaOshieYesNoFuncs[work->yesNo].yes(work);
        } else {
            seq = sWazaOshieYesNoFuncs[work->yesNo].no(work);
        }
        WazaOshie_SetDimmed(FALSE);
    }
    AppTaskMenu_Update(work->menu);
    if (seq != SEQ_YES_NO) {
        AppTaskMenu_Free(work->menu);
    }
    return seq;
}

static u32 WazaOshie_SeqSetMove(WazaOshieWork *work) {
    PokeParty_SetParam(work->param->pkm, PKM_PARAM_MOVE1 + work->param->slot, WazaOshie_GetCursorMove(work));
    PokeParty_SetParam(work->param->pkm, PKM_PARAM_MOVE1_PP_UP + work->param->slot, 0);
    PokeParty_SetParam(work->param->pkm, PKM_PARAM_MOVE1_PP + work->param->slot,
                       PML_MoveGetMaxPP(WazaOshie_GetCursorMove(work), 0));
    work->param->status = MOVE_REMINDER_STATUS_LEARNED;
    return SEQ_FADE_OUT;
}

static u32 WazaOshie_SeqForgot(WazaOshieWork *work) {
    WazaOshie_PrintMessage(work, MSG_FORGOT);
    work->nextSeq = SEQ_SET_MOVE;
    return SEQ_MESSAGE;
}

static u32 WazaOshie_SeqStatusResult(WazaOshieWork *work) {
    if (work->param->slot < 4) {
        WazaOshie_PrintMessage(work, MSG_FORGET_CONFIRM);
        work->yesNo = YES_NO_FORGET;
    } else {
        WazaOshie_PrintMessage(work, MSG_GIVE_UP);
        work->yesNo = YES_NO_GIVE_UP;
    }
    work->nextSeq = SEQ_YES_NO_INIT;
    return SEQ_MESSAGE;
}

static u32 WazaOshie_SeqFadeOut(WazaOshieWork *work) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, HEAPID_WAZA_OSHIE);
    work->nextSeq = SEQ_END;
    return SEQ_FADE_WAIT;
}

static void WazaOshie_DrawCursorMoveInfo(WazaOshieWork *work) {
    u32 move = WazaOshie_GetCursorMove(work);

    if (move != MOVE_END) {
        WazaOshie_DrawMoveInfo(work, move);
    } else {
        WazaOshie_DrawMoveInfo(work, NO_MOVE);
    }
    WazaOshie_UpdateListTypes(work);
}

// Draws the work's string buffer into a window
static void WazaOshie_PrintText(WazaOshieWork *work, u32 window, Font *font, u16 color, u32 align, u8 y) {
    u8 x;
    u8 width;
    u8 windowWidth;

    switch (align) {
    case ALIGN_LEFT:
        x = 0;
        break;
    case ALIGN_RIGHT:
        width = GFL_FontGetBlockWidth(work->strbuf, font, 0);
        windowWidth = BmpWin_GetSizeX(work->windows[window]) * 8;
        x = windowWidth - width;
        break;
    case ALIGN_CENTER:
        width = GFL_FontGetBlockWidth(work->strbuf, font, 0);
        windowWidth = BmpWin_GetSizeX(work->windows[window]) * 8;
        x = (windowWidth - width) / 2;
        break;
    }
    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(work->windows[window]), x, y, work->strbuf, font, color);
}

// Formats a message with a number into the work's string buffer
static void WazaOshie_FormatNumber(WazaOshieWork *work, u32 msg, u32 number, u32 digits, u8 pad) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(work->msgData, msg);

    WordSetNumber(work->wordSet, 0, number, digits, pad, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, work->strbuf, str);
    GFL_StrBufFree(str);
}

// Draws the labels, and on the top screen the Pokémon's name, level and moves
static void WazaOshie_DrawText(WazaOshieWork *work) {
    StrBuf *str;
    MsgData *moveNames;
    BoxPkm *boxPkm;
    u32 i;
    u16 move;

    GFL_MsgDataLoadStrbuf(work->msgData, ShinkaDemoBtlMain_Text_Teach, work->strbuf);
    WazaOshie_PrintText(work, WINDOW_TEACH, work->font, PRINT_COLOR(15, 2, 0), ALIGN_CENTER, 4);
    GFL_MsgDataLoadStrbuf(work->msgData, ShinkaDemoBtlMain_Text_Power, work->strbuf);
    WazaOshie_PrintText(work, WINDOW_POWER_LABEL, work->font, PRINT_COLOR(1, 2, 0), ALIGN_LEFT, 0);
    BmpWin_Transfer(work->windows[WINDOW_POWER_LABEL]);
    GFL_MsgDataLoadStrbuf(work->msgData, ShinkaDemoBtlMain_Text_Accuracy, work->strbuf);
    WazaOshie_PrintText(work, WINDOW_ACCURACY_LABEL, work->font, PRINT_COLOR(1, 2, 0), ALIGN_LEFT, 0);
    BmpWin_Transfer(work->windows[WINDOW_ACCURACY_LABEL]);

    str = GFL_StrBufCreate(0x100, HEAPID_WAZA_OSHIE);
    GFL_MsgDataLoadStrbuf(work->msgData, ShinkaDemoBtlMain_Text_LearnedMoves, str);
    loadPokemonNicknameToStrbuf(work->wordSet, 0, work->param->pkm);
    GFL_WordSetFormatStrbuf(work->wordSet, work->strbuf, str);
    WazaOshie_PrintText(work, WINDOW_TITLE, work->font, PRINT_COLOR(15, 2, 0), ALIGN_LEFT, 4);
    BmpWin_Transfer(work->windows[WINDOW_TITLE]);

    moveNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_MOVE_NAMES, HEAPID_WAZA_OSHIE);
    boxPkm = func_0201d624(work->param->pkm);
    for (i = 0; i < 4; i++) {
        move = PML_PkmGetParam(boxPkm, PKM_PARAM_MOVE1 + i, NULL);
        if (move == 0) {
            func_0204c124(work->actors[ACTOR_KNOWN_TYPE + i], FALSE);
        } else {
            func_0204c124(work->actors[ACTOR_KNOWN_TYPE + i], TRUE);
            WazaOshie_SetKnownMoveType(work, move, i);
            GFL_MsgDataLoadStrbuf(moveNames, move, work->strbuf);
            WazaOshie_PrintText(work, WINDOW_KNOWN_MOVES, work->font, PRINT_COLOR(15, 2, 0), ALIGN_LEFT, i * 32);
            GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(work->windows[WINDOW_KNOWN_MOVES]), 8, i * 32 + 16,
                                           work->ppLabel, work->font, PRINT_COLOR(1, 2, 0));
            BmpWin_Transfer(work->windows[WINDOW_KNOWN_MOVES]);
            WordSetNumber(work->wordSet, 0, PML_PkmGetParam(boxPkm, PKM_PARAM_MOVE1_PP + i, NULL), 2, 1, TRUE);
            WordSetNumber(work->wordSet, 1, PML_PkmGetParam(boxPkm, PKM_PARAM_MOVE1_MAX_PP + i, NULL), 2, 0, TRUE);
            GFL_WordSetFormatStrbuf(work->wordSet, work->strbuf, work->ppFormat);
            GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(work->windows[WINDOW_KNOWN_MOVES]), 32, i * 32 + 16,
                                           work->strbuf, work->font, PRINT_COLOR(1, 2, 0));
            BmpWin_Transfer(work->windows[WINDOW_KNOWN_MOVES]);
        }
    }
    BmpWin_Transfer(work->windows[WINDOW_KNOWN_MOVES]);

    GFL_MsgDataLoadStrbuf(work->msgData, ShinkaDemoBtlMain_Text_Empty_16, str);
    setPartyPokemonSpeciesNameToStrbuf(work->wordSet, 0, work->param->pkm);
    GFL_WordSetFormatStrbuf(work->wordSet, work->strbuf, str);
    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(work->windows[WINDOW_POKEMON]), 0, 0, work->strbuf, work->font,
                                   PRINT_COLOR(1, 2, 0));
    BmpWin_Transfer(work->windows[WINDOW_POKEMON]);
    GFL_MsgDataLoadStrbuf(work->msgData, ShinkaDemoBtlMain_Text_Lv, str);
    WordSetNumber(work->wordSet, 0, PML_PkmGetParam(boxPkm, PKM_PARAM_LEVEL, NULL), 3, 1, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, work->strbuf, str);
    WazaOshie_PrintText(work, WINDOW_POKEMON, work->font, PRINT_COLOR(1, 2, 0), ALIGN_RIGHT, 16);
    BmpWin_Transfer(work->windows[WINDOW_POKEMON]);

    GFL_MsgDataFree(moveNames);
    GFL_StrBufFree(str);
}

static u32 WazaOshie_CountMoves(WazaOshieWork *work) {
    u32 i;

    for (i = 0; i < 256; i++) {
        if (work->param->moves[i] == MOVE_END) {
            break;
        }
    }
    return i;
}

static void WazaOshie_DrawListRow(WazaOshieWork *work, u8 scroll, u8 row) {
    int y = row * 24;
    int index = scroll + row;
    u32 pp;

    if (index >= work->moveCount) {
        return;
    }
    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(work->windows[WINDOW_LIST]), 0, y, work->moveList[index].text,
                                   work->font, PRINT_COLOR(15, 2, 0));
    BmpWin_Transfer(work->windows[WINDOW_LIST]);
    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(work->windows[WINDOW_LIST]), 96, y, work->ppLabel, work->font,
                                   PRINT_COLOR(1, 2, 0));
    BmpWin_Transfer(work->windows[WINDOW_LIST]);
    pp = PML_MoveGetMaxPP(work->moveList[index].value, 0);
    WordSetNumber(work->wordSet, 0, pp, 2, 1, TRUE);
    WordSetNumber(work->wordSet, 1, pp, 2, 0, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, work->strbuf, work->ppFormat);
    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(work->windows[WINDOW_LIST]), 120, y, work->strbuf, work->font,
                                   PRINT_COLOR(1, 2, 0));
    BmpWin_Transfer(work->windows[WINDOW_LIST]);
}

static void WazaOshie_DrawList(WazaOshieWork *work) {
    u32 i;

    GFL_BitmapFill(BmpWin_GetBitmap(work->windows[WINDOW_LIST]), 0);
    for (i = 0; i < LIST_ROWS; i++) {
        WazaOshie_DrawListRow(work, work->param->scroll, i);
    }
    BmpWin_Transfer(work->windows[WINDOW_LIST]);
    GFL_BGSysQueueScrLoad(7);
    WazaOshie_UpdateListTypes(work);
}

static void WazaOshie_InitList(WazaOshieWork *work) {
    MsgData *moveNames;
    u32 i;

    work->moveCount = WazaOshie_CountMoves(work);
    work->moveList = ListMenuCore_CreateOptionList(work->moveCount, HEAPID_WAZA_OSHIE);
    moveNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_MOVE_NAMES, HEAPID_WAZA_OSHIE);
    for (i = 0; i < work->moveCount; i++) {
        if (work->param->moves[i] != MOVE_END) {
            ListMenuCore_AppendMsgOption(work->moveList, moveNames, work->param->moves[i], work->param->moves[i],
                                         HEAPID_WAZA_OSHIE);
        } else {
            ListMenuCore_AppendMsgOption(work->moveList, work->msgData, ShinkaDemoBtlMain_Text_Empty_13, -2,
                                         HEAPID_WAZA_OSHIE);
            break;
        }
    }
    GFL_MsgDataFree(moveNames);
    work->unk1BB = 0;
    work->unk1BC = work->param->cursorPos;
    WazaOshie_DrawList(work);
}

static void WazaOshie_FreeList(WazaOshieWork *work) {
    ListMenuCore_FreeOptionList(work->moveList);
}

// Draws a move's power, accuracy, description and category, or clears them for NO_MOVE
static void WazaOshie_DrawMoveInfo(WazaOshieWork *work, u32 move) {
    u32 power;
    u32 accuracy;
    MsgData *descriptions;

    GFL_BitmapFill(BmpWin_GetBitmap(work->windows[WINDOW_DESCRIPTION]), 0);
    GFL_BitmapFill(BmpWin_GetBitmap(work->windows[WINDOW_POWER]), 0);
    GFL_BitmapFill(BmpWin_GetBitmap(work->windows[WINDOW_ACCURACY]), 0);
    func_0204c124(work->actors[ACTOR_CATEGORY], FALSE);
    if (move != NO_MOVE) {
        power = PML_MoveGetParam(move, MOVE_PARAM_POWER);
        if (power <= 1) {
            GFL_MsgDataLoadStrbuf(work->msgData, ShinkaDemoBtlMain_Text_Empty_14, work->strbuf);
        } else {
            WazaOshie_FormatNumber(work, ShinkaDemoBtlMain_Text_Empty_10, power, 3, 0);
        }
        WazaOshie_PrintText(work, WINDOW_POWER, work->font, PRINT_COLOR(1, 2, 0), ALIGN_RIGHT, 0);
        accuracy = PML_MoveGetParam(move, MOVE_PARAM_ACCURACY);
        if (accuracy == 0 || PML_MoveIsAlwaysHit(move)) {
            GFL_MsgDataLoadStrbuf(work->msgData, ShinkaDemoBtlMain_Text_Empty_14, work->strbuf);
        } else {
            WazaOshie_FormatNumber(work, ShinkaDemoBtlMain_Text_Empty_11, accuracy, 3, 0);
        }
        WazaOshie_PrintText(work, WINDOW_ACCURACY, work->font, PRINT_COLOR(1, 2, 0), ALIGN_RIGHT, 0);
        descriptions = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_13, HEAPID_WAZA_OSHIE);
        GFL_MsgDataLoadStrbuf(descriptions, move, work->strbuf);
        WazaOshie_PrintText(work, WINDOW_DESCRIPTION, work->font, PRINT_COLOR(1, 2, 0), ALIGN_LEFT, 0);
        GFL_MsgDataFree(descriptions);
        WazaOshie_SetCategory(work, move);
        func_0204c124(work->actors[ACTOR_CATEGORY], TRUE);
        BmpWin_Transfer(work->windows[WINDOW_POWER_LABEL]);
        BmpWin_Transfer(work->windows[WINDOW_ACCURACY_LABEL]);
    } else {
        WazaOshie_ClearWindow(work->windows[WINDOW_POWER_LABEL]);
        WazaOshie_ClearWindow(work->windows[WINDOW_ACCURACY_LABEL]);
    }
    BmpWin_Transfer(work->windows[WINDOW_DESCRIPTION]);
    BmpWin_Transfer(work->windows[WINDOW_POWER]);
    BmpWin_Transfer(work->windows[WINDOW_ACCURACY]);
}

// Formats one of the mode's messages into the work's string buffer
static void WazaOshie_FormatMessage(WazaOshieWork *work, u32 msg) {
    StrBuf *str;

    switch (msg) {
    case MSG_SELECT:
        loadPokemonNicknameToStrbuf(work->wordSet, 0, work->param->pkm);
        break;
    case MSG_LEARN:
        loadMoveNameToStrbuf(work->wordSet, 1, WazaOshie_GetCursorMove(work));
        break;
    case MSG_QUIT:
        loadPokemonNicknameToStrbuf(work->wordSet, 0, work->param->pkm);
        break;
    case MSG_LEARNED:
        loadPokemonNicknameToStrbuf(work->wordSet, 0, work->param->pkm);
        loadMoveNameToStrbuf(work->wordSet, 1, WazaOshie_GetCursorMove(work));
        break;
    case MSG_REPLACE:
        loadPokemonNicknameToStrbuf(work->wordSet, 0, work->param->pkm);
        loadMoveNameToStrbuf(work->wordSet, 1, WazaOshie_GetCursorMove(work));
        break;
    case MSG_FORGET:
        loadPokemonNicknameToStrbuf(work->wordSet, 0, work->param->pkm);
        loadMoveNameToStrbuf(work->wordSet, 1, WazaOshie_GetReplacedMove(work));
        break;
    case MSG_FORGOT:
        loadPokemonNicknameToStrbuf(work->wordSet, 0, work->param->pkm);
        loadMoveNameToStrbuf(work->wordSet, 1, WazaOshie_GetCursorMove(work));
        break;
    case MSG_GIVE_UP:
        loadMoveNameToStrbuf(work->wordSet, 1, WazaOshie_GetCursorMove(work));
        break;
    case MSG_NOT_LEARNED:
        loadPokemonNicknameToStrbuf(work->wordSet, 0, work->param->pkm);
        loadMoveNameToStrbuf(work->wordSet, 1, WazaOshie_GetCursorMove(work));
        break;
    case MSG_PAID:
        copyVarForText(work->wordSet, 2, work->param->playerInfo);
        break;
    case MSG_FORGET_CONFIRM:
        loadMoveNameToStrbuf(work->wordSet, 0, WazaOshie_GetReplacedMove(work));
        break;
    }
    str = GFL_MsgDataLoadStrbufNew(work->msgData, sWazaOshieMessages[work->param->mode][msg]);
    GFL_WordSetFormatStrbuf(work->wordSet, work->strbuf, str);
    GFL_StrBufFree(str);
}

static u16 WazaOshie_GetCursorMove(WazaOshieWork *work) {
    return work->param->moves[work->param->scroll + work->param->cursorPos];
}

static u16 WazaOshie_GetReplacedMove(WazaOshieWork *work) {
    return PokeParty_GetParam(work->param->pkm, PKM_PARAM_MOVE1 + work->param->slot, NULL);
}

static void WazaOshie_PrintMessage(WazaOshieWork *work, u32 msg) {
    GFL_BitmapFill(BmpWin_GetBitmap(work->windows[WINDOW_MESSAGE]), 15);
    BmpWin_DrawFrame(work->windows[WINDOW_MESSAGE], 0, 1, 13);
    WazaOshie_FormatMessage(work, msg);
    work->stream = func_02022294(work->windows[WINDOW_MESSAGE], 0, 0, work->strbuf, work->font, func_02017bcc(),
                                 work->tcbManager, 0, HEAPID_WAZA_OSHIE, 0xffff, WazaOshie_StreamCallback);
    BmpWin_TransferNow(work->windows[WINDOW_MESSAGE]);
}

static void WazaOshie_ClearMessage(WazaOshieWork *work) {
    BmpWin_ClearFrame(work->windows[WINDOW_MESSAGE], 2);
    WazaOshie_ClearWindow(work->windows[WINDOW_MESSAGE]);
}

// The messages' events: wait for the sounds, play the forgetting sound, and play the fanfare and wait for it
static BOOL WazaOshie_StreamCallback(u32 event) {
    switch (event) {
    case 1:
        return GFL_SndPlayerIsActiveAny();
    case 2:
        return GFL_SndPlayerIsActiveAny();
    case 3:
        GFL_SndSEPlay(SEQ_SE_KON);
        break;
    case 4:
        GFL_SndBGMSetPaused(TRUE);
        GFL_SndBGMPush();
        GFL_SndBGMPlay(SEQ_ME_LVUP, 0xffff);
        break;
    case 5:
        if (!GFL_SndBGMIsPlaying()) {
            GFL_SndBGMPop();
            GFL_SndBGMSetPaused(FALSE);
            break;
        }
        return TRUE;
    }
    return FALSE;
}

static u8 WazaOshie_CountKnownMoves(WazaOshieWork *work) {
    u8 i;

    for (i = 0; i < 4; i++) {
        if (PokeParty_GetParam(work->param->pkm, PKM_PARAM_MOVE1 + i, NULL) == 0) {
            break;
        }
    }
    return i;
}

static u32 WazaOshie_LearnYes(WazaOshieWork *work) {
    WazaOshie_PrintMessage(work, MSG_LEARNED);
    work->param->slot = WazaOshie_CountKnownMoves(work);
    work->nextSeq = SEQ_SET_MOVE;
    return SEQ_MESSAGE;
}

static u32 WazaOshie_LearnNo(WazaOshieWork *work) {
    WazaOshie_ClearMessage(work);
    return SEQ_SELECT;
}

static u32 WazaOshie_QuitYes(WazaOshieWork *work) {
    work->param->status = MOVE_REMINDER_STATUS_CANCELLED;
    return SEQ_FADE_OUT;
}

static u32 WazaOshie_QuitNo(WazaOshieWork *work) {
    WazaOshie_ClearMessage(work);
    return SEQ_SELECT;
}

static u32 WazaOshie_ReplaceYes(WazaOshieWork *work) {
    work->nextSeq = SEQ_STATUS_FADE_OUT;
    return SEQ_FADE_WAIT;
}

static u32 WazaOshie_ReplaceNo(WazaOshieWork *work) {
    WazaOshie_PrintMessage(work, MSG_GIVE_UP);
    work->yesNo = YES_NO_GIVE_UP;
    work->nextSeq = SEQ_YES_NO_INIT;
    return SEQ_MESSAGE;
}

static u32 WazaOshie_GiveUpYes(WazaOshieWork *work) {
    WazaOshie_PrintMessage(work, MSG_NOT_LEARNED);
    work->nextSeq = SEQ_FADE_OUT;
    work->param->status = MOVE_REMINDER_STATUS_CANCELLED;
    return SEQ_MESSAGE;
}

static u32 WazaOshie_GiveUpNo(WazaOshieWork *work) {
    WazaOshie_PrintMessage(work, MSG_REPLACE);
    work->yesNo = YES_NO_REPLACE;
    work->nextSeq = SEQ_YES_NO_INIT;
    return SEQ_MESSAGE;
}

static u32 WazaOshie_ForgetYes(WazaOshieWork *work) {
    WazaOshie_PrintMessage(work, MSG_FORGET);
    work->nextSeq = SEQ_FORGOT;
    return SEQ_MESSAGE;
}

static u32 WazaOshie_ForgetNo(WazaOshieWork *work) {
    work->nextSeq = SEQ_STATUS_FADE_OUT;
    return SEQ_FADE_WAIT;
}

static void WazaOshie_InitActorSys(WazaOshieWork *work) {
    ClActSys_Create(&sWazaOshieActorSysSetup, &sWazaOshieVRAMConfig, HEAPID_WAZA_OSHIE);
    work->unit = func_0204bf1c(ACTOR_COUNT, 0, HEAPID_WAZA_OSHIE);
    func_0204c028(work->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void WazaOshie_FreeActors(WazaOshieWork *work) {
    u32 i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        func_0204c108(work->actors[i]);
    }
    func_0204bf98(work->unit);
    func_0204b758();
    GFL_ArcToolFree(work->commonArc);
}

static void WazaOshie_LoadActorResources(WazaOshieWork *work, ArcTool *arc) {
    ArcTool *commonArc;
    ArcTool *pokeArc;
    BoxPkm *boxPkm;
    BOOL wasEncrypted;
    u32 i;

    commonArc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_WAZA_OSHIE);
    work->chars[0] = func_0204b81c(commonArc, func_0202d814(), FALSE, CLACT_VRAM_SUB, HEAPID_WAZA_OSHIE);
    work->chars[1] = func_0204b81c(arc, 17, FALSE, CLACT_VRAM_SUB, HEAPID_WAZA_OSHIE);
    work->chars[2] = func_0204b81c(arc, 26, FALSE, CLACT_VRAM_SUB, HEAPID_WAZA_OSHIE);
    for (i = 4; i < 8; i++) {
        work->chars[i] = func_0204b81c(commonArc, func_0202d7f4(0), FALSE, CLACT_VRAM_MAIN, HEAPID_WAZA_OSHIE);
    }
    work->chars[3] = func_0204b81c(arc, 23, FALSE, CLACT_VRAM_SUB, HEAPID_WAZA_OSHIE);
    work->palettes[0] = func_0204bbb8(commonArc, func_0202d810(), CLACT_VRAM_SUB, 0, 0, 4, HEAPID_WAZA_OSHIE);
    work->palettes[1] = func_0204bbb8(arc, 5, CLACT_VRAM_SUB, 4 * 0x20, 0, 1, HEAPID_WAZA_OSHIE);
    work->palettes[2] = func_0204bbb8(commonArc, func_0202d7e4(), CLACT_VRAM_SUB, 6 * 0x20, 0, 3, HEAPID_WAZA_OSHIE);
    work->palettes[3] = func_0204bba0(commonArc, func_0202d7e4(), CLACT_VRAM_MAIN, 0, HEAPID_WAZA_OSHIE);
    work->cellAnims[0] = func_0204bde0(commonArc, func_0202d818(1), func_0202d81c(1), HEAPID_WAZA_OSHIE);
    work->cellAnims[1] = func_0204bde0(arc, 7, 8, HEAPID_WAZA_OSHIE);
    work->cellAnims[2] = func_0204bde0(commonArc, func_0202d7f8(1), func_0202d7fc(1), HEAPID_WAZA_OSHIE);
    work->cellAnims[3] = func_0204bde0(arc, 24, 25, HEAPID_WAZA_OSHIE);
    work->cellAnims[4] = func_0204bde0(arc, 27, 28, HEAPID_WAZA_OSHIE);
    GFL_ArcToolFree(commonArc);

    pokeArc = MakePokeGraArcHandle(HEAPID_WAZA_OSHIE);
    boxPkm = func_0201d620(work->param->pkm);
    wasEncrypted = PML_PkmDecrypt(boxPkm);
    work->chars[8] = PokeGra_LoadClActCharsByBoxData(pokeArc, boxPkm, 0, CLACT_VRAM_MAIN, HEAPID_WAZA_OSHIE);
    work->palettes[4] =
        PokeGra_LoadClActPaletteByBoxData(pokeArc, boxPkm, 0, CLACT_VRAM_MAIN, 4 * 0x20, HEAPID_WAZA_OSHIE);
    work->cellAnims[5] = PokeGra_LoadClActCellAnimsByBoxData(boxPkm, 0, 1, CLACT_VRAM_MAIN, HEAPID_WAZA_OSHIE);
    PML_PkmReEncrypt(boxPkm, wasEncrypted);
    GFL_ArcToolFree(pokeArc);
}

static void WazaOshie_ReplaceChars(ArcTool *arc, u32 chars, u32 fileId, BOOL compressed, HeapID heapId) {
    NNSG2dCharacterData *character;
    void *buffer = GFL_G2DIOReadOBJNCGRArc(arc, fileId, compressed, &character, heapId);

    func_0204ba40(chars, character);
    GFL_HeapFree(buffer);
}

// Gives the icon of one of the Pokémon's moves its type, loaded at the next VBlank
static void WazaOshie_SetKnownMoveType(WazaOshieWork *work, u16 move, u16 index) {
    u32 type = PML_MoveGetParam(move, MOVE_PARAM_TYPE);

    work->typeIcons[index].type = type;
    work->typeIcons[index].dirty = TRUE;
    func_0204c378(work->actors[ACTOR_KNOWN_TYPE + index], func_0202d7e8(type), 1);
}

static void WazaOshie_LoadTypeIcons(WazaOshieWork *work) {
    int i;

    for (i = 0; i < TYPE_ICON_COUNT; i++) {
        if (work->typeIcons[i].dirty) {
            WazaOshie_ReplaceChars(work->commonArc, work->chars[4 + i], func_0202d7f4(work->typeIcons[i].type), FALSE,
                                   HEAPID_WAZA_OSHIE);
            work->typeIcons[i].dirty = FALSE;
        }
    }
}

// Places the list's type icons and sets them to the types of the moves shown
static void WazaOshie_UpdateListTypes(WazaOshieWork *work) {
    u32 i;
    ClActorPos pos;
    u32 type;

    for (i = 0; i < LIST_ROWS; i++) {
        pos.x = 47;
        pos.y = i * 24 + 80;
        func_0204c140(work->actors[ACTOR_LIST_TYPE + i], &pos, CLACT_SURFACE_SUB);
        if (work->param->scroll + i >= work->moveCount) {
            func_0204c124(work->actors[ACTOR_LIST_TYPE + i], FALSE);
        } else {
            type = PML_MoveGetParam(work->param->moves[work->param->scroll + i], MOVE_PARAM_TYPE);
            func_0204c124(work->actors[ACTOR_LIST_TYPE + i], TRUE);
            func_0204c488(work->actors[ACTOR_LIST_TYPE + i], type);
        }
    }
}

static void WazaOshie_SetCategory(WazaOshieWork *work, u16 move) {
    func_0204c488(work->actors[ACTOR_CATEGORY], PML_MoveGetParam(move, MOVE_PARAM_CATEGORY));
}

static const WazaOshieActorData sWazaOshieActors[ACTOR_COUNT] = {
    { 8, 168, 0, 2, 1, 2, CLACT_SURFACE_SUB, 0, 0, 0, 0, 2, 0 },
    { 40, 168, 0, 3, 1, 2, CLACT_SURFACE_SUB, 0, 0, 0, 0, 2, 0 },
    { 130, 84, 0, 0, 0, 3, CLACT_SURFACE_SUB, 1, 1, 1, 1, 2, 0 },
    { 33, 56, 0, 0, 0, 6, CLACT_SURFACE_SUB, 2, 2, 4, 4, 2, 0 },
    { 47, 80, 0, 0, 1, 6, CLACT_SURFACE_SUB, 3, 2, 3, 3, 2, 0 },
    { 47, 104, 0, 0, 1, 6, CLACT_SURFACE_SUB, 3, 2, 3, 3, 2, 0 },
    { 47, 128, 0, 0, 1, 6, CLACT_SURFACE_SUB, 3, 2, 3, 3, 2, 0 },
    { 47, 152, 0, 0, 1, 6, CLACT_SURFACE_SUB, 3, 2, 3, 3, 2, 0 },
    { 38, 48, 0, 0, 1, 0, CLACT_SURFACE_MAIN, 4, 3, 2, 2, 2, 0 },
    { 38, 80, 0, 0, 1, 0, CLACT_SURFACE_MAIN, 5, 3, 2, 2, 2, 0 },
    { 38, 112, 0, 0, 1, 0, CLACT_SURFACE_MAIN, 6, 3, 2, 2, 2, 0 },
    { 38, 144, 0, 0, 1, 0, CLACT_SURFACE_MAIN, 7, 3, 2, 2, 2, 0 },
    { 204, 124, 0, 0, 0, 0, CLACT_SURFACE_MAIN, 8, 4, 5, 5, 2, 0 },
    { 224, 168, 0, 1, 0, 0, CLACT_SURFACE_SUB, 0, 0, 0, 0, 2, 0 },
};

static void WazaOshie_InitActors(WazaOshieWork *work, ArcTool *arc) {
    u32 i;
    ClActorSetup setup;

    WazaOshie_InitActorSys(work);
    WazaOshie_LoadActorResources(work, arc);
    work->commonArc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_WAZA_OSHIE);
    for (i = 0; i < ACTOR_COUNT; i++) {
        setup.x = sWazaOshieActors[i].x;
        setup.y = sWazaOshieActors[i].y;
        setup.sequence = sWazaOshieActors[i].sequence;
        setup.priority = sWazaOshieActors[i].priority;
        setup.bgPriority = sWazaOshieActors[i].bgPriority;
        work->actors[i] = func_0204c040(
            work->unit, work->chars[sWazaOshieActors[i].chars], work->palettes[sWazaOshieActors[i].palette],
            work->cellAnims[sWazaOshieActors[i].cellAnims], &setup, sWazaOshieActors[i].surface, HEAPID_WAZA_OSHIE);
        func_0204c520(work->actors[i], FALSE);
    }
    func_0204c520(work->actors[ACTOR_RETURN], TRUE);
    func_0204c520(work->actors[ACTOR_CURSOR], TRUE);
    func_0204c520(work->actors[ACTOR_ARROW_UP], TRUE);
    func_0204c520(work->actors[ACTOR_ARROW_DOWN], TRUE);
}

// Moves the cursor's actor to a row of the list, an arrow or the buttons, with the frame for each
static void WazaOshie_SetCursorActor(WazaOshieWork *work, u8 pos, u32 unused) {
    ClActorPos actorPos;

    if (pos < LIST_ROWS) {
        actorPos.x = 130;
        actorPos.y = pos * 24 + 84;
        func_0204c140(work->actors[ACTOR_CURSOR], &actorPos, CLACT_SURFACE_SUB);
        func_0204c4d4(work->actors[ACTOR_CURSOR], 0);
    } else if (pos < POS_RETURN) {
        actorPos.x = (pos - POS_ARROW_DOWN) * 40;
        actorPos.y = 168;
        func_0204c140(work->actors[ACTOR_CURSOR], &actorPos, CLACT_SURFACE_SUB);
        func_0204c4d4(work->actors[ACTOR_CURSOR], 1);
    } else {
        actorPos.x = 192;
        actorPos.y = 160;
        func_0204c140(work->actors[ACTOR_CURSOR], &actorPos, CLACT_SURFACE_SUB);
        func_0204c4d4(work->actors[ACTOR_CURSOR], 2);
    }
}

static u32 WazaOshie_SeqStatusFadeOut(WazaOshieWork *work) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, HEAPID_WAZA_OSHIE);
    work->nextSeq = SEQ_STATUS_CALL;
    return SEQ_FADE_WAIT;
}

// Ends the screen's graphics and runs the summary screen to pick the move to forget
static u32 WazaOshie_SeqStatusCall(WazaOshieWork *work) {
    WazaOshie_FreeScreen(work);
    work->status.party = work->param->pkm;
    work->status.trainerData = work->param->trainerData;
    work->status.gameData = GSYS_GetGameData(work->param->gsys);
    work->status.dataType = PSTATUS_DATA_PARTY_PKM;
    work->status.partyIndex = 0;
    work->status.partyCount = 1;
    work->status.move = WazaOshie_GetCursorMove(work);
    work->status.mode = PSTATUS_MODE_FORGET_MOVE;
    work->status.page = PSTATUS_PAGE_INFO;
    work->status.isNationalDex = FALSE;
    GSYS_QueueProc(work->param->gsys, OVERLAY_PSTATUS, &PSTATUS_PROC_FUNCTIONS, &work->status);
    return SEQ_STATUS_RETURN;
}

static u32 WazaOshie_SeqStatusReturn(WazaOshieWork *work) {
    WazaOshie_InitScreen(work);
    WazaOshie_SetCursorActor(work, work->param->cursorPos, 3);
    func_02042ba8(TRUE, HEAPID_WAZA_OSHIE);
    if (work->status.result == PSTATUS_RESULT_FORGET) {
        work->param->slot = work->status.slot;
    } else {
        work->param->slot = 4;
    }
    work->nextSeq = SEQ_STATUS_RESULT;
    return SEQ_FADE_WAIT;
}

// The list's rows, the arrows, the return icon and the two buttons
// clang-format off
static const CursorMoveData sWazaOshieCursorData[] = {
    { 40, 52, 0, 0, 0, 1, 0, 0, { 69, 90, 21, 233 } },
    { 80, 60, 0, 0, 0, 2, 1, 1, { 93, 114, 21, 233 } },
    { 40, 84, 0, 0, 1, 3, 2, 2, { 117, 138, 21, 233 } },
    { 80, 92, 0, 0, 2, 6, 3, 3, { 141, 162, 21, 233 } },
    { 40, 116, 0, 0, 4, 4, 4, 4, { 168, 191, 0, 39 } },
    { 80, 124, 0, 0, 5, 5, 5, 5, { 168, 191, 40, 79 } },
    { 224, 168, 0, 0, 3, 6, 6, 6, { 166, 191, 224, 247 } },
    { 224, 168, 0, 0, 7, 7, 7, 7, { 166, 191, 128, 191 } },
    { 224, 168, 0, 0, 8, 8, 8, 8, { 166, 191, 192, 255 } },
    { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
};

// clang-format on

static const CursorMoveCallbacks sWazaOshieCursorCallbacks = {
    WazaOshie_CursorDummy,
    WazaOshie_CursorDummy,
    WazaOshie_CursorMove,
    WazaOshie_CursorTouch,
};

static void WazaOshie_InitCursor(WazaOshieWork *work) {
    work->cursor =
        CursorMove_Create(sWazaOshieCursorData, &sWazaOshieCursorCallbacks, work, TRUE, 0, HEAPID_WAZA_OSHIE);
    CursorMove_DisablePos(work->cursor, POS_QUIT);
    WazaOshie_UpdateArrowPositions(work);
}

static void WazaOshie_FreeCursor(WazaOshieWork *work) {
    CursorMove_Delete(work->cursor);
}

static void WazaOshie_CursorDummy(void *data, int pos, int prevPos) {
}

// The cursor moved with the keys: onto a row, or past the list's ends, which scrolls it
static void WazaOshie_CursorMove(void *data, int pos, int prevPos) {
    WazaOshieWork *work = data;

    WazaOshie_ShowButtons(work, FALSE);
    if (pos <= 3) {
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        work->param->cursorPos = pos;
        if (work->param->scroll + work->param->cursorPos < work->moveCount) {
            WazaOshie_DrawMoveInfo(work, WazaOshie_GetCursorMove(work));
        } else {
            WazaOshie_DrawMoveInfo(work, NO_MOVE);
        }
    } else if (pos == POS_RETURN) {
        if (prevPos == 3 && work->param->scroll + LIST_ROWS < work->moveCount) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            pos = 3;
            CursorMove_SetPos(work->cursor, pos);
            work->param->scroll++;
            WazaOshie_DrawList(work);
            WazaOshie_DrawMoveInfo(work, WazaOshie_GetCursorMove(work));
            WazaOshie_UpdateArrowPositions(work);
            WazaOshie_AnimateArrows(work, 1);
        } else if (prevPos == 0) {
            if (work->param->scroll != 0) {
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
                work->param->scroll--;
                WazaOshie_DrawList(work);
                WazaOshie_DrawMoveInfo(work, WazaOshie_GetCursorMove(work));
                WazaOshie_UpdateArrowPositions(work);
                WazaOshie_AnimateArrows(work, -1);
            }
            pos = 0;
            CursorMove_SetPos(work->cursor, pos);
        } else {
            pos = 3;
            CursorMove_SetPos(work->cursor, pos);
        }
    } else {
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
    }
    WazaOshie_SetCursorActor(work, pos, 3);
}

// A position was touched: a row, which shows the buttons, or an arrow, which scrolls the list
static void WazaOshie_CursorTouch(void *data, int pos, int prevPos) {
    WazaOshieWork *work = data;

    if (pos <= 3) {
        work->param->cursorPos = pos;
        if (work->param->scroll + work->param->cursorPos < work->moveCount) {
            WazaOshie_ShowButtons(work, TRUE);
            WazaOshie_DrawMoveInfo(work, WazaOshie_GetCursorMove(work));
        } else {
            WazaOshie_ShowButtons(work, FALSE);
            WazaOshie_DrawMoveInfo(work, NO_MOVE);
        }
    } else if (pos == POS_ARROW_DOWN) {
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        pos = work->param->cursorPos;
        work->param->scroll++;
        WazaOshie_DrawMoveInfo(work, WazaOshie_GetCursorMove(work));
        WazaOshie_DrawList(work);
        WazaOshie_ShowButtons(work, FALSE);
        WazaOshie_UpdateArrowPositions(work);
        WazaOshie_AnimateArrows(work, 1);
        CursorMove_SetPos(work->cursor, pos);
    } else if (pos == POS_ARROW_UP) {
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        pos = work->param->cursorPos;
        work->param->scroll--;
        WazaOshie_DrawMoveInfo(work, WazaOshie_GetCursorMove(work));
        WazaOshie_DrawList(work);
        WazaOshie_ShowButtons(work, FALSE);
        WazaOshie_UpdateArrowPositions(work);
        WazaOshie_AnimateArrows(work, -1);
        CursorMove_SetPos(work->cursor, pos);
    } else if (pos == POS_RETURN) {
        if (work->prevPos != POS_RETURN) {
            pos = work->param->cursorPos;
            CursorMove_SetPos(work->cursor, pos);
        }
    } else if (pos == POS_TEACH) {
        pos = work->param->cursorPos;
        CursorMove_SetPos(work->cursor, pos);
    } else if (pos == POS_QUIT) {
        if (work->prevPos != POS_RETURN) {
            pos = work->param->cursorPos;
            CursorMove_SetPos(work->cursor, pos);
        }
    }
    WazaOshie_SetCursorActor(work, pos, 3);
}

// Shows or removes the Teach and Quit buttons
static void WazaOshie_ShowButtons(WazaOshieWork *work, BOOL show) {
    if (show == TRUE) {
        if (work->buttons[BUTTON_TEACH] == NULL) {
            work->buttons[BUTTON_TEACH] =
                AppTaskMenuWin_Create(work->menuRes, &work->buttonItems[BUTTON_TEACH], 13, 21, 9, HEAPID_WAZA_OSHIE);
            work->buttons[BUTTON_QUIT] =
                AppTaskMenuWin_Create(work->menuRes, &work->buttonItems[BUTTON_QUIT], 22, 21, 9, HEAPID_WAZA_OSHIE);
            CursorMove_EnablePos(work->cursor, POS_QUIT);
            work->buttonsShown = TRUE;
        }
    } else {
        if (work->buttons[BUTTON_TEACH] != NULL) {
            AppTaskMenuWin_Free(work->buttons[BUTTON_TEACH]);
            AppTaskMenuWin_Free(work->buttons[BUTTON_QUIT]);
            GFL_BGSysLoadScr(4);
            work->buttons[BUTTON_TEACH] = NULL;
            work->buttons[BUTTON_QUIT] = NULL;
        }
        CursorMove_DisablePos(work->cursor, POS_QUIT);
        work->buttonsShown = FALSE;
    }
}

static BOOL WazaOshie_AreButtonsShown(WazaOshieWork *work) {
    if (work->buttons[BUTTON_TEACH] != NULL) {
        return TRUE;
    }
    return FALSE;
}

// Lets the cursor onto the arrows that can scroll the list
static void WazaOshie_UpdateArrowPositions(WazaOshieWork *work) {
    if (work->moveCount < LIST_ROWS) {
        CursorMove_DisablePos(work->cursor, POS_ARROW_DOWN);
        CursorMove_DisablePos(work->cursor, POS_ARROW_UP);
        return;
    }
    if (work->param->scroll == 0) {
        CursorMove_DisablePos(work->cursor, POS_ARROW_UP);
    } else {
        CursorMove_EnablePos(work->cursor, POS_ARROW_UP);
    }
    if (work->param->scroll + LIST_ROWS >= work->moveCount) {
        CursorMove_DisablePos(work->cursor, POS_ARROW_DOWN);
    } else {
        CursorMove_EnablePos(work->cursor, POS_ARROW_DOWN);
    }
}

// Plays the arrows' animations after the list scrolled down (direction above 0) or up
static void WazaOshie_AnimateArrows(WazaOshieWork *work, int direction) {
    func_0204c4d4(work->actors[ACTOR_ARROW_UP], 0);
    func_0204c4d4(work->actors[ACTOR_ARROW_DOWN], 0);
    if (direction > 0) {
        if (work->param->scroll + LIST_ROWS < work->moveCount) {
            func_0204c488(work->actors[ACTOR_ARROW_DOWN], 10);
        } else {
            func_0204c488(work->actors[ACTOR_ARROW_DOWN], 23);
        }
        func_0204c488(work->actors[ACTOR_ARROW_UP], 3);
    } else {
        if (work->param->scroll == 0) {
            func_0204c488(work->actors[ACTOR_ARROW_UP], 24);
        } else {
            func_0204c488(work->actors[ACTOR_ARROW_UP], 11);
        }
        func_0204c488(work->actors[ACTOR_ARROW_DOWN], 2);
    }
}

static void WazaOshie_InitArrows(WazaOshieWork *work) {
    if (work->moveCount <= LIST_ROWS) {
        func_0204c488(work->actors[ACTOR_ARROW_UP], 17);
        func_0204c488(work->actors[ACTOR_ARROW_DOWN], 16);
    } else if (work->param->scroll + LIST_ROWS < work->moveCount) {
        func_0204c488(work->actors[ACTOR_ARROW_UP], 17);
        func_0204c488(work->actors[ACTOR_ARROW_DOWN], 2);
    } else if (work->param->scroll == 0) {
        func_0204c488(work->actors[ACTOR_ARROW_UP], 3);
        func_0204c488(work->actors[ACTOR_ARROW_DOWN], 16);
    }
}

// Start the animation of the return icon or the flash of a button, after which the process goes to seq
static u32 WazaOshie_DecideActor(WazaOshieWork *work, u16 seq) {
    WazaOshieDecide *decide = &work->decide;

    decide->type = DECIDE_ACTOR;
    decide->index = ACTOR_RETURN;
    decide->seq = seq;
    func_0204c488(work->actors[ACTOR_RETURN], 9);
    return SEQ_DECIDE_WAIT;
}

static u32 WazaOshie_DecideTeach(WazaOshieWork *work, u16 seq) {
    WazaOshieDecide *decide = &work->decide;

    decide->type = DECIDE_BUTTON;
    decide->index = BUTTON_TEACH;
    decide->seq = seq;
    AppTaskMenuWin_SetFlashing(work->buttons[decide->index], TRUE);
    return SEQ_DECIDE_WAIT;
}

static u32 WazaOshie_DecideQuit(WazaOshieWork *work, u16 seq) {
    WazaOshieDecide *decide = &work->decide;

    decide->type = DECIDE_BUTTON;
    decide->index = BUTTON_QUIT;
    decide->seq = seq;
    AppTaskMenuWin_SetFlashing(work->buttons[decide->index], TRUE);
    return SEQ_DECIDE_WAIT;
}

static u32 WazaOshie_SeqDecideWait(WazaOshieWork *work) {
    WazaOshieDecide *decide = &work->decide;

    if (decide->type == DECIDE_ACTOR) {
        if (!func_0204c560(work->actors[decide->index])) {
            return decide->seq;
        }
    } else if (decide->type == DECIDE_BUTTON) {
        if (AppTaskMenuWin_IsFlashFinished(work->buttons[decide->index])) {
            WazaOshie_ShowButtons(work, FALSE);
            return decide->seq;
        }
    }
    return SEQ_DECIDE_WAIT;
}

static u32 WazaOshie_SeqQuit(WazaOshieWork *work) {
    WazaOshie_PrintMessage(work, MSG_QUIT);
    work->yesNo = YES_NO_QUIT;
    work->nextSeq = SEQ_YES_NO_INIT;
    return SEQ_MESSAGE;
}

static u32 WazaOshie_SeqMoveChosen(WazaOshieWork *work) {
    u8 yesNo;

    if (WazaOshie_CountKnownMoves(work) < 4) {
        WazaOshie_PrintMessage(work, MSG_LEARN);
        yesNo = YES_NO_LEARN;
    } else {
        WazaOshie_PrintMessage(work, MSG_REPLACE);
        yesNo = YES_NO_REPLACE;
    }
    work->yesNo = yesNo;
    work->nextSeq = SEQ_YES_NO_INIT;
    return SEQ_MESSAGE;
}

// Darkens the touch screen behind the yes/no menu
static void WazaOshie_SetDimmed(BOOL dimmed) {
    if (dimmed == TRUE) {
        gfxRegSetBrightnessBlend(
            REG_DB_BLDCNT_ADDR,
            GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ, -8);
    } else {
        G2S_BlendNone();
    }
}
