#include "types.h"
#include "app/spin_trade.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "field/union_app.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/net_system.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "nitro/gx.h"
#include "nnsys/g2d.h"
#include "save/player_info.h"
#include "system/bmp_menu.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/btl_pokeparam.h"

// The Spin Trade's receipt screen, where the parent waits for the group to gather and closes it, and the others wait
// for the parent. The ROM has no name for the file; it is named after pokeplatinum's spin_trade_receipt.c, the same
// screen in Gen 4. Names are ours

// The proc's sequences
enum {
    SPIN_TRADE_RECEIPT_PROC_FADE_IN,
    SPIN_TRADE_RECEIPT_PROC_MAIN,
    SPIN_TRADE_RECEIPT_PROC_MAIN_NO_UPDATE,
    SPIN_TRADE_RECEIPT_PROC_FADE_OUT,
    SPIN_TRADE_RECEIPT_PROC_WAIT_START,
};

// The cell actor resources: a palette and the characters of each member's trainer, and the cells
enum {
    SPIN_TRADE_RECEIPT_RES_PLTT,
    SPIN_TRADE_RECEIPT_RES_CHAR = SPIN_TRADE_RECEIPT_RES_PLTT + SPIN_TRADE_MEMBER_MAX,
    SPIN_TRADE_RECEIPT_RES_CELL = SPIN_TRADE_RECEIPT_RES_CHAR + SPIN_TRADE_MEMBER_MAX,
    SPIN_TRADE_RECEIPT_RES_COUNT,
};

// What a member's trainer does next
enum {
    SPIN_TRADE_RECEIPT_ACTOR_NONE,
    SPIN_TRADE_RECEIPT_ACTOR_APPEAR,
    SPIN_TRADE_RECEIPT_ACTOR_SHOWN,
    SPIN_TRADE_RECEIPT_ACTOR_LEAVE,
};

#define SPIN_TRADE_RECEIPT_COLOR_NAME PRINT_COLOR(1, 4, 0)
#define SPIN_TRADE_RECEIPT_COLOR_SELF PRINT_COLOR(2, 3, 0)

// A palette animation of the trainers, which nothing starts
typedef struct {
    BOOL active;
    int index;
    int counter;
    void *file;
    NNSG2dPaletteData *palette;
} SpinTradeReceiptPalAnim;

typedef struct {
    u8 wait;
    // The palette's row in the file, 0xff at the end
    u8 palette;
} SpinTradeReceiptPalAnimStep;

typedef struct {
    UnionAppMember *member;
    UnionAppMember *prev;
} SpinTradeReceiptMember;

typedef union {
    struct {
        u32 id;
        BOOL present;
    } parts;
    u64 value;
} SpinTradeReceiptId;

// A member's trainer ID, and what it was before the last update
typedef struct {
    SpinTradeReceiptId id;
    SpinTradeReceiptId prev;
} SpinTradeReceiptMemberId;

typedef struct {
    u16 x;
    u16 y;
} SpinTradeReceiptPos;

struct SpinTradeReceiptWork {
    int procSeq;
    u8 unk04[4];
    BOOL receiptDone;
    SpinTradeProcWork *g2p;
    SpinTradeCommWork *g2c;
    u8 unk14[4];
    SpinTradeReceiptPalAnim palAnim;
    u8 unk2C[4];
    TCB *vblankTcb;
    ArcTool *trainerArc;
    Font *font;
    WordSet *wordSet;
    MsgData *msgData;
    StrBuf *names[SPIN_TRADE_MEMBER_MAX];
    u8 unk58[4];
    StrBuf *msgBuf;
    StrBuf *titleBuf;
    u8 unk64[4];
    ClActUnit *actorUnit;
    u32 resources[SPIN_TRADE_RECEIPT_RES_COUNT];
    u8 unk98[4];
    ClActor *actors[SPIN_TRADE_MEMBER_MAX];
    u8 unkB0[0x20];
    BmpWin *nameWindow;
    BmpWin *msgWindow;
    BmpWin *subWindow;
    BmpWin *titleWindow;
    u8 unkE0[8];
    BmpMenu *yesNo;
    PrintQueue *printQueue;
    PrintStream *stream;
    TCBExManager *tcbManager;
    BmpWin *unkF8;
    u8 unkFC;
    u8 unkFD[0x13];
    void *trainerPaletteFile;
    NNSG2dPaletteData *trainerPalette;
    int seq;
    // The sequence after SPIN_TRADE_RECEIPT_SEQ_MSG_WAIT
    int nextSeq;
    u8 unk120[4];
    int wait;
    u8 unk128[0x10];
    SpinTradeReceiptMember members[SPIN_TRADE_MEMBER_MAX];
    int actorStates[SPIN_TRADE_MEMBER_MAX];
    u8 unk174[0x4690];
    int memberCount;
    BOOL entryClosed;
    u32 unk480C;
    u8 cancelSent;
    u8 unk4811;
    u8 startCount;
    u8 unk4813;
    u32 syncBits;
    int leftTimer;
    int leftCount;
    s16 signalTimer;
    u8 unk4822[2];
    SpinTradeReceiptMemberId ids[SPIN_TRADE_MEMBER_MAX];
};

typedef int (*SpinTradeReceiptSeqFunc)(SpinTradeReceiptWork *wk, int procSeq);

static UnionApp *SpinTradeReceipt_GetUnionApp(SpinTradeReceiptWork *wk);
static void SpinTradeReceipt_OnJoin(u8 netId, UnionAppMember *member, void *work);
static void SpinTradeReceipt_OnLeave(u8 netId, UnionAppMember *member, void *work);
static void SpinTradeReceipt_UpdatePalAnim(TCB *tcb, SpinTradeReceiptWork *data);
static void SpinTradeReceipt_VBlank(TCB *tcb, void *data);
static void SpinTradeReceipt_UpdatePrint(SpinTradeReceiptWork *wk);
static void SpinTradeReceipt_SetVRAMBanks(void);
static void SpinTradeReceipt_InitBG(void);
static void SpinTradeReceipt_InitWork(SpinTradeReceiptWork *wk, ArcTool *arc);
static void SpinTradeReceipt_FreeWork(SpinTradeReceiptWork *wk);
static void SpinTradeReceipt_ExitBG(void);
static void SpinTradeReceipt_LoadBG(SpinTradeReceiptWork *wk, ArcTool *arc);
static void SpinTradeReceipt_InitActorSys(SpinTradeReceiptWork *wk, ArcTool *arc);
static void SpinTradeReceipt_ExitActorSys(SpinTradeReceiptWork *wk);
static void SpinTradeReceipt_CreateActors(SpinTradeReceiptWork *wk);
static void SpinTradeReceipt_CreateWindows(SpinTradeReceiptWork *wk);
static void SpinTradeReceipt_DrawTitle(BmpWin *window, StrBuf *title, Font *font);
static void SpinTradeReceipt_FreeWindows(SpinTradeReceiptWork *wk);
static int SpinTradeReceiptSeq_Init(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_Main(SpinTradeReceiptWork *wk, int procSeq);
static void SpinTradeReceipt_CheckKeys(SpinTradeReceiptWork *wk);
static void SpinTradeReceipt_SeqEnd(SpinTradeReceiptWork *wk);
static int SpinTradeReceiptSeq_1(SpinTradeReceiptWork *wk, int procSeq);
static void SpinTradeReceipt_SetMsgWait(SpinTradeReceiptWork *wk, int nextSeq);
static int SpinTradeReceiptSeq_MsgWait(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_2(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_CancelYesNoInit(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_CancelYesNo(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_CancelSignalWait(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_CancelOk(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_CancelRefused(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_StartYesNoInit(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_StartYesNo(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_StartSend(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_StartWait(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_EndMsg(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_EndWait(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_ParentCancelYesNoInit(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_ParentCancelYesNo(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_ParentCancelMsg(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_ParentCancelWait(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_ParentCancelSync(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_ParentCancelSyncWait(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_NoMembersMsgWait(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_NoMembersEnd(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_MemberLeft(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_MemberLeft2(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_MemberLeftWait(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceiptSeq_Start(SpinTradeReceiptWork *wk, int procSeq);
static int SpinTradeReceipt_CountMembers(SpinTradeReceiptWork *wk);
static BOOL SpinTradeReceipt_DrawNames(BmpWin *window, int unused, u16 color, SpinTradeReceiptWork *wk);
static void SpinTradeReceipt_Dummy(SpinTradeReceiptWork *wk);
static int SpinTradeReceipt_CountMembers2(SpinTradeReceiptWork *wk);
static u32 SpinTradeReceipt_GetJoined(SpinTradeReceiptWork *wk);
static BOOL SpinTradeReceipt_UpdateMembers(SpinTradeReceiptWork *wk);
static void SpinTradeReceipt_PrintMessage(SpinTradeReceiptWork *wk, u32 messageId, int unused);
static BOOL SpinTradeReceipt_IsMessageDone(SpinTradeReceiptWork *wk);
static void SpinTradeReceipt_ClearMessage(SpinTradeReceiptWork *wk);
static void SpinTradeReceipt_UpdateActors(SpinTradeReceiptWork *wk);
static void SpinTradeReceipt_LoadTrainerPalette(SpinTradeReceiptWork *wk, ArcTool *arc);
static void SpinTradeReceipt_FreeTrainerPalette(SpinTradeReceiptWork *wk);
static void SpinTradeReceipt_SetActorLook(SpinTradeReceiptWork *wk, NNSG2dPaletteData *palette, int id, int look,
                                          int gender);
static int SpinTradeReceipt_CheckMembers(SpinTradeReceiptWork *wk, int unused);
static u32 SpinTradeReceipt_GetPressedKeys(void);

static const ConfirmDialogSetup sYesNoSetup = { 0, 23, 13, 13, 0xd8 };

static const SpinTradeReceiptSeqFunc sSeqFuncs[SPIN_TRADE_RECEIPT_SEQ_COUNT] = {
    SpinTradeReceiptSeq_Init,
    SpinTradeReceiptSeq_1,
    SpinTradeReceiptSeq_2,
    SpinTradeReceiptSeq_Main,
    SpinTradeReceiptSeq_CancelYesNoInit,
    SpinTradeReceiptSeq_CancelYesNo,
    SpinTradeReceiptSeq_CancelSignalWait,
    SpinTradeReceiptSeq_CancelOk,
    SpinTradeReceiptSeq_CancelRefused,
    SpinTradeReceiptSeq_EndMsg,
    SpinTradeReceiptSeq_EndWait,
    SpinTradeReceiptSeq_ParentCancelYesNoInit,
    SpinTradeReceiptSeq_ParentCancelYesNo,
    SpinTradeReceiptSeq_ParentCancelMsg,
    SpinTradeReceiptSeq_ParentCancelWait,
    SpinTradeReceiptSeq_ParentCancelSync,
    SpinTradeReceiptSeq_ParentCancelSyncWait,
    SpinTradeReceiptSeq_NoMembersMsgWait,
    SpinTradeReceiptSeq_NoMembersEnd,
    SpinTradeReceiptSeq_MemberLeft,
    SpinTradeReceiptSeq_MemberLeft2,
    SpinTradeReceiptSeq_MemberLeftWait,
    SpinTradeReceiptSeq_StartYesNoInit,
    SpinTradeReceiptSeq_StartYesNo,
    SpinTradeReceiptSeq_StartSend,
    SpinTradeReceiptSeq_StartWait,
    SpinTradeReceiptSeq_MsgWait,
    SpinTradeReceiptSeq_Start,
};

BOOL SpinTradeReceipt_Init(GameProc *proc, u32 *state, void *param, void *work) {
    SpinTradeProcWork *g2p = param;
    SpinTradeReceiptWork *wk;
    ArcTool *arc;

    switch (*state) {
    case 0:
        GFL_BGSysDisableAllA();
        GFL_BGSysDisableAllB();
        GX_SetVisiblePlane(0);
        GXS_SetVisiblePlane(0);
        GFL_HeapCreateChild(HEAPID_USER, HEAPID_SPIN_TRADE, 0x80000);
        arc = GFL_ArcSysCreateFileHandle(ARCID_GURU2_OBJ, HEAPID_SPIN_TRADE);
        wk = GFL_ProcInitSubsystem(proc, sizeof(SpinTradeReceiptWork), HEAPID_SPIN_TRADE);
        MI_CpuFill8(wk, 0, sizeof(SpinTradeReceiptWork));
        g2p->g2r = wk;
        wk->g2p = g2p;
        wk->g2c = g2p->g2c;
        GFL_BGSysCreate(HEAPID_SPIN_TRADE);
        wk->wordSet = GFL_WordSetSystemCreateDefault(HEAPID_SPIN_TRADE);
        wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_POKEPARAM, HEAPID_SPIN_TRADE);
        wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_SPIN_TRADE);
        SpinTradeReceipt_SetVRAMBanks();
        SpinTradeReceipt_InitBG();
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 16, 1, HEAPID_SPIN_TRADE);
        SpinTradeReceipt_LoadBG(wk, arc);
        wk->vblankTcb = GFL_VBlankTCBAdd(SpinTradeReceipt_VBlank, wk, 1);
        wk->tcbManager = GFL_TCBExMgrCreate(HEAPID_SPIN_TRADE, HEAPID_SPIN_TRADE, 32, 32);
        wk->printQueue = PrintQueue_CreateDefault(HEAPID_SPIN_TRADE);
        SpinTradeReceipt_InitWork(wk, arc);
        SpinTradeReceipt_InitActorSys(wk, arc);
        SpinTradeReceipt_CreateActors(wk);
        SpinTradeReceipt_CreateWindows(wk);
        SpinTradeComm_AddCommands(g2p->g2c);
        GFL_NetSetIconScreen(TRUE, HEAPID_SPIN_TRADE);
        GFL_ArcToolFree(arc);
        (*state)++;
        break;
    case 1:
        *state = 0;
        return TRUE;
    }
    return FALSE;
}

static UnionApp *SpinTradeReceipt_GetUnionApp(SpinTradeReceiptWork *wk) {
    return wk->g2p->param->app;
}

static void SpinTradeReceipt_OnJoin(u8 netId, UnionAppMember *member, void *work) {
    SpinTradeReceiptWork *wk = work;

    if (NetSystem_GetNetId() == 0) {
        u8 id = netId;

        SpinTradeComm_Send(wk->g2c, SPIN_TRADE_NET_CMD_JOIN, &id, sizeof(id));
    }
}

static void SpinTradeReceipt_OnLeave(u8 netId, UnionAppMember *member, void *work) {
}

BOOL SpinTradeReceipt_Main(GameProc *proc, u32 *state, void *param, void *work) {
    SpinTradeReceiptWork *wk = work;

    if (NetSystem_GetNetId() == 0 && wk->g2c->leavingBits != 0) {
        wk->g2c->leavingBits &= UnionApp_GetJoined(SpinTradeReceipt_GetUnionApp(wk));
    }
    switch (wk->procSeq) {
    case SPIN_TRADE_RECEIPT_PROC_FADE_IN:
        if (GFL_WipeIsFinished()) {
            wk->procSeq = SPIN_TRADE_RECEIPT_PROC_MAIN;
            UnionApp_OpenEntry(SpinTradeReceipt_GetUnionApp(wk));
            UnionApp_SetCallbacks(SpinTradeReceipt_GetUnionApp(wk), SpinTradeReceipt_OnJoin, SpinTradeReceipt_OnLeave,
                                  wk);
        }
        break;
    case SPIN_TRADE_RECEIPT_PROC_MAIN:
        if (sSeqFuncs[wk->seq] != NULL) {
            wk->procSeq = sSeqFuncs[wk->seq](wk, wk->procSeq);
        }
        if (!wk->g2c->started) {
            SpinTradeReceipt_DrawNames(wk->nameWindow, 0, SPIN_TRADE_RECEIPT_COLOR_NAME, wk);
        }
        SpinTradeReceipt_UpdateActors(wk);
        if (NetSystem_GetNetId() == 0) {
            int next = SpinTradeReceipt_CheckMembers(wk, 1);

            if (wk->procSeq == SPIN_TRADE_RECEIPT_PROC_MAIN) {
                wk->procSeq = next;
            }
        }
        break;
    case SPIN_TRADE_RECEIPT_PROC_MAIN_NO_UPDATE:
        if (sSeqFuncs[wk->seq] != NULL) {
            wk->procSeq = sSeqFuncs[wk->seq](wk, wk->procSeq);
        }
        break;
    case SPIN_TRADE_RECEIPT_PROC_FADE_OUT:
        if (GFL_WipeIsFinished()) {
            if (wk->receiptDone) {
                return TRUE;
            }
            NetCommand_RemoveTable(SPIN_TRADE_NET_CMD_BASE);
            UnionApp_RequestStart(SpinTradeReceipt_GetUnionApp(wk));
            wk->procSeq = SPIN_TRADE_RECEIPT_PROC_WAIT_START;
        }
        break;
    case SPIN_TRADE_RECEIPT_PROC_WAIT_START:
        if (UnionApp_IsStarted(SpinTradeReceipt_GetUnionApp(wk))) {
            return TRUE;
        }
        break;
    }
    if (GFL_WipeIsFinished() && GFL_NetErrCheck()) {
        return TRUE;
    }
    ClActSys_Draw();
    SpinTradeReceipt_UpdatePrint(wk);
    return FALSE;
}

BOOL SpinTradeReceipt_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    SpinTradeReceiptWork *wk = work;

    GFL_TCBExMgrFreeTasks(wk->tcbManager);
    GFL_TCBExMgrFree(wk->tcbManager);
    PrintQueue_Clear(wk->printQueue);
    PrintQueue_Free(wk->printQueue);
    SpinTradeReceipt_ExitActorSys(wk);
    SpinTradeReceipt_FreeWindows(wk);
    SpinTradeReceipt_ExitBG();
    GFL_MsgDataFree(wk->msgData);
    GFL_WordSetSystemFree(wk->wordSet);
    GFL_FontFree(wk->font);
    GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
    GFL_TCBRemove(wk->vblankTcb);
    wk->g2p->receiptDone = wk->receiptDone;
    wk->g2p->memberCount = SpinTradeReceipt_CountMembers2(wk);
    wk->g2p->memberBits = SpinTradeReceipt_GetJoined(wk);
    SpinTradeReceipt_FreeWork(wk);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_SPIN_TRADE);
    return TRUE;
}

// clang-format off
static const SpinTradeReceiptPalAnimStep sPalAnimSteps[] = {
    { 8, 2 },
    { 8, 3 },
    { 8, 4 },
    { 24, 0 },
    { 0, 0xff },
};

// clang-format on

static void SpinTradeReceipt_UpdatePalAnim(TCB *tcb, SpinTradeReceiptWork *wk) {
    SpinTradeReceiptPalAnim *anim = &wk->palAnim;

    if (anim->active) {
        if (anim->counter > sPalAnimSteps[anim->index].wait) {
            anim->counter = 0;
            anim->index++;
            if (sPalAnimSteps[anim->index].palette == 0xff) {
                anim->index = 0;
            }
            GX_LoadOBJPltt((u8 *)anim->palette->pRawData + sPalAnimSteps[anim->index].palette * 0x20, 0, 0x20);
        } else {
            wk->palAnim.counter++;
        }
    }
}

static void SpinTradeReceipt_VBlank(TCB *tcb, void *data) {
    SpinTradeReceipt_UpdatePalAnim(tcb, data);
    ClActSys_VBlank();
    GFL_BGSysUpdate();
}

static void SpinTradeReceipt_UpdatePrint(SpinTradeReceiptWork *wk) {
    if (wk->printQueue != NULL) {
        PrintQueue_Main(wk->printQueue);
    }
    GFL_TCBExMgrUpdate(wk->tcbManager);
}

static const BGSysVRAMConfig sVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static void SpinTradeReceipt_SetVRAMBanks(void) {
    GFL_BGSysSetVRAMBanks(&sVRAMConfig);
}

static void SpinTradeReceipt_InitBG(void) {
    {
        BGSysLCDConfig config = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

        GFL_BGSysSetLCDConfig(&config);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf000),
            GX_BG_CHARBASE(0x10000),
            0x4000,
            GX_BG_EXTPLTT_01,
            0,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(4, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(4);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf800),
            GX_BG_CHARBASE(0x00000),
            0x4000,
            GX_BG_EXTPLTT_01,
            1,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(5, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(5);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xd800),
            GX_BG_CHARBASE(0x08000),
            0x4000,
            GX_BG_EXTPLTT_01,
            2,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(6, &setup, BGMODE_TEXT);
        GFL_BGSysClearCharCore(4, 0x20, 0, HEAPID_SPIN_TRADE);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf800),
            GX_BG_CHARBASE(0x00000),
            0x4000,
            GX_BG_EXTPLTT_01,
            0,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(0, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(0);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf000),
            GX_BG_CHARBASE(0x08000),
            0x4000,
            GX_BG_EXTPLTT_01,
            1,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(1, &setup, BGMODE_TEXT);
        GFL_BGSysClearCharCore(0, 0x20, 0, HEAPID_SPIN_TRADE);
    }
}

static void SpinTradeReceipt_InitWork(SpinTradeReceiptWork *wk, ArcTool *arc) {
    int i;

    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++) {
        wk->names[i] = GFL_StrBufCreate(8, HEAPID_SPIN_TRADE);
        wk->members[i].member = NULL;
        wk->members[i].prev = NULL;
        wk->actorStates[i] = SPIN_TRADE_RECEIPT_ACTOR_NONE;
    }
    wk->msgBuf = GFL_StrBufCreate(180, HEAPID_SPIN_TRADE);
    wk->titleBuf = GFL_StrBufCreate(40, HEAPID_SPIN_TRADE);
    wk->seq = SPIN_TRADE_RECEIPT_SEQ_INIT;
    GFL_MsgDataLoadStrbuf(wk->msgData, BtlPokeparam_Text_SeekingSpinTrade, wk->titleBuf);
    SpinTradeReceipt_LoadTrainerPalette(wk, arc);
    wk->palAnim.active = FALSE;
    wk->palAnim.counter = 0;
    wk->palAnim.index = 0;
    wk->palAnim.file = GFL_G2DIOReadNCLRArc(arc, 2, &wk->palAnim.palette, HEAPID_SPIN_TRADE);
    wk->memberCount = 0;
    wk->yesNo = NULL;
    wk->g2c->joinCount = 2;
}

static void SpinTradeReceipt_FreeWork(SpinTradeReceiptWork *wk) {
    int i;

    SpinTradeReceipt_FreeTrainerPalette(wk);
    GFL_HeapFree(wk->palAnim.file);
    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++) {
        GFL_StrBufFree(wk->names[i]);
    }
    GFL_StrBufFree(wk->titleBuf);
    GFL_StrBufFree(wk->msgBuf);
}

static void SpinTradeReceipt_ExitBG(void) {
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(0);
    GFL_BGSysFree();
}

static void SpinTradeReceipt_LoadBG(SpinTradeReceiptWork *wk, ArcTool *arc) {
    int gender = getTrainerGender(GetGameDataPlayerInfo(wk->g2p->param->gameData));

    GFL_G2DIOLoadArcNCLRDefault(arc, 1, PALTYPE_MAIN_BG, 0, 0x200, HEAPID_SPIN_TRADE);
    GFL_BGSysLoadArcNCGRStatic(arc, 7, 1, 0, 0x2000, FALSE, HEAPID_SPIN_TRADE);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 13, 1, 0, 0x600, FALSE, HEAPID_SPIN_TRADE);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 13 * 0x20, 0x20, HEAPID_SPIN_TRADE);
    GFL_BGSysLoadNCLRDefault(ARCID_C_GEAR, gender % 2, PALTYPE_SUB_BG, 0, 0x160, HEAPID_SPIN_TRADE);
    GFL_BGSysLoadNCGRStatic(ARCID_C_GEAR, 2, 6, 0, 0, FALSE, HEAPID_SPIN_TRADE);
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_C_GEAR, 3, 6, 0, 0, FALSE, HEAPID_SPIN_TRADE);
    LoadSysMsgBox(0, 1, 10, 0, HEAPID_SPIN_TRADE);
    LoadSysMsgBox(0, 31, 11, 0, HEAPID_SPIN_TRADE);
    GFL_BGSysSetBGEnabled(0, TRUE);
    GFL_BGSysSetBGEnabled(1, TRUE);
    GFL_BGSysSetBGEnabled(6, TRUE);
}

// The files of the trainers' cell actor by the player's gender: characters, palette, cells, and animations
static const u32 sActorFiles[4][2] = {
    { 20, 23 },
    { 4, 6 },
    { 18, 21 },
    { 19, 22 },
};

static void SpinTradeReceipt_InitActorSys(SpinTradeReceiptWork *wk, ArcTool *unused) {
    int gender = getTrainerGender(GetGameDataPlayerInfo(wk->g2p->param->gameData));
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_WIFI_2DCHAR, HEAPID_SPIN_TRADE);
    int i;

    ClActSys_Create(&data_02093f08, &sVRAMConfig, HEAPID_SPIN_TRADE);
    wk->actorUnit = ClActUnit_Create(30, 1, HEAPID_SPIN_TRADE);
    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++) {
        wk->resources[SPIN_TRADE_RECEIPT_RES_CHAR + i] =
            ClActSys_LoadChars(arc, sActorFiles[0][gender], FALSE, 0, HEAPID_SPIN_TRADE);
        wk->resources[SPIN_TRADE_RECEIPT_RES_PLTT + i] =
            ClActSys_LoadPlttRange(arc, sActorFiles[1][gender], 0, i * 0x20, 0, 1, HEAPID_SPIN_TRADE);
    }
    wk->resources[SPIN_TRADE_RECEIPT_RES_CELL] =
        ClActSys_LoadCellAnims(arc, sActorFiles[2][gender], sActorFiles[3][gender], HEAPID_SPIN_TRADE);
    GFL_ArcToolFree(arc);
    wk->trainerArc = GFL_ArcSysCreateFileHandle(ARCID_WIFI_UNIONCHAR, HEAPID_SPIN_TRADE);
}

static void SpinTradeReceipt_ExitActorSys(SpinTradeReceiptWork *wk) {
    int i;

    GFL_ArcToolFree(wk->trainerArc);
    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++) {
        // BUG: the palettes are freed as characters and the characters as palettes. Each kind's handles count
        // from 0, so the five of one kind are the five of the other
#ifdef BUGFIX
        ClActSys_FreeChars(wk->resources[SPIN_TRADE_RECEIPT_RES_CHAR + i]);
        ClActSys_FreePltt(wk->resources[SPIN_TRADE_RECEIPT_RES_PLTT + i]);
#else
        ClActSys_FreeChars(wk->resources[SPIN_TRADE_RECEIPT_RES_PLTT + i]);
        ClActSys_FreePltt(wk->resources[SPIN_TRADE_RECEIPT_RES_CHAR + i]);
#endif
    }
    ClActSys_FreeCellAnims(wk->resources[SPIN_TRADE_RECEIPT_RES_CELL]);
    ClActUnit_Delete(wk->actorUnit);
    ClActSys_Delete();
}

// Where the trainers stand: the middle, then each member's place
// clang-format off
static const SpinTradeReceiptPos sActorPositions[1 + SPIN_TRADE_MEMBER_MAX] = {
    { 204, 84 },
    { 204, 53 },
    { 233, 72 },
    { 223, 103 },
    { 184, 103 },
    { 174, 72 },
};

// clang-format on

static void SpinTradeReceipt_CreateActors(SpinTradeReceiptWork *wk) {
    ClActorSetup setup;
    int i;

    setup.bgPriority = 1;
    setup.priority = 0;
    setup.sequence = 0;
    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++) {
        setup.x = sActorPositions[i + 1].x;
        setup.y = sActorPositions[i + 1].y;
        wk->actors[i] = ClActor_Create(wk->actorUnit, wk->resources[SPIN_TRADE_RECEIPT_RES_CHAR + i],
                                       wk->resources[SPIN_TRADE_RECEIPT_RES_PLTT + i],
                                       wk->resources[SPIN_TRADE_RECEIPT_RES_CELL], &setup, 0, HEAPID_SPIN_TRADE);
        ClActor_SetAutoAnim(wk->actors[i], TRUE);
        ClActor_SetSequence(wk->actors[i], 0);
        ClActor_SetVisible(wk->actors[i], FALSE);
    }
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, FALSE);
}

static void SpinTradeReceipt_CreateWindows(SpinTradeReceiptWork *wk) {
    BmpWin_InitAllocator(HEAPID_SPIN_TRADE);
    wk->subWindow = BmpWin_CreateDynamic(5, 26, 20, 6, 2, 13, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->subWindow), 0);
    wk->msgWindow = BmpWin_CreateDynamic(0, 2, 19, 27, 4, 13, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->msgWindow), 15);
    wk->titleWindow = BmpWin_CreateDynamic(0, 3, 1, 26, 2, 15, TRUE);
    SpinTradeReceipt_DrawTitle(wk->titleWindow, wk->titleBuf, wk->font);
    wk->nameWindow = BmpWin_CreateDynamic(0, 2, 6, 16, 11, 15, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->nameWindow), 0);
    SpinTradeReceipt_DrawNames(wk->nameWindow, 0, SPIN_TRADE_RECEIPT_COLOR_NAME, wk);
    wk->unkF8 = wk->titleWindow;
    wk->unkFC = 0;
}

static void SpinTradeReceipt_DrawTitle(BmpWin *window, StrBuf *title, Font *font) {
    int width = GFL_FontGetBlockWidth(title, font, 0);

    GFL_BitmapFill(BmpWin_GetBitmap(window), 0);
    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(window), (26 * 8 - width) / 2, 1, title, font,
                                   SPIN_TRADE_RECEIPT_COLOR_NAME);
    BmpWin_TransferNow(window);
}

static void SpinTradeReceipt_FreeWindows(SpinTradeReceiptWork *wk) {
    if (wk->yesNo != NULL) {
        ConfirmDialog_Free(wk->yesNo);
    }
    BmpWin_Free(wk->nameWindow);
    BmpWin_Free(wk->titleWindow);
    BmpWin_Free(wk->subWindow);
    BmpWin_Free(wk->msgWindow);
    BmpWin_FreeAllocator();
}

static int SpinTradeReceiptSeq_Init(SpinTradeReceiptWork *wk, int procSeq) {
    if (NetSystem_GetNetId() == 0) {
        GflNet_SetAutoExit(FALSE);
        if (UnionApp_CountJoined(SpinTradeReceipt_GetUnionApp(wk)) >= 2) {
            SpinTradeReceipt_PrintMessage(wk, BtlPokeparam_Text_WhenGroupReadyButton, 0);
        }
    } else {
        GflNet_SetAutoExit(TRUE);
        SpinTradeReceipt_PrintMessage(wk, BtlPokeparam_Text_PleaseWaitBButton, 0);
    }
    SpinTradeReceipt_SetMsgWait(wk, SPIN_TRADE_RECEIPT_SEQ_MAIN);
    return procSeq;
}

static int SpinTradeReceiptSeq_Main(SpinTradeReceiptWork *wk, int procSeq) {
    SpinTradeReceipt_CheckKeys(wk);
    if (NetSystem_GetNetId() == 0) {
        if (SpinTradeReceipt_CountMembers(wk) != 1) {
            SpinTradeReceipt_Dummy(wk);
        }
    } else {
        SpinTradeReceipt_Dummy(wk);
    }
    return procSeq;
}

static void SpinTradeReceipt_CheckKeys(SpinTradeReceiptWork *wk) {
    SpinTradeCommWork *g2c;

    if (SpinTradeReceipt_GetPressedKeys() & PAD_BUTTON_A) {
        if (NetSystem_GetNetId() == 0) {
            g2c = wk->g2c;
            if (g2c->joinCount == SpinTradeReceipt_CountMembers2(wk) && g2c->leavingBits == 0) {
                if (UnionApp_CloseEntry(SpinTradeReceipt_GetUnionApp(wk))) {
                    GFL_SndSEPlay(SEQ_SE_DECIDE1);
                    SpinTradeReceipt_PrintMessage(wk, BtlPokeparam_Text_GroupOk, 0);
                    SpinTradeReceipt_SetMsgWait(wk, SPIN_TRADE_RECEIPT_SEQ_START_YESNO_INIT);
                }
            } else {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
        }
    } else if (SpinTradeReceipt_GetPressedKeys() & PAD_BUTTON_B) {
        if (NetSystem_GetNetId() != 0) {
            if (wk->g2c->unk28 == 0) {
                SpinTradeReceipt_PrintMessage(wk, BtlPokeparam_Text_CancelSpinTrade, 0);
                SpinTradeReceipt_SetMsgWait(wk, SPIN_TRADE_RECEIPT_SEQ_CANCEL_YESNO_INIT);
            } else {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
        } else {
            g2c = wk->g2c;
            if (g2c->joinCount == UnionApp_CountJoined(SpinTradeReceipt_GetUnionApp(wk)) && g2c->leavingBits == 0) {
                if (UnionApp_CloseEntry(SpinTradeReceipt_GetUnionApp(wk))) {
                    SpinTradeReceipt_PrintMessage(wk, BtlPokeparam_Text_CancelSpinTrade, 0);
                    SpinTradeReceipt_SetMsgWait(wk, SPIN_TRADE_RECEIPT_SEQ_CANCEL_YESNO_INIT);
                    GFL_SndSEPlay(SEQ_SE_CANCEL1);
                }
            } else {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
        }
    } else {
        if (!wk->entryClosed && NetSystem_GetNetId() == 0 &&
            wk->g2c->joinCount == UnionApp_CountJoined(SpinTradeReceipt_GetUnionApp(wk))) {
            UnionApp_OpenEntry(SpinTradeReceipt_GetUnionApp(wk));
        }
    }
}

// Runs at the end of each sequence
static void SpinTradeReceipt_SeqEnd(SpinTradeReceiptWork *wk) {
}

static int SpinTradeReceiptSeq_1(SpinTradeReceiptWork *wk, int procSeq) {
    wk->seq = SPIN_TRADE_RECEIPT_SEQ_2;
    SpinTradeReceipt_SeqEnd(wk);
    return procSeq;
}

static void SpinTradeReceipt_SetMsgWait(SpinTradeReceiptWork *wk, int nextSeq) {
    wk->seq = SPIN_TRADE_RECEIPT_SEQ_MSG_WAIT;
    wk->nextSeq = nextSeq;
}

static int SpinTradeReceiptSeq_MsgWait(SpinTradeReceiptWork *wk, int procSeq) {
    if (SpinTradeReceipt_IsMessageDone(wk)) {
        wk->seq = wk->nextSeq;
    }
    SpinTradeReceipt_SeqEnd(wk);
    return procSeq;
}

static int SpinTradeReceiptSeq_2(SpinTradeReceiptWork *wk, int procSeq) {
    if (NetSystem_GetNetId() == 0) {
        UnionApp_OpenEntry(SpinTradeReceipt_GetUnionApp(wk));
    }
    wk->seq = SPIN_TRADE_RECEIPT_SEQ_MAIN;
    SpinTradeReceipt_SeqEnd(wk);
    return procSeq;
}

static int SpinTradeReceiptSeq_CancelYesNoInit(SpinTradeReceiptWork *wk, int procSeq) {
    wk->yesNo = ShopUI_CreateConfirmDialog(&sYesNoSetup, 31, 11, 0, HEAPID_SPIN_TRADE);
    wk->seq = SPIN_TRADE_RECEIPT_SEQ_CANCEL_YESNO;
    SpinTradeReceipt_SeqEnd(wk);
    return procSeq;
}

static int SpinTradeReceiptSeq_CancelYesNo(SpinTradeReceiptWork *wk, int procSeq) {
    u32 result;

    if (NetSystem_GetNetId() != 0) {
        if (wk->g2c->unk28 != 0) {
            if (SpinTradeReceipt_GetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B | PAD_KEY_UP | PAD_KEY_DOWN)) {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
            SpinTradeReceipt_SeqEnd(wk);
            return procSeq;
        }
    } else {
        if (wk->g2c->leavingBits != 0) {
            if (SpinTradeReceipt_GetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B | PAD_KEY_UP | PAD_KEY_DOWN)) {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
            SpinTradeReceipt_SeqEnd(wk);
            return procSeq;
        }
    }
    if (SpinTradeReceipt_CountMembers2(wk) != UnionApp_CountJoined(SpinTradeReceipt_GetUnionApp(wk))) {
        SpinTradeReceipt_SeqEnd(wk);
        return procSeq;
    }
    result = ConfirmDialog_Update(wk->yesNo);
    if (result != BMPMENU_NULL) {
        if (result == BMPMENU_CANCEL) {
            if (NetSystem_GetNetId() == 0) {
                UnionApp_OpenEntry(SpinTradeReceipt_GetUnionApp(wk));
            }
            wk->seq = SPIN_TRADE_RECEIPT_SEQ_INIT;
        } else {
            if (NetSystem_GetNetId() == 0) {
                SpinTradeReceipt_SetMsgWait(wk, SPIN_TRADE_RECEIPT_SEQ_PARENT_CANCEL_YESNO_INIT);
                SpinTradeReceipt_PrintMessage(wk, BtlPokeparam_Text_IfLeaderDropsOut, 0);
            } else {
                SpinTradeCommSignal signal;

                MI_CpuFill8(&signal, 0, sizeof(signal));
                signal.type = SPIN_TRADE_SIGNAL_LEAVE_REQUEST;
                signal.netId = NetSystem_GetNetId();
                if (SpinTradeComm_Send(wk->g2c, SPIN_TRADE_NET_CMD_SIGNAL, &signal, sizeof(signal))) {
                    wk->cancelSent = TRUE;
                    wk->signalTimer = 0;
                    wk->seq = SPIN_TRADE_RECEIPT_SEQ_CANCEL_SIGNAL_WAIT;
                } else {
                    wk->seq = SPIN_TRADE_RECEIPT_SEQ_INIT;
                }
            }
        }
        wk->yesNo = NULL;
    }
    SpinTradeReceipt_SeqEnd(wk);
    return procSeq;
}

static int SpinTradeReceiptSeq_CancelSignalWait(SpinTradeReceiptWork *wk, int procSeq) {
    wk->signalTimer = 0;
    SpinTradeReceipt_SeqEnd(wk);
    return procSeq;
}

static int SpinTradeReceiptSeq_CancelOk(SpinTradeReceiptWork *wk, int procSeq) {
    if (wk->g2c->signalCount != UnionApp_CountJoined(SpinTradeReceipt_GetUnionApp(wk)) ||
        wk->g2c->signalCount != SpinTradeReceipt_CountMembers2(wk)) {
        wk->signalTimer = 0;
        wk->seq = SPIN_TRADE_RECEIPT_SEQ_CANCEL_REFUSED;
        SpinTradeReceipt_SeqEnd(wk);
        return procSeq;
    }
    wk->signalTimer++;
    if (wk->signalTimer > 30) {
        SpinTradeCommSignal signal;

        MI_CpuFill8(&signal, 0, sizeof(signal));
        signal.type = SPIN_TRADE_SIGNAL_LEAVE;
        signal.netId = NetSystem_GetNetId();
        SpinTradeComm_Send(wk->g2c, SPIN_TRADE_NET_CMD_SIGNAL, &signal, sizeof(signal));
        wk->signalTimer = 0;
        wk->seq = SPIN_TRADE_RECEIPT_SEQ_END_MSG;
    }
    SpinTradeReceipt_SeqEnd(wk);
    return procSeq;
}

static int SpinTradeReceiptSeq_CancelRefused(SpinTradeReceiptWork *wk, int procSeq) {
    wk->cancelSent = FALSE;
    SpinTradeReceipt_SetMsgWait(wk, SPIN_TRADE_RECEIPT_SEQ_INIT);
    SpinTradeReceipt_SeqEnd(wk);
    return procSeq;
}

static int SpinTradeReceiptSeq_StartYesNoInit(SpinTradeReceiptWork *wk, int procSeq) {
    wk->yesNo = ShopUI_CreateConfirmDialog(&sYesNoSetup, 31, 11, 0, HEAPID_SPIN_TRADE);
    wk->seq = SPIN_TRADE_RECEIPT_SEQ_START_YESNO;
    SpinTradeReceipt_SeqEnd(wk);
    return procSeq;
}

static int SpinTradeReceiptSeq_StartYesNo(SpinTradeReceiptWork *wk, int procSeq) {
    SpinTradeCommWork *g2c = wk->g2c;
    u32 result;

    if (g2c->joinCount != SpinTradeReceipt_CountMembers2(wk) || g2c->leavingBits != 0) {
        if (SpinTradeReceipt_GetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B | PAD_KEY_UP | PAD_KEY_DOWN)) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
        SpinTradeReceipt_SeqEnd(wk);
        return procSeq;
    }
    result = ConfirmDialog_Update(wk->yesNo);
    if (result != BMPMENU_NULL) {
        if (result == BMPMENU_CANCEL) {
            UnionApp_OpenEntry(SpinTradeReceipt_GetUnionApp(wk));
            wk->seq = SPIN_TRADE_RECEIPT_SEQ_INIT;
        } else if (NetSystem_GetNetId() == 0) {
            wk->seq = SPIN_TRADE_RECEIPT_SEQ_START_SEND;
            wk->startCount = SpinTradeReceipt_CountMembers2(wk);
        }
        wk->yesNo = NULL;
    }
    SpinTradeReceipt_SeqEnd(wk);
    return procSeq;
}

static int SpinTradeReceiptSeq_StartSend(SpinTradeReceiptWork *wk, int procSeq) {
    if (wk->unk4813 == 0) {
        if (SpinTradeComm_Send(wk->g2c, SPIN_TRADE_NET_CMD_START, NULL, 0) == TRUE) {
            wk->seq = SPIN_TRADE_RECEIPT_SEQ_START;
            GFL_NetSetDisconnectError(TRUE);
        }
    }
    return procSeq;
}

static int SpinTradeReceiptSeq_StartWait(SpinTradeReceiptWork *wk, int procSeq) {
    if (NetSystem_GetNetId() == 0) {
        UnionApp_CountJoined(SpinTradeReceipt_GetUnionApp(wk));
    }
    if (wk->g2c->startCount == UnionApp_CountJoined(SpinTradeReceipt_GetUnionApp(wk))) {
        wk->seq = SPIN_TRADE_RECEIPT_SEQ_START;
    }
    return procSeq;
}

static int SpinTradeReceiptSeq_EndMsg(SpinTradeReceiptWork *wk, int procSeq) {
    SpinTradeReceipt_PrintMessage(wk, BtlPokeparam_Text_SpinTradeWillCanceled, 0);
    SpinTradeReceipt_SetMsgWait(wk, SPIN_TRADE_RECEIPT_SEQ_END_WAIT);
    SpinTradeReceipt_SeqEnd(wk);
    return procSeq;
}

static int SpinTradeReceiptSeq_EndWait(SpinTradeReceiptWork *wk, int procSeq) {
    wk->wait++;
    if (wk->wait > 60) {
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 16, 1, HEAPID_SPIN_TRADE);
        procSeq = SPIN_TRADE_RECEIPT_PROC_FADE_OUT;
    }
    SpinTradeReceipt_SeqEnd(wk);
    return procSeq;
}

static int SpinTradeReceiptSeq_ParentCancelYesNoInit(SpinTradeReceiptWork *wk, int procSeq) {
    wk->yesNo = ShopUI_CreateConfirmDialog(&sYesNoSetup, 31, 11, 0, HEAPID_SPIN_TRADE);
    wk->seq = SPIN_TRADE_RECEIPT_SEQ_PARENT_CANCEL_YESNO;
    SpinTradeReceipt_SeqEnd(wk);
    return procSeq;
}

static int SpinTradeReceiptSeq_ParentCancelYesNo(SpinTradeReceiptWork *wk, int procSeq) {
    SpinTradeCommWork *g2c = wk->g2c;
    u32 result;

    if (g2c->joinCount != SpinTradeReceipt_CountMembers2(wk) || g2c->leavingBits != 0) {
        if (SpinTradeReceipt_GetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B | PAD_KEY_UP | PAD_KEY_DOWN)) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
        SpinTradeReceipt_SeqEnd(wk);
        return procSeq;
    }
    result = ConfirmDialog_Update(wk->yesNo);
    if (result != BMPMENU_NULL) {
        if (result == BMPMENU_CANCEL) {
            wk->seq = SPIN_TRADE_RECEIPT_SEQ_INIT;
            UnionApp_OpenEntry(SpinTradeReceipt_GetUnionApp(wk));
        } else {
            wk->seq = SPIN_TRADE_RECEIPT_SEQ_PARENT_CANCEL_MSG;
            SpinTradeComm_Send(wk->g2c, SPIN_TRADE_NET_CMD_CANCEL, NULL, 0);
            copyVarForText(wk->wordSet, 0, &UnionApp_GetMember(SpinTradeReceipt_GetUnionApp(wk), 0)->info);
        }
        wk->yesNo = NULL;
    }
    SpinTradeReceipt_SeqEnd(wk);
    return procSeq;
}

static int SpinTradeReceiptSeq_ParentCancelMsg(SpinTradeReceiptWork *wk, int procSeq) {
    if (NetSystem_GetNetId() == 0) {
        SpinTradeReceipt_PrintMessage(wk, BtlPokeparam_Text_SpinTradeWillCanceled, 0);
    } else {
        copyVarForText(wk->wordSet, 0, &UnionApp_GetMember(SpinTradeReceipt_GetUnionApp(wk), 0)->info);
        SpinTradeReceipt_PrintMessage(wk, BtlPokeparam_Text_LeaderDroppedOutGroup, 0);
    }
    SpinTradeReceipt_SetMsgWait(wk, SPIN_TRADE_RECEIPT_SEQ_PARENT_CANCEL_WAIT);
    wk->wait = 0;
    SpinTradeReceipt_SeqEnd(wk);
    return procSeq;
}

static int SpinTradeReceiptSeq_ParentCancelWait(SpinTradeReceiptWork *wk, int procSeq) {
    wk->wait++;
    if (wk->wait > 45) {
        wk->seq = SPIN_TRADE_RECEIPT_SEQ_PARENT_CANCEL_SYNC;
    }
    SpinTradeReceipt_SeqEnd(wk);
    return procSeq;
}

static int SpinTradeReceiptSeq_ParentCancelSync(SpinTradeReceiptWork *wk, int procSeq) {
    NetHandle_GetOwn();
    wk->syncBits = wk->unk480C ^ SpinTradeReceipt_GetJoined(wk);
    NetHandle_SendTimingPairToMachines(NetHandle_GetOwn(), 0xcf, 40, (u16)wk->syncBits);
    wk->seq = SPIN_TRADE_RECEIPT_SEQ_PARENT_CANCEL_SYNC_WAIT;
    SpinTradeReceipt_SeqEnd(wk);
    return procSeq;
}

static int SpinTradeReceiptSeq_ParentCancelSyncWait(SpinTradeReceiptWork *wk, int procSeq) {
    if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0xcf, 40) ||
        wk->syncBits != (SpinTradeReceipt_GetJoined(wk) ^ wk->unk480C)) {
        GflNet_SetAutoExit(FALSE);
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 16, 1, HEAPID_SPIN_TRADE);
        procSeq = SPIN_TRADE_RECEIPT_PROC_FADE_OUT;
    }
    SpinTradeReceipt_SeqEnd(wk);
    return procSeq;
}

static int SpinTradeReceiptSeq_NoMembersMsgWait(SpinTradeReceiptWork *wk, int procSeq) {
    if (SpinTradeReceipt_IsMessageDone(wk)) {
        SpinTradeReceipt_PrintMessage(wk, BtlPokeparam_Text_ThereNoMembersSpin, 0);
        wk->seq = SPIN_TRADE_RECEIPT_SEQ_NO_MEMBERS_END;
    }
    SpinTradeReceipt_SeqEnd(wk);
    return procSeq;
}

static int SpinTradeReceiptSeq_NoMembersEnd(SpinTradeReceiptWork *wk, int procSeq) {
    if (SpinTradeReceipt_IsMessageDone(wk)) {
        wk->seq = SPIN_TRADE_RECEIPT_SEQ_END_WAIT;
    }
    SpinTradeReceipt_SeqEnd(wk);
    return procSeq;
}

static int SpinTradeReceiptSeq_MemberLeft(SpinTradeReceiptWork *wk, int procSeq) {
    SpinTradeReceipt_PrintMessage(wk, BtlPokeparam_Text_HasLeft, 1);
    wk->seq = SPIN_TRADE_RECEIPT_SEQ_MEMBER_LEFT_2;
    NetSystem_GetNetId();
    wk->leftCount = UnionApp_CountJoined(SpinTradeReceipt_GetUnionApp(wk));
    wk->leftTimer = 300;
    SpinTradeReceipt_SeqEnd(wk);
    return procSeq;
}

static int SpinTradeReceiptSeq_MemberLeft2(SpinTradeReceiptWork *wk, int procSeq) {
    if (wk->leftCount != 0 && wk->leftCount != UnionApp_CountJoined(SpinTradeReceipt_GetUnionApp(wk))) {
        wk->leftCount = 0;
    }
    wk->seq = SPIN_TRADE_RECEIPT_SEQ_MEMBER_LEFT_WAIT;
    wk->wait = 0;
    SpinTradeReceipt_SeqEnd(wk);
    return procSeq;
}

static int SpinTradeReceiptSeq_MemberLeftWait(SpinTradeReceiptWork *wk, int procSeq) {
    if (wk->leftCount != 0 && wk->leftCount != UnionApp_CountJoined(SpinTradeReceipt_GetUnionApp(wk))) {
        wk->leftCount = 0;
    }
    wk->leftTimer--;
    if (wk->leftTimer < 0) {
        wk->leftCount = 0;
    }
    wk->wait++;
    if (wk->wait > 60 && wk->leftCount == 0) {
        wk->seq = SPIN_TRADE_RECEIPT_SEQ_INIT;
        NetSystem_GetNetId();
    }
    SpinTradeReceipt_SeqEnd(wk);
    return procSeq;
}

static int SpinTradeReceiptSeq_Start(SpinTradeReceiptWork *wk, int procSeq) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 16, 1, HEAPID_SPIN_TRADE);
    wk->receiptDone = TRUE;
    SpinTradeReceipt_SeqEnd(wk);
    return SPIN_TRADE_RECEIPT_PROC_FADE_OUT;
}

void SpinTradeReceipt_SetSeq(SpinTradeReceiptWork *wk, int seq, u8 netId) {
    switch (seq) {
    case SPIN_TRADE_RECEIPT_SEQ_2:
        SpinTradeReceipt_ClearMessage(wk);
        break;
    case SPIN_TRADE_RECEIPT_SEQ_PARENT_CANCEL_MSG:
        if (wk->yesNo != NULL) {
            ConfirmDialog_Free(wk->yesNo);
            wk->yesNo = NULL;
        }
        break;
    case SPIN_TRADE_RECEIPT_SEQ_START_WAIT:
        SpinTradeReceipt_PrintMessage(wk, BtlPokeparam_Text_Empty_2, 0);
        if (wk->yesNo != NULL) {
            ConfirmDialog_Free(wk->yesNo);
            wk->yesNo = NULL;
        }
        break;
    case SPIN_TRADE_RECEIPT_SEQ_MEMBER_LEFT:
        if (wk->cancelSent == TRUE) {
            return;
        }
        copyVarForText(wk->wordSet, 0, &UnionApp_GetMember(SpinTradeReceipt_GetUnionApp(wk), netId)->info);
        if (netId == NetSystem_GetNetId()) {
            return;
        }
        if (wk->yesNo != NULL) {
            ConfirmDialog_Free(wk->yesNo);
            wk->yesNo = NULL;
        }
        if (NetSystem_GetNetId() == 0) {
            wk->g2c->leavingBits &= 0xffff ^ netId;
        }
        break;
    case SPIN_TRADE_RECEIPT_SEQ_START:
        if (!SpinTradeReceipt_IsMessageDone(wk)) {
            PrintStream_Pause(wk->stream);
        }
        if (wk->yesNo != NULL) {
            ConfirmDialog_Free(wk->yesNo);
            wk->yesNo = NULL;
        }
        break;
    case SPIN_TRADE_RECEIPT_SEQ_CANCEL_OK:
    case SPIN_TRADE_RECEIPT_SEQ_CANCEL_REFUSED:
        break;
    default:
        return;
    }
    wk->seq = seq;
}

void SpinTradeReceipt_SetSeqOnJoin(SpinTradeReceiptWork *wk, int seq, u8 netId) {
    if (wk->seq == SPIN_TRADE_RECEIPT_SEQ_MAIN) {
        switch (seq) {
        case SPIN_TRADE_RECEIPT_SEQ_1: {
            UnionAppMember *member = UnionApp_GetMember(SpinTradeReceipt_GetUnionApp(wk), netId);

            if (member != NULL) {
                copyVarForText(wk->wordSet, 0, &member->info);
            }
            wk->seq = seq;
            wk->g2c->joinCount = UnionApp_CountJoined(SpinTradeReceipt_GetUnionApp(wk));
            wk->g2c->leavingBits = 0;
            break;
        }
        case SPIN_TRADE_RECEIPT_SEQ_MEMBER_LEFT:
            copyVarForText(wk->wordSet, 0, &UnionApp_GetMember(SpinTradeReceipt_GetUnionApp(wk), netId)->info);
            if (netId == NetSystem_GetNetId()) {
                return;
            }
            if (NetSystem_GetNetId() == 0) {
                wk->g2c->leavingBits &= 0xffff ^ netId;
            }
            wk->seq = seq;
            break;
        }
    } else if (seq == SPIN_TRADE_RECEIPT_SEQ_1) {
        wk->g2c->leavingBits = 0;
        wk->g2c->joinCount = UnionApp_CountJoined(SpinTradeReceipt_GetUnionApp(wk));
    }
}

static int SpinTradeReceipt_CountMembers(SpinTradeReceiptWork *wk) {
    int i;
    int count = 0;

    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++) {
        if (UnionApp_GetMember(SpinTradeReceipt_GetUnionApp(wk), i) != NULL) {
            count++;
        }
    }
    return count;
}

// Draws the members' names and IDs again if the members changed, and returns whether they did
static BOOL SpinTradeReceipt_DrawNames(BmpWin *window, int unused, u16 color, SpinTradeReceiptWork *wk) {
    int self = NetSystem_GetNetId();
    int i;

    if (!SpinTradeReceipt_UpdateMembers(wk)) {
        return FALSE;
    }
    GFL_BitmapFill(BmpWin_GetBitmap(window), 0);
    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++) {
        if (wk->members[i].member != NULL) {
            u16 id = getTrainerID(&wk->members[i].member->info);
            StrBuf *idBuf = GFL_StrBufCreate(16, HEAPID_SPIN_TRADE);
            StrBuf *format;
            u16 nameColor;

            textCopy(wk->members[i].member->info.name, wk->names[i]);
            WordSetNumber(wk->wordSet, 0, id, 5, 2, TRUE);
            format = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlPokeparam_Text_Id);
            GFL_WordSetFormatStrbuf(wk->wordSet, idBuf, format);
            if (self == i) {
                nameColor = SPIN_TRADE_RECEIPT_COLOR_SELF;
            } else {
                nameColor = color;
            }
            GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(window), 5, i * 18 + 1, wk->names[i], wk->font, nameColor);
            GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(window), 70, i * 18 + 1, idBuf, wk->font, nameColor);
            GFL_StrBufFree(idBuf);
            GFL_StrBufFree(format);
        }
    }
    BmpWin_TransferNow(window);
    return TRUE;
}

static void SpinTradeReceipt_Dummy(SpinTradeReceiptWork *wk) {
}

static int SpinTradeReceipt_CountMembers2(SpinTradeReceiptWork *wk) {
    int i;
    int count = 0;

    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++) {
        if (UnionApp_GetMember(SpinTradeReceipt_GetUnionApp(wk), i) != NULL) {
            count++;
        }
    }
    return count;
}

static u32 SpinTradeReceipt_GetJoined(SpinTradeReceiptWork *wk) {
    return UnionApp_GetJoined(SpinTradeReceipt_GetUnionApp(wk));
}

// Reads the members, sets what the trainer of each one that changed does, and returns whether any changed
static BOOL SpinTradeReceipt_UpdateMembers(SpinTradeReceiptWork *wk) {
    BOOL changed = FALSE;
    int i;

    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++) {
        wk->members[i].prev = wk->members[i].member;
        wk->members[i].member = UnionApp_GetMember(SpinTradeReceipt_GetUnionApp(wk), i);
        wk->ids[i].prev.parts = wk->ids[i].id.parts;
        if (wk->members[i].member != NULL) {
            wk->ids[i].id.parts.id = getIDAsUInt(&wk->members[i].member->info);
            wk->ids[i].id.parts.present = TRUE;
        } else {
            wk->ids[i].id.value = 0;
        }
    }
    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++) {
        if (wk->members[i].prev != wk->members[i].member) {
            changed = TRUE;
            if (wk->members[i].member == NULL) {
                wk->actorStates[i] = SPIN_TRADE_RECEIPT_ACTOR_LEAVE;
            } else {
                wk->actorStates[i] = SPIN_TRADE_RECEIPT_ACTOR_APPEAR;
            }
        } else if (wk->ids[i].id.value != wk->ids[i].prev.value) {
            changed = TRUE;
            if (!wk->ids[i].id.parts.present) {
                wk->actorStates[i] = SPIN_TRADE_RECEIPT_ACTOR_LEAVE;
            } else {
                wk->actorStates[i] = SPIN_TRADE_RECEIPT_ACTOR_APPEAR;
            }
        }
    }
    return changed;
}

static void SpinTradeReceipt_PrintMessage(SpinTradeReceiptWork *wk, u32 messageId, int unused) {
    StrBuf *format = GFL_StrBufCreate(180, HEAPID_SPIN_TRADE);

    GFL_MsgDataLoadStrbuf(wk->msgData, messageId, format);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->msgBuf, format);
    GFL_StrBufFree(format);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->msgWindow), 15);
    BmpWin_DrawFrame(wk->msgWindow, FALSE, 1, 10);
    if (wk->stream != NULL) {
        PrintStream_Delete(wk->stream);
    }
    wk->stream =
        PrintStream_Create(wk->msgWindow, 0, 0, wk->msgBuf, wk->font, 0, wk->tcbManager, 0, HEAPID_SPIN_TRADE, 15);
    BmpWin_TransferNow(wk->msgWindow);
}

static BOOL SpinTradeReceipt_IsMessageDone(SpinTradeReceiptWork *wk) {
    u32 state;

    if (wk->stream == NULL) {
        return TRUE;
    }
    state = PrintStream_GetState(wk->stream);
    if (state == PRINT_STREAM_RUNNING) {
        if (GCTX_HIDGetHeldKeys() & PAD_BUTTON_A) {
            PrintStream_SkipWait(wk->stream, 0);
        }
    } else if (state == PRINT_STREAM_PAUSED) {
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            PrintStream_Continue(wk->stream);
        }
    } else if (state == PRINT_STREAM_DONE) {
        PrintStream_Delete(wk->stream);
        wk->stream = NULL;
        return TRUE;
    }
    return FALSE;
}

static void SpinTradeReceipt_ClearMessage(SpinTradeReceiptWork *wk) {
    BmpWin_ClearFrame(wk->msgWindow, FALSE);
}

static void SpinTradeReceipt_UpdateActors(SpinTradeReceiptWork *wk) {
    int i;
    BOOL appeared = FALSE;

    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++) {
        switch (wk->actorStates[i]) {
        case SPIN_TRADE_RECEIPT_ACTOR_APPEAR: {
            int gender = getTrainerGender(&wk->members[i].member->info);
            int look = PlayerInfo_GetUnionView(&wk->members[i].member->info);

            if (i == NetSystem_GetNetId()) {
                ClActor_SetSequence(wk->actors[i], 4);
            } else {
                SpinTradeReceipt_SetActorLook(wk, wk->trainerPalette, i, look, gender);
                ClActor_SetSequence(wk->actors[i], 4);
            }
            ClActor_SetVisible(wk->actors[i], TRUE);
            wk->actorStates[i] = SPIN_TRADE_RECEIPT_ACTOR_SHOWN;
            appeared = TRUE;
            break;
        }
        case SPIN_TRADE_RECEIPT_ACTOR_LEAVE:
            if (i == NetSystem_GetNetId()) {
                getTrainerGender(&wk->members[i].member->info);
                ClActor_SetSequence(wk->actors[i], 5);
            } else {
                ClActor_SetSequence(wk->actors[i], 5);
            }
            wk->actorStates[i] = SPIN_TRADE_RECEIPT_ACTOR_NONE;
            break;
        case SPIN_TRADE_RECEIPT_ACTOR_NONE:
        case SPIN_TRADE_RECEIPT_ACTOR_SHOWN:
            break;
        }
    }
    if (appeared) {
        GFL_SndSEPlay(SEQ_SE_FLD_05);
    }
}

static void SpinTradeReceipt_LoadTrainerPalette(SpinTradeReceiptWork *wk, ArcTool *arc) {
    wk->trainerPaletteFile = GFL_G2DIOReadNCLR(ARCID_WIFI_UNIONCHAR, 0, &wk->trainerPalette, HEAPID_SPIN_TRADE);
}

static void SpinTradeReceipt_FreeTrainerPalette(SpinTradeReceiptWork *wk) {
    GFL_HeapFree(wk->trainerPaletteFile);
}

// The row of the trainer palette file for each appearance (PlayerInfo_GetUnionView)
static const u8 sLookPalettes[16] = { 2, 3, 6, 5, 4, 5, 2, 0, 1, 3, 6, 5, 4, 7, 7, 0 };

// Gives a member's actor the palette and characters of the member's appearance
static void SpinTradeReceipt_SetActorLook(SpinTradeReceiptWork *wk, NNSG2dPaletteData *palette, int id, int look,
                                          int gender) {
    NNSG2dCharacterData *character;
    void *file;

    GX_LoadOBJPltt((u8 *)palette->pRawData + sLookPalettes[look] * 0x20, id * 0x20, 0x20);
    file = GFL_G2DIOReadOBJNCGR(ARCID_WIFI_UNIONCHAR, look + 1, FALSE, &character, HEAPID_SPIN_TRADE);
    ClActSys_ReplaceCharData(wk->resources[SPIN_TRADE_RECEIPT_RES_CHAR + id], character);
    GFL_HeapFree(file);
}

static inline int SpinTradeReceipt_GetMemberCount(const SpinTradeReceiptWork *wk) {
    return wk->memberCount;
}

// The parent's check of the members each frame, which returns the proc's next sequence
static int SpinTradeReceipt_CheckMembers(SpinTradeReceiptWork *wk, int unused) {
    int count = SpinTradeReceipt_CountMembers2(wk);

    if (count > wk->g2c->joinCount) {
        if (UnionApp_CloseEntry(SpinTradeReceipt_GetUnionApp(wk))) {
            wk->entryClosed = TRUE;
        }
    } else {
        wk->entryClosed = FALSE;
    }
    if (count == wk->memberCount) {
        return SPIN_TRADE_RECEIPT_PROC_MAIN;
    }
    switch (count) {
    case 1:
        if (UnionApp_CountJoined(SpinTradeReceipt_GetUnionApp(wk)) > 1 ||
            UnionApp_GetJoined(SpinTradeReceipt_GetUnionApp(wk)) > 1) {
            return SPIN_TRADE_RECEIPT_PROC_MAIN;
        }
        wk->seq = SPIN_TRADE_RECEIPT_SEQ_NO_MEMBERS_MSG_WAIT;
        if (wk->yesNo != NULL) {
            ConfirmDialog_Free(wk->yesNo);
            wk->yesNo = NULL;
        }
        return SPIN_TRADE_RECEIPT_PROC_MAIN_NO_UPDATE;
    case 2:
    case 3:
    case 4:
        if (count < SpinTradeReceipt_GetMemberCount(wk)) {
            wk->g2c->joinCount = UnionApp_CountJoined(SpinTradeReceipt_GetUnionApp(wk));
        }
        break;
    case 0:
    case 5:
        break;
    }
    wk->memberCount = SpinTradeReceipt_CountMembers2(wk);
    return SPIN_TRADE_RECEIPT_PROC_MAIN;
}

static u32 SpinTradeReceipt_GetPressedKeys(void) {
    return GCTX_HIDGetPressedKeys();
}
