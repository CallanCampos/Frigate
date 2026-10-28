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
#include "pml/move.h"
#include "pml/move_reminder.h"
#include "pml/poke_graphic.h"
#include "pml/poke_party.h"
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
#include "text/system/evolution_demo_btl_main.h"

// The move reminder's screen, which the move tutors' scripts show too: the Pokémon and its moves on the top screen,
// and on the touch screen the list of moves to pick from with the picked move's data. Replacing a move goes through
// the summary screen, which ends this screen's graphics and builds them again on return. The ROM has no name for this
// file; "waza_oshie_main.c" is a guess from "waza_oshie.c", the file of overlay 12 that allocates its parameter

#define HEAPID_MOVE_TUTOR 0x4f

#define RESOURCE_COUNT 9
#define TYPE_ICON_COUNT 8
#define LIST_ROWS 4
#define MOVE_END 0xffff
// What the info windows are drawn for when the cursor's row has no move
#define NO_MOVE 0xfffffffe

// The windows
enum {
    WINDOW_POWER_LABEL = 2,
    WINDOW_ACCURACY_LABEL,
    WINDOW_POWER,
    WINDOW_ACCURACY,
    WINDOW_DESCRIPTION,
    WINDOW_MESSAGE,
    WINDOW_TEACH = 9,
    WINDOW_LIST = 11,
    WINDOW_TITLE = 13,
    WINDOW_KNOWN_MOVES,
    WINDOW_POKEMON,
    WINDOW_COUNT,
};

// The actors
enum {
    ACTOR_ARROW_DOWN,
    ACTOR_ARROW_UP,
    ACTOR_CURSOR,
    ACTOR_CATEGORY,
    ACTOR_LIST_TYPE,
    ACTOR_KNOWN_TYPE = 8,
    ACTOR_POKEMON = 12,
    ACTOR_RETURN,
    ACTOR_COUNT,
};

// The cursor's positions: the list's rows, then
enum {
    POS_ARROW_DOWN = 4,
    POS_ARROW_UP,
    POS_RETURN,
    POS_TEACH,
    POS_QUIT,
};

// The buttons shown once a move is picked by touch
enum {
    BUTTON_TEACH,
    BUTTON_QUIT,
};

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

// The messages, which index a row of sMoveTutorMessages
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

// The questions, which index sMoveTutorYesNoFuncs
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

// How MoveTutor_PrintText places the text in its window
enum {
    ALIGN_LEFT,
    ALIGN_RIGHT,
    ALIGN_CENTER,
};

typedef struct {
    u32 type;
    BOOL dirty;
} MoveTutorTypeIcon;

// What SEQ_DECIDE_WAIT waits for: an actor's animation or a button's flash, and the sequence that follows
typedef struct {
    u8 unk00[6];
    u16 index;
    u16 seq;
    u16 type;
} MoveTutorDecide;

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
    MoveTutorTypeIcon typeIcons[TYPE_ICON_COUNT];
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
    MoveTutorDecide decide;
    AppTaskMenuRes *menuRes;
    AppTaskMenu *menu;
    AppTaskMenuItem buttonItems[2];
    AppTaskMenuItem yesNoItems[2];
    u32 unk214;
    PrintQueue *printQueue;
    KeyCursor *keyCursor;
    AppTaskMenuWin *buttons[2];
} MoveTutorWork;

typedef struct {
    u32 msg;
    u32 type;
} MoveTutorMenuItem;

typedef u32 (*MoveTutorYesNoFunc)(MoveTutorWork *work);

typedef struct {
    MoveTutorYesNoFunc yes;
    MoveTutorYesNoFunc no;
} MoveTutorYesNoFuncs;

typedef struct {
    u32 bg;
    u32 x;
    u32 y;
    u32 width;
    u32 height;
    u32 palette;
    u32 charOffset;
} MoveTutorWindowData;

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
} MoveTutorActorData;

static BOOL MoveTutorProc_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MoveTutorProc_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MoveTutorProc_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void MoveTutor_InitMenu(MoveTutorWork *work);
static void MoveTutor_FreeMenu(MoveTutorWork *work);
static void MoveTutor_UpdateButtons(MoveTutorWork *work);
static void MoveTutor_InitScreen(MoveTutorWork *work);
static void MoveTutor_FreeScreen(MoveTutorWork *work);
static void MoveTutor_VBlank(TCB *tcb, void *data);
static void MoveTutor_SetVRAMBanks(void);
static void MoveTutor_InitBG(void);
static void MoveTutor_FreeBG(void);
static void MoveTutor_LoadBG(MoveTutorWork *work, ArcTool *arc);
static void MoveTutor_FreeScreenBuffer(MoveTutorWork *work);
static void MoveTutor_InitWindows(MoveTutorWork *work);
static void MoveTutor_FreeWindows(MoveTutorWork *work);
static void MoveTutor_InitMsg(MoveTutorWork *work);
static void MoveTutor_FreeMsg(MoveTutorWork *work);
static u32 MoveTutor_SeqFadeWait(MoveTutorWork *work);
static u32 MoveTutor_SeqSelect(MoveTutorWork *work);
static u32 MoveTutor_SeqMessage(MoveTutorWork *work);
static u32 MoveTutor_SeqYesNoInit(MoveTutorWork *work);
static u32 MoveTutor_SeqYesNo(MoveTutorWork *work);
static u32 MoveTutor_SeqSetMove(MoveTutorWork *work);
static u32 MoveTutor_SeqForgot(MoveTutorWork *work);
static u32 MoveTutor_SeqStatusResult(MoveTutorWork *work);
static u32 MoveTutor_SeqFadeOut(MoveTutorWork *work);
static void MoveTutor_DrawCursorMoveInfo(MoveTutorWork *work);
static void MoveTutor_PrintText(MoveTutorWork *work, u32 window, Font *font, u16 color, u32 align, u8 y);
static void MoveTutor_FormatNumber(MoveTutorWork *work, u32 msg, u32 number, u32 digits, u8 pad);
static void MoveTutor_DrawText(MoveTutorWork *work);
static u32 MoveTutor_CountMoves(MoveTutorWork *work);
static void MoveTutor_DrawListRow(MoveTutorWork *work, u8 scroll, u8 row);
static void MoveTutor_DrawList(MoveTutorWork *work);
static void MoveTutor_InitList(MoveTutorWork *work);
static void MoveTutor_FreeList(MoveTutorWork *work);
static void MoveTutor_DrawMoveInfo(MoveTutorWork *work, u32 move);
static void MoveTutor_FormatMessage(MoveTutorWork *work, u32 msg);
static u16 MoveTutor_GetCursorMove(MoveTutorWork *work);
static u16 MoveTutor_GetReplacedMove(MoveTutorWork *work);
static void MoveTutor_PrintMessage(MoveTutorWork *work, u32 msg);
static void MoveTutor_ClearMessage(MoveTutorWork *work);
static BOOL MoveTutor_StreamCallback(u32 event);
static u8 MoveTutor_CountKnownMoves(MoveTutorWork *work);
static u32 MoveTutor_LearnYes(MoveTutorWork *work);
static u32 MoveTutor_LearnNo(MoveTutorWork *work);
static u32 MoveTutor_QuitYes(MoveTutorWork *work);
static u32 MoveTutor_QuitNo(MoveTutorWork *work);
static u32 MoveTutor_ReplaceYes(MoveTutorWork *work);
static u32 MoveTutor_ReplaceNo(MoveTutorWork *work);
static u32 MoveTutor_GiveUpYes(MoveTutorWork *work);
static u32 MoveTutor_GiveUpNo(MoveTutorWork *work);
static u32 MoveTutor_ForgetYes(MoveTutorWork *work);
static u32 MoveTutor_ForgetNo(MoveTutorWork *work);
static void MoveTutor_InitActorSys(MoveTutorWork *work);
static void MoveTutor_FreeActors(MoveTutorWork *work);
static void MoveTutor_LoadActorResources(MoveTutorWork *work, ArcTool *arc);
static void MoveTutor_ReplaceChars(ArcTool *arc, u32 chars, u32 fileId, BOOL compressed, HeapID heapId);
static void MoveTutor_SetKnownMoveType(MoveTutorWork *work, u16 move, u16 index);
static void MoveTutor_LoadTypeIcons(MoveTutorWork *work);
static void MoveTutor_UpdateListTypes(MoveTutorWork *work);
static void MoveTutor_SetCategory(MoveTutorWork *work, u16 move);
static void MoveTutor_InitActors(MoveTutorWork *work, ArcTool *arc);
static void MoveTutor_SetCursorActor(MoveTutorWork *work, u8 pos, u32 unused);
static u32 MoveTutor_SeqStatusFadeOut(MoveTutorWork *work);
static u32 MoveTutor_SeqStatusCall(MoveTutorWork *work);
static u32 MoveTutor_SeqStatusReturn(MoveTutorWork *work);
static void MoveTutor_InitCursor(MoveTutorWork *work);
static void MoveTutor_FreeCursor(MoveTutorWork *work);
static void MoveTutor_CursorDummy(void *data, int pos, int prevPos);
static void MoveTutor_CursorMove(void *data, int pos, int prevPos);
static void MoveTutor_CursorTouch(void *data, int pos, int prevPos);
static void MoveTutor_ShowButtons(MoveTutorWork *work, BOOL show);
static BOOL MoveTutor_AreButtonsShown(MoveTutorWork *work);
static void MoveTutor_UpdateArrowPositions(MoveTutorWork *work);
static void MoveTutor_AnimateArrows(MoveTutorWork *work, int direction);
static void MoveTutor_InitArrows(MoveTutorWork *work);
static u32 MoveTutor_DecideActor(MoveTutorWork *work, u16 seq);
static u32 MoveTutor_DecideTeach(MoveTutorWork *work, u16 seq);
static u32 MoveTutor_DecideQuit(MoveTutorWork *work, u16 seq);
static u32 MoveTutor_SeqDecideWait(MoveTutorWork *work);
static u32 MoveTutor_SeqQuit(MoveTutorWork *work);
static u32 MoveTutor_SeqMoveChosen(MoveTutorWork *work);
static void MoveTutor_SetDimmed(BOOL dimmed);

// BmpWin_ClearScreenNow with the screen copied at the next VBlank
static inline void MoveTutor_ClearWindow(BmpWin *window) {
    BmpWin_ClearScreen(window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
}

const GameProcFunctions MoveTutorProcFunctions = { MoveTutorProc_Init, MoveTutorProc_Main, MoveTutorProc_Exit };

// The tables are declared in the order that gives the ROM's .rodata, with those of the actors and the cursor ahead
// of the functions that read them

// clang-format off
static const BGSysVRAMConfig sMoveTutorVRAMConfig = {
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

static const MoveTutorMenuItem sMoveTutorButtonItems[2] = {
    { EvolutionDemoBtlMain_Text_Teach, 0 },
    { EvolutionDemoBtlMain_Text_Quit, APP_TASKMENU_ITEM_RETURN },
};

static const MoveTutorMenuItem sMoveTutorYesNoItems[2] = {
    { EvolutionDemoBtlMain_Text_Yes, 0 },
    { EvolutionDemoBtlMain_Text_No, 0 },
};

// clang-format off
static const MoveTutorWindowData sMoveTutorMainWindows[WINDOW_COUNT] = {
    { 2, 0, 25, 1, 1, 0, 0 },      { 6, 0, 25, 1, 1, 0, 0 },       { 6, 8, 6, 6, 2, 15, 43 },
    { 6, 19, 6, 8, 2, 15, 55 },    { 6, 14, 6, 3, 2, 15, 71 },     { 6, 27, 6, 3, 2, 15, 77 },
    { 6, 1, 0, 30, 6, 15, 83 },    { 4, 2, 19, 27, 4, 14, 263 },   { 6, 1, 0, 30, 3, 15, 371 },
    { 6, 17, 21, 6, 3, 15, 461 },  { 6, 25, 21, 6, 3, 15, 479 },   { 6, 8, 9, 21, 11, 15, 497 },
    { 4, 23, 13, 7, 4, 14, 728 },  { 2, 1, 0, 20, 3, 15, 1 },      { 2, 7, 5, 11, 16, 15, 61 },
    { 2, 21, 4, 9, 8, 15, 237 },
};

// clang-format on

// The messages, by the parameter's mode
static const u32 sMoveTutorMessages[2][MESSAGE_COUNT] = {
    {
        EvolutionDemoBtlMain_Text_Empty,
        EvolutionDemoBtlMain_Text_Empty_2,
        EvolutionDemoBtlMain_Text_Empty_3,
        EvolutionDemoBtlMain_Text_Learned,
        EvolutionDemoBtlMain_Text_WantsLearnMoveHowever,
        EvolutionDemoBtlMain_Text_12TaDa,
        EvolutionDemoBtlMain_Text_LearnedHowUse,
        EvolutionDemoBtlMain_Text_GiveUpLearningMove,
        EvolutionDemoBtlMain_Text_DidNotLearn,
        EvolutionDemoBtlMain_Text_Empty_4,
        EvolutionDemoBtlMain_Text_Empty_5,
    },
    {
        EvolutionDemoBtlMain_Text_WhichMoveShouldRemember,
        EvolutionDemoBtlMain_Text_ShouldMoveRemembered,
        EvolutionDemoBtlMain_Text_GiveUpTryingMake,
        EvolutionDemoBtlMain_Text_RememberedMove,
        EvolutionDemoBtlMain_Text_WantsRememberMoveBut,
        EvolutionDemoBtlMain_Text_12TaDa_2,
        EvolutionDemoBtlMain_Text_RememberedMove_2,
        EvolutionDemoBtlMain_Text_GiveUpMakingPokemon,
        EvolutionDemoBtlMain_Text_DidNotRememberMove,
        EvolutionDemoBtlMain_Text_HandedOverOneHeart,
        EvolutionDemoBtlMain_Text_OkForgetMove,
    },
};

// clang-format off
static const MoveTutorYesNoFuncs sMoveTutorYesNoFuncs[5] = {
    { MoveTutor_LearnYes, MoveTutor_LearnNo },
    { MoveTutor_QuitYes, MoveTutor_QuitNo },
    { MoveTutor_ReplaceYes, MoveTutor_ReplaceNo },
    { MoveTutor_GiveUpYes, MoveTutor_GiveUpNo },
    { MoveTutor_ForgetYes, MoveTutor_ForgetNo },
};

// clang-format on

static const ClActSysSetup sMoveTutorActorSysSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 9, 5, 6, 32, 16, 16 };

static BOOL MoveTutorProc_Init(GameProc *proc, u32 *state, void *param, void *work) {
    MoveTutorWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_MOVE_TUTOR, 0x50000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(MoveTutorWork), HEAPID_MOVE_TUTOR);
    MI_CpuFill8(wk, 0, sizeof(MoveTutorWork));
    wk->param = param;
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_MOVE_TUTOR);
    wk->tcbManager = GFL_TCBExMgrCreate(HEAPID_MOVE_TUTOR, HEAPID_MOVE_TUTOR, 32, 32);
    MoveTutor_InitScreen(wk);
    MoveTutor_SetCursorActor(wk, wk->param->cursorPos, 3);
    wk->nextSeq = SEQ_SELECT;
    GFL_NetSetIconScreen(TRUE, HEAPID_MOVE_TUTOR);
    return TRUE;
}

static BOOL MoveTutorProc_Main(GameProc *proc, u32 *state, void *param, void *work) {
    MoveTutorWork *wk = work;

    switch (*state) {
    case SEQ_FADE_WAIT:
        *state = MoveTutor_SeqFadeWait(wk);
        break;
    case SEQ_SELECT:
        *state = MoveTutor_SeqSelect(wk);
        break;
    case SEQ_MESSAGE:
        *state = MoveTutor_SeqMessage(wk);
        break;
    case SEQ_YES_NO_INIT:
        *state = MoveTutor_SeqYesNoInit(wk);
        break;
    case SEQ_YES_NO:
        *state = MoveTutor_SeqYesNo(wk);
        break;
    case SEQ_SET_MOVE:
        *state = MoveTutor_SeqSetMove(wk);
        break;
    case SEQ_FORGOT:
        *state = MoveTutor_SeqForgot(wk);
        break;
    case SEQ_STATUS_RESULT:
        *state = MoveTutor_SeqStatusResult(wk);
        break;
    case SEQ_FADE_OUT:
        *state = MoveTutor_SeqFadeOut(wk);
        break;
    case SEQ_END:
        return TRUE;
    case SEQ_STATUS_FADE_OUT:
        *state = MoveTutor_SeqStatusFadeOut(wk);
        break;
    case SEQ_STATUS_CALL:
        *state = MoveTutor_SeqStatusCall(wk);
        return FALSE;
    case SEQ_STATUS_RETURN:
        *state = MoveTutor_SeqStatusReturn(wk);
        return FALSE;
    case SEQ_DECIDE_WAIT:
        *state = MoveTutor_SeqDecideWait(wk);
        break;
    case SEQ_QUIT:
        *state = MoveTutor_SeqQuit(wk);
        break;
    case SEQ_MOVE_CHOSEN:
        *state = MoveTutor_SeqMoveChosen(wk);
        break;
    }
    MoveTutor_UpdateButtons(wk);
    ClActSys_Draw();
    GFL_TCBExMgrUpdate(wk->tcbManager);
    PrintQueue_Main(wk->printQueue);
    return FALSE;
}

static BOOL MoveTutorProc_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    MoveTutorWork *wk = work;

    MoveTutor_FreeScreen(wk);
    GFL_TCBExMgrFree(wk->tcbManager);
    GFL_FontFree(wk->font);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_MOVE_TUTOR);
    return TRUE;
}

static void MoveTutor_InitMenu(MoveTutorWork *work) {
    int i;

    work->printQueue = PrintQueue_CreateDefault(HEAPID_MOVE_TUTOR);
    work->menuRes = AppTaskMenuRes_Create(4, 10, work->font, work->printQueue, HEAPID_MOVE_TUTOR);
    for (i = 0; i < 2; i++) {
        work->buttonItems[i].str = GFL_MsgDataLoadStrbufNew(work->msgData, sMoveTutorButtonItems[i].msg);
        work->buttonItems[i].color = PRINT_COLOR(14, 15, 3);
        work->buttonItems[i].type = sMoveTutorButtonItems[i].type;
    }
    for (i = 0; i < 2; i++) {
        work->yesNoItems[i].str = GFL_MsgDataLoadStrbufNew(work->msgData, sMoveTutorYesNoItems[i].msg);
        work->yesNoItems[i].color = PRINT_COLOR(14, 15, 3);
        work->yesNoItems[i].type = sMoveTutorYesNoItems[i].type;
    }
    work->buttons[BUTTON_TEACH] = NULL;
    work->buttons[BUTTON_QUIT] = NULL;
}

static void MoveTutor_FreeMenu(MoveTutorWork *work) {
    MoveTutor_ShowButtons(work, FALSE);
    GFL_StrBufFree(work->buttonItems[0].str);
    GFL_StrBufFree(work->buttonItems[1].str);
    GFL_StrBufFree(work->yesNoItems[0].str);
    GFL_StrBufFree(work->yesNoItems[1].str);
    AppTaskMenuRes_Free(work->menuRes);
    PrintQueue_Free(work->printQueue);
}

static void MoveTutor_UpdateButtons(MoveTutorWork *work) {
    if (work->buttons[BUTTON_TEACH] != NULL) {
        AppTaskMenuWin_Update(work->buttons[BUTTON_TEACH]);
        AppTaskMenuWin_Update(work->buttons[BUTTON_QUIT]);
    }
}

static void MoveTutor_InitScreen(MoveTutorWork *work) {
    ArcTool *arc;

    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GX_SetVisiblePlane(0);
    GXS_SetVisiblePlane(0);
    arc = GFL_ArcSysCreateFileHandle(ARCID_MOVE_TUTOR, HEAPID_MOVE_TUTOR);
    MoveTutor_SetVRAMBanks();
    MoveTutor_InitBG();
    MoveTutor_LoadBG(work, arc);
    MoveTutor_InitActors(work, arc);
    MoveTutor_InitMsg(work);
    MoveTutor_InitWindows(work);
    MoveTutor_InitList(work);
    MoveTutor_DrawCursorMoveInfo(work);
    MoveTutor_DrawText(work);
    MoveTutor_InitCursor(work);
    MoveTutor_InitArrows(work);
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, HEAPID_MOVE_TUTOR);
    GFL_BGSysSetDisplayLayout(1);
    work->vblankTcb = GFL_VBlankTCBAdd(MoveTutor_VBlank, work, 16);
    GFL_ArcToolFree(arc);
    MoveTutor_InitMenu(work);
}

static void MoveTutor_FreeScreen(MoveTutorWork *work) {
    MoveTutor_FreeMenu(work);
    MoveTutor_FreeCursor(work);
    MoveTutor_FreeList(work);
    MoveTutor_FreeWindows(work);
    MoveTutor_FreeScreenBuffer(work);
    MoveTutor_FreeBG();
    MoveTutor_FreeMsg(work);
    MoveTutor_FreeActors(work);
    GFL_TCBRemove(work->vblankTcb);
}

static void MoveTutor_VBlank(TCB *tcb, void *data) {
    MoveTutor_LoadTypeIcons(data);
    GFL_BGSysUpdate();
    ClActSys_VBlank();
    OS_SetIrqCheckFlag(OS_IE_V_BLANK);
}

static void MoveTutor_SetVRAMBanks(void) {
    GFL_BGSysSetVRAMBanks(&sMoveTutorVRAMConfig);
}

static void MoveTutor_InitBG(void) {
    GFL_BGSysCreate(HEAPID_MOVE_TUTOR);
    BmpWin_InitAllocator(HEAPID_MOVE_TUTOR);
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

        GFL_BGSysCreateBG(2, &setup, BGMODE_TEXT);
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

        GFL_BGSysCreateBG(3, &setup, BGMODE_TEXT);
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

        GFL_BGSysCreateBG(4, &setup, BGMODE_TEXT);
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

        GFL_BGSysCreateBG(6, &setup, BGMODE_TEXT);
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

        GFL_BGSysCreateBG(5, &setup, BGMODE_TEXT);
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

        GFL_BGSysCreateBG(7, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(7);
    }
    GFL_BGSysSetBGEnabled(2, TRUE);
    GFL_BGSysSetBGEnabled(3, TRUE);
    GFL_BGSysSetBGEnabled(4, TRUE);
    GFL_BGSysSetBGEnabled(5, TRUE);
    GFL_BGSysSetBGEnabled(6, TRUE);
    GFL_BGSysSetBGEnabled(7, TRUE);
    GFL_BGSysClearCharCore(2, 0x20, 0, HEAPID_MOVE_TUTOR);
    GFL_BGSysClearCharCore(4, 0x20, 0, HEAPID_MOVE_TUTOR);
    GFL_BGSysClearCharCore(5, 0x20, 0, HEAPID_MOVE_TUTOR);
    GFL_BGSysClearCharCore(2, 0x20, 0, HEAPID_MOVE_TUTOR);
    GFL_BGSysClearCharCore(4, 0x20, 0, HEAPID_MOVE_TUTOR);
    G2_BlendNone();
    G2S_BlendNone();
}

static void MoveTutor_FreeBG(void) {
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

static void MoveTutor_LoadBG(MoveTutorWork *work, ArcTool *arc) {
    GFL_BGSysLoadArcNCGRStatic(arc, 1, 3, 0, 0, FALSE, HEAPID_MOVE_TUTOR);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 4, 3, 0, 0x800, FALSE, HEAPID_MOVE_TUTOR);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, 0, 0, 0, HEAPID_MOVE_TUTOR);
    GFL_BGSysLoadArcNCGRStatic(arc, 1, 7, 0, 0, FALSE, HEAPID_MOVE_TUTOR);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 2, 7, 0, 0x800, FALSE, HEAPID_MOVE_TUTOR);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, 4, 0, 0, HEAPID_MOVE_TUTOR);
    work->screenBuffer = GFL_G2DIOReadNSCRArc(arc, 3, FALSE, &work->screen, HEAPID_MOVE_TUTOR);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 14 * 0x20, 0x20, HEAPID_MOVE_TUTOR);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 15 * 0x20, 0x20, HEAPID_MOVE_TUTOR);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 15 * 0x20, 0x20, HEAPID_MOVE_TUTOR);
    LoadSysMsgBox(4, 1, 13, 0, HEAPID_MOVE_TUTOR);
}

static void MoveTutor_FreeScreenBuffer(MoveTutorWork *work) {
    GFL_HeapFree(work->screenBuffer);
}

static void MoveTutor_InitWindows(MoveTutorWork *work) {
    u32 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        work->windows[i] = BmpWin_CreateDynamic(sMoveTutorMainWindows[i].bg, sMoveTutorMainWindows[i].x,
                                                sMoveTutorMainWindows[i].y, sMoveTutorMainWindows[i].width,
                                                sMoveTutorMainWindows[i].height, sMoveTutorMainWindows[i].palette, 1);
        GFL_BitmapFill(BmpWin_GetBitmap(work->windows[i]), 0);
    }
    MoveTutor_ClearWindow(work->windows[WINDOW_TEACH]);
    work->keyCursor = KeyCursor_Create(15, TRUE, TRUE, HEAPID_MOVE_TUTOR);
}

static void MoveTutor_FreeWindows(MoveTutorWork *work) {
    u32 i;

    KeyCursor_Free(work->keyCursor);
    for (i = 0; i < WINDOW_COUNT; i++) {
        BmpWin_Free(work->windows[i]);
    }
    BmpWin_FreeAllocator();
}

static void MoveTutor_InitMsg(MoveTutorWork *work) {
    work->msgData =
        GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVOLUTION_DEMO_BTL_MAIN, HEAPID_MOVE_TUTOR);
    work->wordSet = GFL_WordSetSystemCreateDefault(HEAPID_MOVE_TUTOR);
    work->strbuf = GFL_StrBufCreate(0x100, HEAPID_MOVE_TUTOR);
    work->ppLabel = GFL_MsgDataLoadStrbufNew(work->msgData, EvolutionDemoBtlMain_Text_Pp);
    work->ppFormat = GFL_MsgDataLoadStrbufNew(work->msgData, EvolutionDemoBtlMain_Text_Empty_12);
}

static void MoveTutor_FreeMsg(MoveTutorWork *work) {
    GFL_MsgDataFree(work->msgData);
    GFL_WordSetSystemFree(work->wordSet);
    GFL_StrBufFree(work->strbuf);
    GFL_StrBufFree(work->ppLabel);
    GFL_StrBufFree(work->ppFormat);
}

static u32 MoveTutor_SeqFadeWait(MoveTutorWork *work) {
    if (GFL_WipeIsFinished() == TRUE) {
        return work->nextSeq;
    }
    return SEQ_FADE_WAIT;
}

static u32 MoveTutor_SeqSelect(MoveTutorWork *work) {
    u32 pos;

    work->prevPos = CursorMove_GetPos(work->cursor);
    pos = CursorMove_Update(work->cursor);
    switch (pos) {
    case 0:
    case 1:
    case 2:
    case 3:
        if (!GFL_HIDIsTouchPressed()) {
            if (work->param->scroll + pos < work->moveCount) {
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
                return MoveTutor_SeqMoveChosen(work);
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
        if (MoveTutor_AreButtonsShown(work)) {
            return MoveTutor_DecideQuit(work, SEQ_QUIT);
        }
        return MoveTutor_DecideActor(work, SEQ_QUIT);
    case CURSOR_MOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        MoveTutor_ShowButtons(work, FALSE);
        return MoveTutor_DecideActor(work, SEQ_QUIT);
    case POS_TEACH:
        if (MoveTutor_AreButtonsShown(work)) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            return MoveTutor_DecideTeach(work, SEQ_MOVE_CHOSEN);
        }
        break;
    case CURSOR_MOVE_EDGE_UP:
        if (work->param->scroll != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            work->param->scroll--;
            MoveTutor_DrawMoveInfo(work, MoveTutor_GetCursorMove(work));
            MoveTutor_DrawList(work);
            MoveTutor_UpdateArrowPositions(work);
            MoveTutor_AnimateArrows(work, -1);
        }
        break;
    case CURSOR_MOVE_NONE:
    case CURSOR_MOVE_MOVED:
    case CURSOR_MOVE_CURSOR_ON:
        break;
    }
    return SEQ_SELECT;
}

static u32 MoveTutor_SeqMessage(MoveTutorWork *work) {
    u32 state = PrintStream_GetState(work->stream);

    KeyCursor_Update(work->keyCursor, work->stream, work->windows[WINDOW_MESSAGE]);
    if (state == PRINT_STREAM_RUNNING) {
        if (GCTX_HIDGetHeldKeys() & PAD_BUTTON_A) {
            PrintStream_SkipWait(work->stream, 0);
        }
    } else if (state == PRINT_STREAM_DONE) {
        PrintStream_Delete(work->stream);
        return work->nextSeq;
    } else if (state == PRINT_STREAM_PAUSED) {
        if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || GFL_HIDIsTouchPressed()) {
            PrintStream_Continue(work->stream);
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
        }
    }
    return SEQ_MESSAGE;
}

static u32 MoveTutor_SeqYesNoInit(MoveTutorWork *work) {
    AppTaskMenuInit init;

    init.heapId = HEAPID_MOVE_TUTOR;
    init.itemCount = 2;
    init.items = work->yesNoItems;
    init.posType = APP_TASKMENU_POS_TOP_LEFT;
    init.x = 21;
    init.y = 8;
    init.width = 10;
    init.height = 3;
    work->menu = AppTaskMenu_Create(&init, work->menuRes);
    MoveTutor_SetDimmed(TRUE);
    return SEQ_YES_NO;
}

static u32 MoveTutor_SeqYesNo(MoveTutorWork *work) {
    u32 seq = SEQ_YES_NO;

    if (AppTaskMenu_IsFlashFinished(work->menu)) {
        if (AppTaskMenu_GetCursorPos(work->menu) == 0) {
            seq = sMoveTutorYesNoFuncs[work->yesNo].yes(work);
        } else {
            seq = sMoveTutorYesNoFuncs[work->yesNo].no(work);
        }
        MoveTutor_SetDimmed(FALSE);
    }
    AppTaskMenu_Update(work->menu);
    if (seq != SEQ_YES_NO) {
        AppTaskMenu_Free(work->menu);
    }
    return seq;
}

static u32 MoveTutor_SeqSetMove(MoveTutorWork *work) {
    PokeParty_SetParam(work->param->pkm, PKM_PARAM_MOVE1 + work->param->slot, MoveTutor_GetCursorMove(work));
    PokeParty_SetParam(work->param->pkm, PKM_PARAM_MOVE1_PP_UP + work->param->slot, 0);
    PokeParty_SetParam(work->param->pkm, PKM_PARAM_MOVE1_PP + work->param->slot,
                       PML_MoveGetMaxPP(MoveTutor_GetCursorMove(work), 0));
    work->param->status = MOVE_REMINDER_STATUS_LEARNED;
    return SEQ_FADE_OUT;
}

static u32 MoveTutor_SeqForgot(MoveTutorWork *work) {
    MoveTutor_PrintMessage(work, MSG_FORGOT);
    work->nextSeq = SEQ_SET_MOVE;
    return SEQ_MESSAGE;
}

static u32 MoveTutor_SeqStatusResult(MoveTutorWork *work) {
    if (work->param->slot < 4) {
        MoveTutor_PrintMessage(work, MSG_FORGET_CONFIRM);
        work->yesNo = YES_NO_FORGET;
    } else {
        MoveTutor_PrintMessage(work, MSG_GIVE_UP);
        work->yesNo = YES_NO_GIVE_UP;
    }
    work->nextSeq = SEQ_YES_NO_INIT;
    return SEQ_MESSAGE;
}

static u32 MoveTutor_SeqFadeOut(MoveTutorWork *work) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, HEAPID_MOVE_TUTOR);
    work->nextSeq = SEQ_END;
    return SEQ_FADE_WAIT;
}

static void MoveTutor_DrawCursorMoveInfo(MoveTutorWork *work) {
    u32 move = MoveTutor_GetCursorMove(work);

    if (move != MOVE_END) {
        MoveTutor_DrawMoveInfo(work, move);
    } else {
        MoveTutor_DrawMoveInfo(work, NO_MOVE);
    }
    MoveTutor_UpdateListTypes(work);
}

// Draws the work's string buffer into a window
static void MoveTutor_PrintText(MoveTutorWork *work, u32 window, Font *font, u16 color, u32 align, u8 y) {
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
static void MoveTutor_FormatNumber(MoveTutorWork *work, u32 msg, u32 number, u32 digits, u8 pad) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(work->msgData, msg);

    WordSetNumber(work->wordSet, 0, number, digits, pad, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, work->strbuf, str);
    GFL_StrBufFree(str);
}

// Draws the labels, and on the top screen the Pokémon's name, level and moves
static void MoveTutor_DrawText(MoveTutorWork *work) {
    StrBuf *str;
    MsgData *moveNames;
    BoxPkm *boxPkm;
    u32 i;
    u16 move;

    GFL_MsgDataLoadStrbuf(work->msgData, EvolutionDemoBtlMain_Text_Teach, work->strbuf);
    MoveTutor_PrintText(work, WINDOW_TEACH, work->font, PRINT_COLOR(15, 2, 0), ALIGN_CENTER, 4);
    GFL_MsgDataLoadStrbuf(work->msgData, EvolutionDemoBtlMain_Text_Power, work->strbuf);
    MoveTutor_PrintText(work, WINDOW_POWER_LABEL, work->font, PRINT_COLOR(1, 2, 0), ALIGN_LEFT, 0);
    BmpWin_Transfer(work->windows[WINDOW_POWER_LABEL]);
    GFL_MsgDataLoadStrbuf(work->msgData, EvolutionDemoBtlMain_Text_Accuracy, work->strbuf);
    MoveTutor_PrintText(work, WINDOW_ACCURACY_LABEL, work->font, PRINT_COLOR(1, 2, 0), ALIGN_LEFT, 0);
    BmpWin_Transfer(work->windows[WINDOW_ACCURACY_LABEL]);

    str = GFL_StrBufCreate(0x100, HEAPID_MOVE_TUTOR);
    GFL_MsgDataLoadStrbuf(work->msgData, EvolutionDemoBtlMain_Text_LearnedMoves, str);
    loadPokemonNicknameToStrbuf(work->wordSet, 0, work->param->pkm);
    GFL_WordSetFormatStrbuf(work->wordSet, work->strbuf, str);
    MoveTutor_PrintText(work, WINDOW_TITLE, work->font, PRINT_COLOR(15, 2, 0), ALIGN_LEFT, 4);
    BmpWin_Transfer(work->windows[WINDOW_TITLE]);

    moveNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_MOVE_NAMES, HEAPID_MOVE_TUTOR);
    boxPkm = PokeParty_ToBoxPkm(work->param->pkm);
    for (i = 0; i < 4; i++) {
        move = PML_PkmGetParam(boxPkm, PKM_PARAM_MOVE1 + i, NULL);
        if (move == 0) {
            ClActor_SetVisible(work->actors[ACTOR_KNOWN_TYPE + i], FALSE);
        } else {
            ClActor_SetVisible(work->actors[ACTOR_KNOWN_TYPE + i], TRUE);
            MoveTutor_SetKnownMoveType(work, move, i);
            GFL_MsgDataLoadStrbuf(moveNames, move, work->strbuf);
            MoveTutor_PrintText(work, WINDOW_KNOWN_MOVES, work->font, PRINT_COLOR(15, 2, 0), ALIGN_LEFT, i * 32);
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

    GFL_MsgDataLoadStrbuf(work->msgData, EvolutionDemoBtlMain_Text_Empty_16, str);
    setPartyPokemonSpeciesNameToStrbuf(work->wordSet, 0, work->param->pkm);
    GFL_WordSetFormatStrbuf(work->wordSet, work->strbuf, str);
    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(work->windows[WINDOW_POKEMON]), 0, 0, work->strbuf, work->font,
                                   PRINT_COLOR(1, 2, 0));
    BmpWin_Transfer(work->windows[WINDOW_POKEMON]);
    GFL_MsgDataLoadStrbuf(work->msgData, EvolutionDemoBtlMain_Text_Lv, str);
    WordSetNumber(work->wordSet, 0, PML_PkmGetParam(boxPkm, PKM_PARAM_LEVEL, NULL), 3, 1, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, work->strbuf, str);
    MoveTutor_PrintText(work, WINDOW_POKEMON, work->font, PRINT_COLOR(1, 2, 0), ALIGN_RIGHT, 16);
    BmpWin_Transfer(work->windows[WINDOW_POKEMON]);

    GFL_MsgDataFree(moveNames);
    GFL_StrBufFree(str);
}

static u32 MoveTutor_CountMoves(MoveTutorWork *work) {
    u32 i;

    for (i = 0; i < 256; i++) {
        if (work->param->moves[i] == MOVE_END) {
            break;
        }
    }
    return i;
}

static void MoveTutor_DrawListRow(MoveTutorWork *work, u8 scroll, u8 row) {
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

static void MoveTutor_DrawList(MoveTutorWork *work) {
    u32 i;

    GFL_BitmapFill(BmpWin_GetBitmap(work->windows[WINDOW_LIST]), 0);
    for (i = 0; i < LIST_ROWS; i++) {
        MoveTutor_DrawListRow(work, work->param->scroll, i);
    }
    BmpWin_Transfer(work->windows[WINDOW_LIST]);
    GFL_BGSysQueueScrLoad(7);
    MoveTutor_UpdateListTypes(work);
}

static void MoveTutor_InitList(MoveTutorWork *work) {
    MsgData *moveNames;
    u32 i;

    work->moveCount = MoveTutor_CountMoves(work);
    work->moveList = ListMenuCore_CreateOptionList(work->moveCount, HEAPID_MOVE_TUTOR);
    moveNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_MOVE_NAMES, HEAPID_MOVE_TUTOR);
    for (i = 0; i < work->moveCount; i++) {
        if (work->param->moves[i] != MOVE_END) {
            ListMenuCore_AppendMsgOption(work->moveList, moveNames, work->param->moves[i], work->param->moves[i],
                                         HEAPID_MOVE_TUTOR);
        } else {
            ListMenuCore_AppendMsgOption(work->moveList, work->msgData, EvolutionDemoBtlMain_Text_Empty_13, -2,
                                         HEAPID_MOVE_TUTOR);
            break;
        }
    }
    GFL_MsgDataFree(moveNames);
    work->unk1BB = 0;
    work->unk1BC = work->param->cursorPos;
    MoveTutor_DrawList(work);
}

static void MoveTutor_FreeList(MoveTutorWork *work) {
    ListMenuCore_FreeOptionList(work->moveList);
}

// Draws a move's power, accuracy, description and category, or clears them for NO_MOVE
static void MoveTutor_DrawMoveInfo(MoveTutorWork *work, u32 move) {
    u32 power;
    u32 accuracy;
    MsgData *descriptions;

    GFL_BitmapFill(BmpWin_GetBitmap(work->windows[WINDOW_DESCRIPTION]), 0);
    GFL_BitmapFill(BmpWin_GetBitmap(work->windows[WINDOW_POWER]), 0);
    GFL_BitmapFill(BmpWin_GetBitmap(work->windows[WINDOW_ACCURACY]), 0);
    ClActor_SetVisible(work->actors[ACTOR_CATEGORY], FALSE);
    if (move != NO_MOVE) {
        power = PML_MoveGetParam(move, MOVE_PARAM_POWER);
        if (power <= 1) {
            GFL_MsgDataLoadStrbuf(work->msgData, EvolutionDemoBtlMain_Text_Empty_14, work->strbuf);
        } else {
            MoveTutor_FormatNumber(work, EvolutionDemoBtlMain_Text_Empty_10, power, 3, 0);
        }
        MoveTutor_PrintText(work, WINDOW_POWER, work->font, PRINT_COLOR(1, 2, 0), ALIGN_RIGHT, 0);
        accuracy = PML_MoveGetParam(move, MOVE_PARAM_ACCURACY);
        if (accuracy == 0 || PML_MoveIsAlwaysHit(move)) {
            GFL_MsgDataLoadStrbuf(work->msgData, EvolutionDemoBtlMain_Text_Empty_14, work->strbuf);
        } else {
            MoveTutor_FormatNumber(work, EvolutionDemoBtlMain_Text_Empty_11, accuracy, 3, 0);
        }
        MoveTutor_PrintText(work, WINDOW_ACCURACY, work->font, PRINT_COLOR(1, 2, 0), ALIGN_RIGHT, 0);
        descriptions = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_13, HEAPID_MOVE_TUTOR);
        GFL_MsgDataLoadStrbuf(descriptions, move, work->strbuf);
        MoveTutor_PrintText(work, WINDOW_DESCRIPTION, work->font, PRINT_COLOR(1, 2, 0), ALIGN_LEFT, 0);
        GFL_MsgDataFree(descriptions);
        MoveTutor_SetCategory(work, move);
        ClActor_SetVisible(work->actors[ACTOR_CATEGORY], TRUE);
        BmpWin_Transfer(work->windows[WINDOW_POWER_LABEL]);
        BmpWin_Transfer(work->windows[WINDOW_ACCURACY_LABEL]);
    } else {
        MoveTutor_ClearWindow(work->windows[WINDOW_POWER_LABEL]);
        MoveTutor_ClearWindow(work->windows[WINDOW_ACCURACY_LABEL]);
    }
    BmpWin_Transfer(work->windows[WINDOW_DESCRIPTION]);
    BmpWin_Transfer(work->windows[WINDOW_POWER]);
    BmpWin_Transfer(work->windows[WINDOW_ACCURACY]);
}

// Formats one of the mode's messages into the work's string buffer
static void MoveTutor_FormatMessage(MoveTutorWork *work, u32 msg) {
    StrBuf *str;

    switch (msg) {
    case MSG_SELECT:
        loadPokemonNicknameToStrbuf(work->wordSet, 0, work->param->pkm);
        break;
    case MSG_LEARN:
        loadMoveNameToStrbuf(work->wordSet, 1, MoveTutor_GetCursorMove(work));
        break;
    case MSG_QUIT:
        loadPokemonNicknameToStrbuf(work->wordSet, 0, work->param->pkm);
        break;
    case MSG_LEARNED:
        loadPokemonNicknameToStrbuf(work->wordSet, 0, work->param->pkm);
        loadMoveNameToStrbuf(work->wordSet, 1, MoveTutor_GetCursorMove(work));
        break;
    case MSG_REPLACE:
        loadPokemonNicknameToStrbuf(work->wordSet, 0, work->param->pkm);
        loadMoveNameToStrbuf(work->wordSet, 1, MoveTutor_GetCursorMove(work));
        break;
    case MSG_FORGET:
        loadPokemonNicknameToStrbuf(work->wordSet, 0, work->param->pkm);
        loadMoveNameToStrbuf(work->wordSet, 1, MoveTutor_GetReplacedMove(work));
        break;
    case MSG_FORGOT:
        loadPokemonNicknameToStrbuf(work->wordSet, 0, work->param->pkm);
        loadMoveNameToStrbuf(work->wordSet, 1, MoveTutor_GetCursorMove(work));
        break;
    case MSG_GIVE_UP:
        loadMoveNameToStrbuf(work->wordSet, 1, MoveTutor_GetCursorMove(work));
        break;
    case MSG_NOT_LEARNED:
        loadPokemonNicknameToStrbuf(work->wordSet, 0, work->param->pkm);
        loadMoveNameToStrbuf(work->wordSet, 1, MoveTutor_GetCursorMove(work));
        break;
    case MSG_PAID:
        copyVarForText(work->wordSet, 2, work->param->playerInfo);
        break;
    case MSG_FORGET_CONFIRM:
        loadMoveNameToStrbuf(work->wordSet, 0, MoveTutor_GetReplacedMove(work));
        break;
    }
    str = GFL_MsgDataLoadStrbufNew(work->msgData, sMoveTutorMessages[work->param->mode][msg]);
    GFL_WordSetFormatStrbuf(work->wordSet, work->strbuf, str);
    GFL_StrBufFree(str);
}

static u16 MoveTutor_GetCursorMove(MoveTutorWork *work) {
    return work->param->moves[work->param->scroll + work->param->cursorPos];
}

static u16 MoveTutor_GetReplacedMove(MoveTutorWork *work) {
    return PokeParty_GetParam(work->param->pkm, PKM_PARAM_MOVE1 + work->param->slot, NULL);
}

static void MoveTutor_PrintMessage(MoveTutorWork *work, u32 msg) {
    GFL_BitmapFill(BmpWin_GetBitmap(work->windows[WINDOW_MESSAGE]), 15);
    BmpWin_DrawFrame(work->windows[WINDOW_MESSAGE], 0, 1, 13);
    MoveTutor_FormatMessage(work, msg);
    work->stream = PrintStream_CreateWithCallback(work->windows[WINDOW_MESSAGE], 0, 0, work->strbuf, work->font,
                                                  TextSpeed_GetCurrentWait(), work->tcbManager, 0, HEAPID_MOVE_TUTOR,
                                                  0xffff, MoveTutor_StreamCallback);
    BmpWin_TransferNow(work->windows[WINDOW_MESSAGE]);
}

static void MoveTutor_ClearMessage(MoveTutorWork *work) {
    BmpWin_ClearFrame(work->windows[WINDOW_MESSAGE], 2);
    MoveTutor_ClearWindow(work->windows[WINDOW_MESSAGE]);
}

// The messages' events: wait for the sounds, play the forgetting sound, and play the fanfare and wait for it
static BOOL MoveTutor_StreamCallback(u32 event) {
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

static u8 MoveTutor_CountKnownMoves(MoveTutorWork *work) {
    u8 i;

    for (i = 0; i < 4; i++) {
        if (PokeParty_GetParam(work->param->pkm, PKM_PARAM_MOVE1 + i, NULL) == 0) {
            break;
        }
    }
    return i;
}

static u32 MoveTutor_LearnYes(MoveTutorWork *work) {
    MoveTutor_PrintMessage(work, MSG_LEARNED);
    work->param->slot = MoveTutor_CountKnownMoves(work);
    work->nextSeq = SEQ_SET_MOVE;
    return SEQ_MESSAGE;
}

static u32 MoveTutor_LearnNo(MoveTutorWork *work) {
    MoveTutor_ClearMessage(work);
    return SEQ_SELECT;
}

static u32 MoveTutor_QuitYes(MoveTutorWork *work) {
    work->param->status = MOVE_REMINDER_STATUS_CANCELLED;
    return SEQ_FADE_OUT;
}

static u32 MoveTutor_QuitNo(MoveTutorWork *work) {
    MoveTutor_ClearMessage(work);
    return SEQ_SELECT;
}

static u32 MoveTutor_ReplaceYes(MoveTutorWork *work) {
    work->nextSeq = SEQ_STATUS_FADE_OUT;
    return SEQ_FADE_WAIT;
}

static u32 MoveTutor_ReplaceNo(MoveTutorWork *work) {
    MoveTutor_PrintMessage(work, MSG_GIVE_UP);
    work->yesNo = YES_NO_GIVE_UP;
    work->nextSeq = SEQ_YES_NO_INIT;
    return SEQ_MESSAGE;
}

static u32 MoveTutor_GiveUpYes(MoveTutorWork *work) {
    MoveTutor_PrintMessage(work, MSG_NOT_LEARNED);
    work->nextSeq = SEQ_FADE_OUT;
    work->param->status = MOVE_REMINDER_STATUS_CANCELLED;
    return SEQ_MESSAGE;
}

static u32 MoveTutor_GiveUpNo(MoveTutorWork *work) {
    MoveTutor_PrintMessage(work, MSG_REPLACE);
    work->yesNo = YES_NO_REPLACE;
    work->nextSeq = SEQ_YES_NO_INIT;
    return SEQ_MESSAGE;
}

static u32 MoveTutor_ForgetYes(MoveTutorWork *work) {
    MoveTutor_PrintMessage(work, MSG_FORGET);
    work->nextSeq = SEQ_FORGOT;
    return SEQ_MESSAGE;
}

static u32 MoveTutor_ForgetNo(MoveTutorWork *work) {
    work->nextSeq = SEQ_STATUS_FADE_OUT;
    return SEQ_FADE_WAIT;
}

static void MoveTutor_InitActorSys(MoveTutorWork *work) {
    ClActSys_Create(&sMoveTutorActorSysSetup, &sMoveTutorVRAMConfig, HEAPID_MOVE_TUTOR);
    work->unit = ClActUnit_Create(ACTOR_COUNT, 0, HEAPID_MOVE_TUTOR);
    ClActUnit_UseSystemRenderer(work->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void MoveTutor_FreeActors(MoveTutorWork *work) {
    u32 i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        ClActor_Delete(work->actors[i]);
    }
    ClActUnit_Delete(work->unit);
    ClActSys_Delete();
    GFL_ArcToolFree(work->commonArc);
}

static void MoveTutor_LoadActorResources(MoveTutorWork *work, ArcTool *arc) {
    ArcTool *commonArc;
    ArcTool *pokeArc;
    BoxPkm *boxPkm;
    BOOL wasEncrypted;
    u32 i;

    commonArc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_MOVE_TUTOR);
    work->chars[0] =
        ClActSys_LoadChars(commonArc, AppMenuCommon_GetTouchBarCharFile(), FALSE, CLACT_VRAM_SUB, HEAPID_MOVE_TUTOR);
    work->chars[1] = ClActSys_LoadChars(arc, 17, FALSE, CLACT_VRAM_SUB, HEAPID_MOVE_TUTOR);
    work->chars[2] = ClActSys_LoadChars(arc, 26, FALSE, CLACT_VRAM_SUB, HEAPID_MOVE_TUTOR);
    for (i = 4; i < 8; i++) {
        work->chars[i] = ClActSys_LoadChars(commonArc, AppMenuCommon_GetTypeIconCharFile(0), FALSE, CLACT_VRAM_MAIN,
                                            HEAPID_MOVE_TUTOR);
    }
    work->chars[3] = ClActSys_LoadChars(arc, 23, FALSE, CLACT_VRAM_SUB, HEAPID_MOVE_TUTOR);
    work->palettes[0] = ClActSys_LoadPlttRange(commonArc, AppMenuCommon_GetTouchBarPaletteFile(), CLACT_VRAM_SUB, 0, 0,
                                               4, HEAPID_MOVE_TUTOR);
    work->palettes[1] = ClActSys_LoadPlttRange(arc, 5, CLACT_VRAM_SUB, 4 * 0x20, 0, 1, HEAPID_MOVE_TUTOR);
    work->palettes[2] = ClActSys_LoadPlttRange(commonArc, AppMenuCommon_GetTypeIconPaletteFile(), CLACT_VRAM_SUB,
                                               6 * 0x20, 0, 3, HEAPID_MOVE_TUTOR);
    work->palettes[3] =
        ClActSys_LoadPltt(commonArc, AppMenuCommon_GetTypeIconPaletteFile(), CLACT_VRAM_MAIN, 0, HEAPID_MOVE_TUTOR);
    work->cellAnims[0] = ClActSys_LoadCellAnims(commonArc, AppMenuCommon_GetTouchBarCellFile(1),
                                                AppMenuCommon_GetTouchBarAnimFile(1), HEAPID_MOVE_TUTOR);
    work->cellAnims[1] = ClActSys_LoadCellAnims(arc, 7, 8, HEAPID_MOVE_TUTOR);
    work->cellAnims[2] = ClActSys_LoadCellAnims(commonArc, AppMenuCommon_GetTypeIconCellFile(1),
                                                AppMenuCommon_GetTypeIconAnimFile(1), HEAPID_MOVE_TUTOR);
    work->cellAnims[3] = ClActSys_LoadCellAnims(arc, 24, 25, HEAPID_MOVE_TUTOR);
    work->cellAnims[4] = ClActSys_LoadCellAnims(arc, 27, 28, HEAPID_MOVE_TUTOR);
    GFL_ArcToolFree(commonArc);

    pokeArc = MakePokeGraArcHandle(HEAPID_MOVE_TUTOR);
    boxPkm = PokeParty_GetBoxPkm(work->param->pkm);
    wasEncrypted = PML_PkmDecrypt(boxPkm);
    work->chars[8] =
        PokeGra_LoadClActCharsByBoxData(pokeArc, boxPkm, POKEGRA_DIR_FRONT, CLACT_VRAM_MAIN, HEAPID_MOVE_TUTOR);
    work->palettes[4] = PokeGra_LoadClActPaletteByBoxData(pokeArc, boxPkm, POKEGRA_DIR_FRONT, CLACT_VRAM_MAIN, 4 * 0x20,
                                                          HEAPID_MOVE_TUTOR);
    work->cellAnims[5] =
        PokeGra_LoadClActCellAnimsByBoxData(boxPkm, POKEGRA_DIR_FRONT, 1, CLACT_VRAM_MAIN, HEAPID_MOVE_TUTOR);
    PML_PkmReEncrypt(boxPkm, wasEncrypted);
    GFL_ArcToolFree(pokeArc);
}

static void MoveTutor_ReplaceChars(ArcTool *arc, u32 chars, u32 fileId, BOOL compressed, HeapID heapId) {
    NNSG2dCharacterData *character;
    void *buffer = GFL_G2DIOReadOBJNCGRArc(arc, fileId, compressed, &character, heapId);

    ClActSys_ReplaceCharData(chars, character);
    GFL_HeapFree(buffer);
}

// Gives the icon of one of the Pokémon's moves its type, loaded at the next VBlank
static void MoveTutor_SetKnownMoveType(MoveTutorWork *work, u16 move, u16 index) {
    u32 type = PML_MoveGetParam(move, MOVE_PARAM_TYPE);

    work->typeIcons[index].type = type;
    work->typeIcons[index].dirty = TRUE;
    ClActor_SetPalette(work->actors[ACTOR_KNOWN_TYPE + index], AppMenuCommon_GetTypeIconPaletteIndex(type), 1);
}

static void MoveTutor_LoadTypeIcons(MoveTutorWork *work) {
    int i;

    for (i = 0; i < TYPE_ICON_COUNT; i++) {
        if (work->typeIcons[i].dirty) {
            MoveTutor_ReplaceChars(work->commonArc, work->chars[4 + i],
                                   AppMenuCommon_GetTypeIconCharFile(work->typeIcons[i].type), FALSE,
                                   HEAPID_MOVE_TUTOR);
            work->typeIcons[i].dirty = FALSE;
        }
    }
}

// Places the list's type icons and sets them to the types of the moves shown
static void MoveTutor_UpdateListTypes(MoveTutorWork *work) {
    u32 i;
    ClActorPos pos;
    u32 type;

    for (i = 0; i < LIST_ROWS; i++) {
        pos.x = 47;
        pos.y = i * 24 + 80;
        ClActor_SetSurfacePos(work->actors[ACTOR_LIST_TYPE + i], &pos, CLACT_SURFACE_SUB);
        if (work->param->scroll + i >= work->moveCount) {
            ClActor_SetVisible(work->actors[ACTOR_LIST_TYPE + i], FALSE);
        } else {
            type = PML_MoveGetParam(work->param->moves[work->param->scroll + i], MOVE_PARAM_TYPE);
            ClActor_SetVisible(work->actors[ACTOR_LIST_TYPE + i], TRUE);
            ClActor_SetSequence(work->actors[ACTOR_LIST_TYPE + i], type);
        }
    }
}

static void MoveTutor_SetCategory(MoveTutorWork *work, u16 move) {
    ClActor_SetSequence(work->actors[ACTOR_CATEGORY], PML_MoveGetParam(move, MOVE_PARAM_CATEGORY));
}

static const MoveTutorActorData sMoveTutorActors[ACTOR_COUNT] = {
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

static void MoveTutor_InitActors(MoveTutorWork *work, ArcTool *arc) {
    u32 i;
    ClActorSetup setup;

    MoveTutor_InitActorSys(work);
    MoveTutor_LoadActorResources(work, arc);
    work->commonArc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_MOVE_TUTOR);
    for (i = 0; i < ACTOR_COUNT; i++) {
        setup.x = sMoveTutorActors[i].x;
        setup.y = sMoveTutorActors[i].y;
        setup.sequence = sMoveTutorActors[i].sequence;
        setup.priority = sMoveTutorActors[i].priority;
        setup.bgPriority = sMoveTutorActors[i].bgPriority;
        work->actors[i] = ClActor_Create(
            work->unit, work->chars[sMoveTutorActors[i].chars], work->palettes[sMoveTutorActors[i].palette],
            work->cellAnims[sMoveTutorActors[i].cellAnims], &setup, sMoveTutorActors[i].surface, HEAPID_MOVE_TUTOR);
        ClActor_SetAutoAnim(work->actors[i], FALSE);
    }
    ClActor_SetAutoAnim(work->actors[ACTOR_RETURN], TRUE);
    ClActor_SetAutoAnim(work->actors[ACTOR_CURSOR], TRUE);
    ClActor_SetAutoAnim(work->actors[ACTOR_ARROW_UP], TRUE);
    ClActor_SetAutoAnim(work->actors[ACTOR_ARROW_DOWN], TRUE);
}

// Moves the cursor's actor to a row of the list, an arrow or the buttons, with the frame for each
static void MoveTutor_SetCursorActor(MoveTutorWork *work, u8 pos, u32 unused) {
    ClActorPos actorPos;

    if (pos < LIST_ROWS) {
        actorPos.x = 130;
        actorPos.y = pos * 24 + 84;
        ClActor_SetSurfacePos(work->actors[ACTOR_CURSOR], &actorPos, CLACT_SURFACE_SUB);
        ClActor_SetAnimTime(work->actors[ACTOR_CURSOR], 0);
    } else if (pos < POS_RETURN) {
        actorPos.x = (pos - POS_ARROW_DOWN) * 40;
        actorPos.y = 168;
        ClActor_SetSurfacePos(work->actors[ACTOR_CURSOR], &actorPos, CLACT_SURFACE_SUB);
        ClActor_SetAnimTime(work->actors[ACTOR_CURSOR], 1);
    } else {
        actorPos.x = 192;
        actorPos.y = 160;
        ClActor_SetSurfacePos(work->actors[ACTOR_CURSOR], &actorPos, CLACT_SURFACE_SUB);
        ClActor_SetAnimTime(work->actors[ACTOR_CURSOR], 2);
    }
}

static u32 MoveTutor_SeqStatusFadeOut(MoveTutorWork *work) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, HEAPID_MOVE_TUTOR);
    work->nextSeq = SEQ_STATUS_CALL;
    return SEQ_FADE_WAIT;
}

// Ends the screen's graphics and runs the summary screen to pick the move to forget
static u32 MoveTutor_SeqStatusCall(MoveTutorWork *work) {
    MoveTutor_FreeScreen(work);
    work->status.party = work->param->pkm;
    work->status.trainerData = work->param->trainerData;
    work->status.gameData = GSYS_GetGameData(work->param->gsys);
    work->status.dataType = PSTATUS_DATA_PARTY_PKM;
    work->status.partyIndex = 0;
    work->status.partyCount = 1;
    work->status.move = MoveTutor_GetCursorMove(work);
    work->status.mode = PSTATUS_MODE_FORGET_MOVE;
    work->status.page = PSTATUS_PAGE_INFO;
    work->status.isNationalDex = FALSE;
    GSYS_QueueProc(work->param->gsys, OVERLAY_PSTATUS, &PSTATUS_PROC_FUNCTIONS, &work->status);
    return SEQ_STATUS_RETURN;
}

static u32 MoveTutor_SeqStatusReturn(MoveTutorWork *work) {
    MoveTutor_InitScreen(work);
    MoveTutor_SetCursorActor(work, work->param->cursorPos, 3);
    GFL_NetSetIconScreen(TRUE, HEAPID_MOVE_TUTOR);
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
static const CursorMoveData sMoveTutorCursorData[] = {
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

static const CursorMoveCallbacks sMoveTutorCursorCallbacks = {
    MoveTutor_CursorDummy,
    MoveTutor_CursorDummy,
    MoveTutor_CursorMove,
    MoveTutor_CursorTouch,
};

static void MoveTutor_InitCursor(MoveTutorWork *work) {
    work->cursor =
        CursorMove_Create(sMoveTutorCursorData, &sMoveTutorCursorCallbacks, work, TRUE, 0, HEAPID_MOVE_TUTOR);
    CursorMove_DisablePos(work->cursor, POS_QUIT);
    MoveTutor_UpdateArrowPositions(work);
}

static void MoveTutor_FreeCursor(MoveTutorWork *work) {
    CursorMove_Delete(work->cursor);
}

static void MoveTutor_CursorDummy(void *data, int pos, int prevPos) {
}

// The cursor moved with the keys: onto a row, or past the list's ends, which scrolls it
static void MoveTutor_CursorMove(void *data, int pos, int prevPos) {
    MoveTutorWork *work = data;

    MoveTutor_ShowButtons(work, FALSE);
    if (pos <= 3) {
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        work->param->cursorPos = pos;
        if (work->param->scroll + work->param->cursorPos < work->moveCount) {
            MoveTutor_DrawMoveInfo(work, MoveTutor_GetCursorMove(work));
        } else {
            MoveTutor_DrawMoveInfo(work, NO_MOVE);
        }
    } else if (pos == POS_RETURN) {
        if (prevPos == 3 && work->param->scroll + LIST_ROWS < work->moveCount) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            pos = 3;
            CursorMove_SetPos(work->cursor, pos);
            work->param->scroll++;
            MoveTutor_DrawList(work);
            MoveTutor_DrawMoveInfo(work, MoveTutor_GetCursorMove(work));
            MoveTutor_UpdateArrowPositions(work);
            MoveTutor_AnimateArrows(work, 1);
        } else if (prevPos == 0) {
            if (work->param->scroll != 0) {
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
                work->param->scroll--;
                MoveTutor_DrawList(work);
                MoveTutor_DrawMoveInfo(work, MoveTutor_GetCursorMove(work));
                MoveTutor_UpdateArrowPositions(work);
                MoveTutor_AnimateArrows(work, -1);
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
    MoveTutor_SetCursorActor(work, pos, 3);
}

// A position was touched: a row, which shows the buttons, or an arrow, which scrolls the list
static void MoveTutor_CursorTouch(void *data, int pos, int prevPos) {
    MoveTutorWork *work = data;

    if (pos <= 3) {
        work->param->cursorPos = pos;
        if (work->param->scroll + work->param->cursorPos < work->moveCount) {
            MoveTutor_ShowButtons(work, TRUE);
            MoveTutor_DrawMoveInfo(work, MoveTutor_GetCursorMove(work));
        } else {
            MoveTutor_ShowButtons(work, FALSE);
            MoveTutor_DrawMoveInfo(work, NO_MOVE);
        }
    } else if (pos == POS_ARROW_DOWN) {
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        pos = work->param->cursorPos;
        work->param->scroll++;
        MoveTutor_DrawMoveInfo(work, MoveTutor_GetCursorMove(work));
        MoveTutor_DrawList(work);
        MoveTutor_ShowButtons(work, FALSE);
        MoveTutor_UpdateArrowPositions(work);
        MoveTutor_AnimateArrows(work, 1);
        CursorMove_SetPos(work->cursor, pos);
    } else if (pos == POS_ARROW_UP) {
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        pos = work->param->cursorPos;
        work->param->scroll--;
        MoveTutor_DrawMoveInfo(work, MoveTutor_GetCursorMove(work));
        MoveTutor_DrawList(work);
        MoveTutor_ShowButtons(work, FALSE);
        MoveTutor_UpdateArrowPositions(work);
        MoveTutor_AnimateArrows(work, -1);
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
    MoveTutor_SetCursorActor(work, pos, 3);
}

// Shows or removes the Teach and Quit buttons
static void MoveTutor_ShowButtons(MoveTutorWork *work, BOOL show) {
    if (show == TRUE) {
        if (work->buttons[BUTTON_TEACH] == NULL) {
            work->buttons[BUTTON_TEACH] =
                AppTaskMenuWin_Create(work->menuRes, &work->buttonItems[BUTTON_TEACH], 13, 21, 9, HEAPID_MOVE_TUTOR);
            work->buttons[BUTTON_QUIT] =
                AppTaskMenuWin_Create(work->menuRes, &work->buttonItems[BUTTON_QUIT], 22, 21, 9, HEAPID_MOVE_TUTOR);
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

static BOOL MoveTutor_AreButtonsShown(MoveTutorWork *work) {
    if (work->buttons[BUTTON_TEACH] != NULL) {
        return TRUE;
    }
    return FALSE;
}

// Lets the cursor onto the arrows that can scroll the list
static void MoveTutor_UpdateArrowPositions(MoveTutorWork *work) {
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
static void MoveTutor_AnimateArrows(MoveTutorWork *work, int direction) {
    ClActor_SetAnimTime(work->actors[ACTOR_ARROW_UP], 0);
    ClActor_SetAnimTime(work->actors[ACTOR_ARROW_DOWN], 0);
    if (direction > 0) {
        if (work->param->scroll + LIST_ROWS < work->moveCount) {
            ClActor_SetSequence(work->actors[ACTOR_ARROW_DOWN], 10);
        } else {
            ClActor_SetSequence(work->actors[ACTOR_ARROW_DOWN], 23);
        }
        ClActor_SetSequence(work->actors[ACTOR_ARROW_UP], 3);
    } else {
        if (work->param->scroll == 0) {
            ClActor_SetSequence(work->actors[ACTOR_ARROW_UP], 24);
        } else {
            ClActor_SetSequence(work->actors[ACTOR_ARROW_UP], 11);
        }
        ClActor_SetSequence(work->actors[ACTOR_ARROW_DOWN], 2);
    }
}

static void MoveTutor_InitArrows(MoveTutorWork *work) {
    if (work->moveCount <= LIST_ROWS) {
        ClActor_SetSequence(work->actors[ACTOR_ARROW_UP], 17);
        ClActor_SetSequence(work->actors[ACTOR_ARROW_DOWN], 16);
    } else if (work->param->scroll + LIST_ROWS < work->moveCount) {
        ClActor_SetSequence(work->actors[ACTOR_ARROW_UP], 17);
        ClActor_SetSequence(work->actors[ACTOR_ARROW_DOWN], 2);
    } else if (work->param->scroll == 0) {
        ClActor_SetSequence(work->actors[ACTOR_ARROW_UP], 3);
        ClActor_SetSequence(work->actors[ACTOR_ARROW_DOWN], 16);
    }
}

// Start the animation of the return icon or the flash of a button, after which the process goes to seq
static u32 MoveTutor_DecideActor(MoveTutorWork *work, u16 seq) {
    MoveTutorDecide *decide = &work->decide;

    decide->type = DECIDE_ACTOR;
    decide->index = ACTOR_RETURN;
    decide->seq = seq;
    ClActor_SetSequence(work->actors[ACTOR_RETURN], 9);
    return SEQ_DECIDE_WAIT;
}

static u32 MoveTutor_DecideTeach(MoveTutorWork *work, u16 seq) {
    MoveTutorDecide *decide = &work->decide;

    decide->type = DECIDE_BUTTON;
    decide->index = BUTTON_TEACH;
    decide->seq = seq;
    AppTaskMenuWin_SetFlashing(work->buttons[decide->index], TRUE);
    return SEQ_DECIDE_WAIT;
}

static u32 MoveTutor_DecideQuit(MoveTutorWork *work, u16 seq) {
    MoveTutorDecide *decide = &work->decide;

    decide->type = DECIDE_BUTTON;
    decide->index = BUTTON_QUIT;
    decide->seq = seq;
    AppTaskMenuWin_SetFlashing(work->buttons[decide->index], TRUE);
    return SEQ_DECIDE_WAIT;
}

static u32 MoveTutor_SeqDecideWait(MoveTutorWork *work) {
    MoveTutorDecide *decide = &work->decide;

    if (decide->type == DECIDE_ACTOR) {
        if (!ClActor_IsAnimActive(work->actors[decide->index])) {
            return decide->seq;
        }
    } else if (decide->type == DECIDE_BUTTON) {
        if (AppTaskMenuWin_IsFlashFinished(work->buttons[decide->index])) {
            MoveTutor_ShowButtons(work, FALSE);
            return decide->seq;
        }
    }
    return SEQ_DECIDE_WAIT;
}

static u32 MoveTutor_SeqQuit(MoveTutorWork *work) {
    MoveTutor_PrintMessage(work, MSG_QUIT);
    work->yesNo = YES_NO_QUIT;
    work->nextSeq = SEQ_YES_NO_INIT;
    return SEQ_MESSAGE;
}

static u32 MoveTutor_SeqMoveChosen(MoveTutorWork *work) {
    u8 yesNo;

    if (MoveTutor_CountKnownMoves(work) < 4) {
        MoveTutor_PrintMessage(work, MSG_LEARN);
        yesNo = YES_NO_LEARN;
    } else {
        MoveTutor_PrintMessage(work, MSG_REPLACE);
        yesNo = YES_NO_REPLACE;
    }
    work->yesNo = yesNo;
    work->nextSeq = SEQ_YES_NO_INIT;
    return SEQ_MESSAGE;
}

// Darkens the touch screen behind the yes/no menu
static void MoveTutor_SetDimmed(BOOL dimmed) {
    if (dimmed == TRUE) {
        G2x_SetBlendBrightness_(
            REG_DB_BLDCNT_ADDR,
            GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ, -8);
    } else {
        G2S_BlendNone();
    }
}
