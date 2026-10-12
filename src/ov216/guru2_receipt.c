#include "types.h"
#include "app/guru2.h"
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
// for the parent. The ROM has no name for the file; it is named after pokeplatinum's guru2_receipt.c, the same screen
// in Gen 4. Names are ours

#define GURU2_RECEIPT_ARCID 181
#define GURU2_RECEIPT_OBJ_ARCID 30
#define GURU2_RECEIPT_TRAINER_ARCID 31

// The proc's sequences
enum {
    GURU2_RECEIPT_PROC_FADE_IN,
    GURU2_RECEIPT_PROC_MAIN,
    GURU2_RECEIPT_PROC_MAIN_NO_UPDATE,
    GURU2_RECEIPT_PROC_FADE_OUT,
    GURU2_RECEIPT_PROC_WAIT_START,
};

// The cell actor resources: a palette and the characters of each member's trainer, and the cells
enum {
    GURU2_RECEIPT_RES_PLTT,
    GURU2_RECEIPT_RES_CHAR = GURU2_RECEIPT_RES_PLTT + GURU2_MEMBER_MAX,
    GURU2_RECEIPT_RES_CELL = GURU2_RECEIPT_RES_CHAR + GURU2_MEMBER_MAX,
    GURU2_RECEIPT_RES_COUNT,
};

// What a member's trainer does next
enum {
    GURU2_RECEIPT_ACTOR_NONE,
    GURU2_RECEIPT_ACTOR_APPEAR,
    GURU2_RECEIPT_ACTOR_SHOWN,
    GURU2_RECEIPT_ACTOR_LEAVE,
};

#define GURU2_RECEIPT_COLOR_NAME PRINT_COLOR(1, 4, 0)
#define GURU2_RECEIPT_COLOR_SELF PRINT_COLOR(2, 3, 0)

// A palette animation of the trainers, which nothing starts
typedef struct {
    BOOL active;
    int index;
    int counter;
    void *file;
    NNSG2dPaletteData *palette;
} Guru2ReceiptPalAnim;

typedef struct {
    u8 wait;
    // The palette's row in the file, 0xff at the end
    u8 palette;
} Guru2ReceiptPalAnimStep;

typedef struct {
    UnionAppMember *member;
    UnionAppMember *prev;
} Guru2ReceiptMember;

typedef union {
    struct {
        u32 id;
        BOOL present;
    } parts;
    u64 value;
} Guru2ReceiptId;

// A member's trainer ID, and what it was before the last update
typedef struct {
    Guru2ReceiptId id;
    Guru2ReceiptId prev;
} Guru2ReceiptMemberId;

typedef struct {
    u16 x;
    u16 y;
} Guru2ReceiptPos;

struct Guru2ReceiptWork {
    int procSeq;
    u8 unk04[4];
    BOOL receiptDone;
    Guru2ProcWork *g2p;
    Guru2CommWork *g2c;
    u8 unk14[4];
    Guru2ReceiptPalAnim palAnim;
    u8 unk2C[4];
    TCB *vblankTcb;
    ArcTool *trainerArc;
    Font *font;
    WordSet *wordSet;
    MsgData *msgData;
    StrBuf *names[GURU2_MEMBER_MAX];
    u8 unk58[4];
    StrBuf *msgBuf;
    StrBuf *titleBuf;
    u8 unk64[4];
    ClActUnit *actorUnit;
    u32 resources[GURU2_RECEIPT_RES_COUNT];
    u8 unk98[4];
    ClActor *actors[GURU2_MEMBER_MAX];
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
    // The sequence after GURU2_RECEIPT_SEQ_MSG_WAIT
    int nextSeq;
    u8 unk120[4];
    int wait;
    u8 unk128[0x10];
    Guru2ReceiptMember members[GURU2_MEMBER_MAX];
    int actorStates[GURU2_MEMBER_MAX];
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
    Guru2ReceiptMemberId ids[GURU2_MEMBER_MAX];
};

typedef int (*Guru2ReceiptSeqFunc)(Guru2ReceiptWork *wk, int procSeq);

static UnionApp *Guru2Receipt_GetUnionApp(Guru2ReceiptWork *wk);
static void Guru2Receipt_OnJoin(u8 netId, UnionAppMember *member, void *work);
static void Guru2Receipt_OnLeave(u8 netId, UnionAppMember *member, void *work);
static void Guru2Receipt_UpdatePalAnim(TCB *tcb, void *data);
static void Guru2Receipt_VBlank(TCB *tcb, void *data);
static void Guru2Receipt_UpdatePrint(Guru2ReceiptWork *wk);
static void Guru2Receipt_SetVRAMBanks(void);
static void Guru2Receipt_InitBG(void);
static void Guru2Receipt_InitWork(Guru2ReceiptWork *wk, ArcTool *arc);
static void Guru2Receipt_FreeWork(Guru2ReceiptWork *wk);
static void Guru2Receipt_ExitBG(void);
static void Guru2Receipt_LoadBG(Guru2ReceiptWork *wk, ArcTool *arc);
static void Guru2Receipt_InitActorSys(Guru2ReceiptWork *wk, ArcTool *arc);
static void Guru2Receipt_ExitActorSys(Guru2ReceiptWork *wk);
static void Guru2Receipt_CreateActors(Guru2ReceiptWork *wk);
static void Guru2Receipt_CreateWindows(Guru2ReceiptWork *wk);
static void Guru2Receipt_DrawTitle(BmpWin *window, StrBuf *title, Font *font);
static void Guru2Receipt_FreeWindows(Guru2ReceiptWork *wk);
static int Guru2ReceiptSeq_Init(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_Main(Guru2ReceiptWork *wk, int procSeq);
static void Guru2Receipt_CheckKeys(Guru2ReceiptWork *wk);
static void Guru2Receipt_SeqEnd(Guru2ReceiptWork *wk);
static int Guru2ReceiptSeq_1(Guru2ReceiptWork *wk, int procSeq);
static void Guru2Receipt_SetMsgWait(Guru2ReceiptWork *wk, int nextSeq);
static int Guru2ReceiptSeq_MsgWait(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_2(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_CancelYesNoInit(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_CancelYesNo(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_CancelSignalWait(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_CancelOk(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_CancelRefused(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_StartYesNoInit(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_StartYesNo(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_StartSend(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_StartWait(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_EndMsg(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_EndWait(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_ParentCancelYesNoInit(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_ParentCancelYesNo(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_ParentCancelMsg(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_ParentCancelWait(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_ParentCancelSync(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_ParentCancelSyncWait(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_NoMembersMsgWait(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_NoMembersEnd(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_MemberLeft(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_MemberLeft2(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_MemberLeftWait(Guru2ReceiptWork *wk, int procSeq);
static int Guru2ReceiptSeq_Start(Guru2ReceiptWork *wk, int procSeq);
static int Guru2Receipt_CountMembers(Guru2ReceiptWork *wk);
static BOOL Guru2Receipt_DrawNames(BmpWin *window, int unused, u16 color, Guru2ReceiptWork *wk);
static void Guru2Receipt_Dummy(Guru2ReceiptWork *wk);
static int Guru2Receipt_CountMembers2(Guru2ReceiptWork *wk);
static u32 Guru2Receipt_GetJoined(Guru2ReceiptWork *wk);
static BOOL Guru2Receipt_UpdateMembers(Guru2ReceiptWork *wk);
static void Guru2Receipt_PrintMessage(Guru2ReceiptWork *wk, u32 messageId, int unused);
static BOOL Guru2Receipt_IsMessageDone(Guru2ReceiptWork *wk);
static void Guru2Receipt_ClearMessage(Guru2ReceiptWork *wk);
static void Guru2Receipt_UpdateActors(Guru2ReceiptWork *wk);
static void Guru2Receipt_LoadTrainerPalette(Guru2ReceiptWork *wk, ArcTool *arc);
static void Guru2Receipt_FreeTrainerPalette(Guru2ReceiptWork *wk);
static void Guru2Receipt_SetActorLook(Guru2ReceiptWork *wk, NNSG2dPaletteData *palette, int id, int look, int gender);
static int Guru2Receipt_CheckMembers(Guru2ReceiptWork *wk, int unused);
static u32 Guru2Receipt_GetPressedKeys(void);

static const ConfirmDialogSetup sYesNoSetup = { 0, 23, 13, 13, 0xd8 };

static const Guru2ReceiptSeqFunc sSeqFuncs[GURU2_RECEIPT_SEQ_COUNT] = {
    Guru2ReceiptSeq_Init,
    Guru2ReceiptSeq_1,
    Guru2ReceiptSeq_2,
    Guru2ReceiptSeq_Main,
    Guru2ReceiptSeq_CancelYesNoInit,
    Guru2ReceiptSeq_CancelYesNo,
    Guru2ReceiptSeq_CancelSignalWait,
    Guru2ReceiptSeq_CancelOk,
    Guru2ReceiptSeq_CancelRefused,
    Guru2ReceiptSeq_EndMsg,
    Guru2ReceiptSeq_EndWait,
    Guru2ReceiptSeq_ParentCancelYesNoInit,
    Guru2ReceiptSeq_ParentCancelYesNo,
    Guru2ReceiptSeq_ParentCancelMsg,
    Guru2ReceiptSeq_ParentCancelWait,
    Guru2ReceiptSeq_ParentCancelSync,
    Guru2ReceiptSeq_ParentCancelSyncWait,
    Guru2ReceiptSeq_NoMembersMsgWait,
    Guru2ReceiptSeq_NoMembersEnd,
    Guru2ReceiptSeq_MemberLeft,
    Guru2ReceiptSeq_MemberLeft2,
    Guru2ReceiptSeq_MemberLeftWait,
    Guru2ReceiptSeq_StartYesNoInit,
    Guru2ReceiptSeq_StartYesNo,
    Guru2ReceiptSeq_StartSend,
    Guru2ReceiptSeq_StartWait,
    Guru2ReceiptSeq_MsgWait,
    Guru2ReceiptSeq_Start,
};

BOOL Guru2Receipt_Init(GameProc *proc, u32 *state, void *param, void *work) {
    Guru2ProcWork *g2p = param;
    Guru2ReceiptWork *wk;
    ArcTool *arc;

    switch (*state) {
    case 0:
        GFL_BGSysDisableAllA();
        GFL_BGSysDisableAllB();
        GX_SetVisiblePlane(0);
        GXS_SetVisiblePlane(0);
        GFL_HeapCreateChild(HEAPID_USER, HEAPID_GURU2, 0x80000);
        arc = GFL_ArcSysCreateFileHandle(GURU2_RECEIPT_ARCID, HEAPID_GURU2);
        wk = GFL_ProcInitSubsystem(proc, sizeof(Guru2ReceiptWork), HEAPID_GURU2);
        sys_memset(wk, 0, sizeof(Guru2ReceiptWork));
        g2p->g2r = wk;
        wk->g2p = g2p;
        wk->g2c = g2p->g2c;
        GFL_BGSysCreate(HEAPID_GURU2);
        wk->wordSet = GFL_WordSetSystemCreateDefault(HEAPID_GURU2);
        wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_POKEPARAM, HEAPID_GURU2);
        wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_GURU2);
        Guru2Receipt_SetVRAMBanks();
        Guru2Receipt_InitBG();
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 16, 1,
                    HEAPID_GURU2);
        Guru2Receipt_LoadBG(wk, arc);
        wk->vblankTcb = GFL_VBlankTCBAdd(Guru2Receipt_VBlank, wk, 1);
        wk->tcbManager = GFL_TCBExMgrCreate(HEAPID_GURU2, HEAPID_GURU2, 32, 32);
        wk->printQueue = func_02021998(HEAPID_GURU2);
        Guru2Receipt_InitWork(wk, arc);
        Guru2Receipt_InitActorSys(wk, arc);
        Guru2Receipt_CreateActors(wk);
        Guru2Receipt_CreateWindows(wk);
        Guru2Comm_AddCommands(g2p->g2c);
        func_02042ba8(TRUE, HEAPID_GURU2);
        GFL_ArcToolFree(arc);
        (*state)++;
        break;
    case 1:
        *state = 0;
        return TRUE;
    }
    return FALSE;
}

static UnionApp *Guru2Receipt_GetUnionApp(Guru2ReceiptWork *wk) {
    return wk->g2p->param->app;
}

static void Guru2Receipt_OnJoin(u8 netId, UnionAppMember *member, void *work) {
    Guru2ReceiptWork *wk = work;

    if (func_0203ffc4() == 0) {
        u8 id = netId;

        Guru2Comm_Send(wk->g2c, GURU2_NET_CMD_JOIN, &id, sizeof(id));
    }
}

static void Guru2Receipt_OnLeave(u8 netId, UnionAppMember *member, void *work) {
}

BOOL Guru2Receipt_Main(GameProc *proc, u32 *state, void *param, void *work) {
    Guru2ReceiptWork *wk = work;

    if (func_0203ffc4() == 0 && wk->g2c->leavingBits != 0) {
        wk->g2c->leavingBits &= UnionApp_GetJoined(Guru2Receipt_GetUnionApp(wk));
    }
    switch (wk->procSeq) {
    case GURU2_RECEIPT_PROC_FADE_IN:
        if (GFL_WipeIsFinished()) {
            wk->procSeq = GURU2_RECEIPT_PROC_MAIN;
            UnionApp_OpenEntry(Guru2Receipt_GetUnionApp(wk));
            UnionApp_SetCallbacks(Guru2Receipt_GetUnionApp(wk), Guru2Receipt_OnJoin, Guru2Receipt_OnLeave, wk);
        }
        break;
    case GURU2_RECEIPT_PROC_MAIN:
        if (sSeqFuncs[wk->seq] != NULL) {
            wk->procSeq = sSeqFuncs[wk->seq](wk, wk->procSeq);
        }
        if (!wk->g2c->started) {
            Guru2Receipt_DrawNames(wk->nameWindow, 0, GURU2_RECEIPT_COLOR_NAME, wk);
        }
        Guru2Receipt_UpdateActors(wk);
        if (func_0203ffc4() == 0) {
            int next = Guru2Receipt_CheckMembers(wk, 1);

            if (wk->procSeq == GURU2_RECEIPT_PROC_MAIN) {
                wk->procSeq = next;
            }
        }
        break;
    case GURU2_RECEIPT_PROC_MAIN_NO_UPDATE:
        if (sSeqFuncs[wk->seq] != NULL) {
            wk->procSeq = sSeqFuncs[wk->seq](wk, wk->procSeq);
        }
        break;
    case GURU2_RECEIPT_PROC_FADE_OUT:
        if (GFL_WipeIsFinished()) {
            if (wk->receiptDone) {
                return TRUE;
            }
            func_02040c64(GURU2_NET_CMD_BASE);
            UnionApp_RequestStart(Guru2Receipt_GetUnionApp(wk));
            wk->procSeq = GURU2_RECEIPT_PROC_WAIT_START;
        }
        break;
    case GURU2_RECEIPT_PROC_WAIT_START:
        if (UnionApp_IsStarted(Guru2Receipt_GetUnionApp(wk))) {
            return TRUE;
        }
        break;
    }
    if (GFL_WipeIsFinished() && GFL_NetErrCheck()) {
        return TRUE;
    }
    func_0204b794();
    Guru2Receipt_UpdatePrint(wk);
    return FALSE;
}

BOOL Guru2Receipt_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    Guru2ReceiptWork *wk = work;

    GFL_TCBExMgrFreeTasks(wk->tcbManager);
    GFL_TCBExMgrFree(wk->tcbManager);
    func_02021c44(wk->printQueue);
    func_02021a18(wk->printQueue);
    Guru2Receipt_ExitActorSys(wk);
    Guru2Receipt_FreeWindows(wk);
    Guru2Receipt_ExitBG();
    GFL_MsgDataFree(wk->msgData);
    GFL_WordSetSystemFree(wk->wordSet);
    GFL_FontFree(wk->font);
    GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
    GFL_TCBRemove(wk->vblankTcb);
    wk->g2p->receiptDone = wk->receiptDone;
    wk->g2p->memberCount = Guru2Receipt_CountMembers2(wk);
    wk->g2p->memberBits = Guru2Receipt_GetJoined(wk);
    Guru2Receipt_FreeWork(wk);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_GURU2);
    return TRUE;
}

static const Guru2ReceiptPalAnimStep sPalAnimSteps[] = {
    { 8, 2 },
    { 8, 3 },
    { 8, 4 },
    { 24, 0 },
    { 0, 0xff },
};

static void Guru2Receipt_UpdatePalAnim(TCB *tcb, void *data) {
    Guru2ReceiptWork *wk = data;
    Guru2ReceiptPalAnim *anim = &wk->palAnim;

    if (anim->active) {
        if (anim->counter > sPalAnimSteps[anim->index].wait) {
            anim->index++;
            anim->counter = 0;
            if (sPalAnimSteps[anim->index].palette == 0xff) {
                anim->index = 0;
            }
            gfxUploadStdPaletteObjA((u8 *)anim->palette->rawData + sPalAnimSteps[anim->index].palette * 0x20, 0, 0x20);
        } else {
            wk->palAnim.counter++;
        }
    }
}

static void Guru2Receipt_VBlank(TCB *tcb, void *data) {
    Guru2Receipt_UpdatePalAnim(tcb, data);
    func_0204b7c8();
    GFL_BGSysUpdate();
}

static void Guru2Receipt_UpdatePrint(Guru2ReceiptWork *wk) {
    if (wk->printQueue != NULL) {
        func_02021a3c(wk->printQueue);
    }
    GFL_TCBExMgrUpdate(wk->tcbManager);
}

static const BGSysVRAMConfig sVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static void Guru2Receipt_SetVRAMBanks(void) {
    GFL_BGSysSetVRAMBanks(&sVRAMConfig);
}

static void Guru2Receipt_InitBG(void) {
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
        GFL_BGSysClearCharCore(4, 0x20, 0, HEAPID_GURU2);
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
        GFL_BGSysClearCharCore(0, 0x20, 0, HEAPID_GURU2);
    }
}

static void Guru2Receipt_InitWork(Guru2ReceiptWork *wk, ArcTool *arc) {
    int i;

    for (i = 0; i < GURU2_MEMBER_MAX; i++) {
        wk->names[i] = GFL_StrBufCreate(8, HEAPID_GURU2);
        wk->members[i].member = NULL;
        wk->members[i].prev = NULL;
        wk->actorStates[i] = GURU2_RECEIPT_ACTOR_NONE;
    }
    wk->msgBuf = GFL_StrBufCreate(180, HEAPID_GURU2);
    wk->titleBuf = GFL_StrBufCreate(40, HEAPID_GURU2);
    wk->seq = GURU2_RECEIPT_SEQ_INIT;
    GFL_MsgDataLoadStrbuf(wk->msgData, BtlPokeparam_Text_SeekingSpinTrade, wk->titleBuf);
    Guru2Receipt_LoadTrainerPalette(wk, arc);
    wk->palAnim.active = FALSE;
    wk->palAnim.counter = 0;
    wk->palAnim.index = 0;
    wk->palAnim.file = GFL_G2DIOReadNCLRArc(arc, 2, &wk->palAnim.palette, HEAPID_GURU2);
    wk->memberCount = 0;
    wk->yesNo = NULL;
    wk->g2c->joinCount = 2;
}

static void Guru2Receipt_FreeWork(Guru2ReceiptWork *wk) {
    int i;

    Guru2Receipt_FreeTrainerPalette(wk);
    GFL_HeapFree(wk->palAnim.file);
    for (i = 0; i < GURU2_MEMBER_MAX; i++) {
        GFL_StrBufFree(wk->names[i]);
    }
    GFL_StrBufFree(wk->titleBuf);
    GFL_StrBufFree(wk->msgBuf);
}

static void Guru2Receipt_ExitBG(void) {
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(0);
    GFL_BGSysFree();
}

static void Guru2Receipt_LoadBG(Guru2ReceiptWork *wk, ArcTool *arc) {
    int gender = getTrainerGender(GetGameDataPlayerInfo(wk->g2p->param->gameData));

    GFL_G2DIOLoadArcNCLRDefault(arc, 1, PALTYPE_MAIN_BG, 0, 0x200, HEAPID_GURU2);
    GFL_BGSysLoadArcNCGRStatic(arc, 7, 1, 0, 0x2000, FALSE, HEAPID_GURU2);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 13, 1, 0, 0x600, FALSE, HEAPID_GURU2);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 13 * 0x20, 0x20, HEAPID_GURU2);
    GFL_BGSysLoadNCLRDefault(ARCID_C_GEAR, gender % 2, PALTYPE_SUB_BG, 0, 0x160, HEAPID_GURU2);
    GFL_BGSysLoadNCGRStatic(ARCID_C_GEAR, 2, 6, 0, 0, FALSE, HEAPID_GURU2);
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_C_GEAR, 3, 6, 0, 0, FALSE, HEAPID_GURU2);
    LoadSysMsgBox(0, 1, 10, 0, HEAPID_GURU2);
    LoadSysMsgBox(0, 31, 11, 0, HEAPID_GURU2);
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

static void Guru2Receipt_InitActorSys(Guru2ReceiptWork *wk, ArcTool *unused) {
    int gender = getTrainerGender(GetGameDataPlayerInfo(wk->g2p->param->gameData));
    ArcTool *arc = GFL_ArcSysCreateFileHandle(GURU2_RECEIPT_OBJ_ARCID, HEAPID_GURU2);
    int i;

    ClActSys_Create(&data_02093f08, &sVRAMConfig, HEAPID_GURU2);
    wk->actorUnit = func_0204bf1c(30, 1, HEAPID_GURU2);
    for (i = 0; i < GURU2_MEMBER_MAX; i++) {
        wk->resources[GURU2_RECEIPT_RES_CHAR + i] =
            func_0204b81c(arc, sActorFiles[0][gender], FALSE, 0, HEAPID_GURU2);
        wk->resources[GURU2_RECEIPT_RES_PLTT + i] =
            func_0204bbb8(arc, sActorFiles[1][gender], 0, i * 0x20, 0, 1, HEAPID_GURU2);
    }
    wk->resources[GURU2_RECEIPT_RES_CELL] =
        func_0204bde0(arc, sActorFiles[2][gender], sActorFiles[3][gender], HEAPID_GURU2);
    GFL_ArcToolFree(arc);
    wk->trainerArc = GFL_ArcSysCreateFileHandle(GURU2_RECEIPT_TRAINER_ARCID, HEAPID_GURU2);
}

static void Guru2Receipt_ExitActorSys(Guru2ReceiptWork *wk) {
    int i;

    GFL_ArcToolFree(wk->trainerArc);
    for (i = 0; i < GURU2_MEMBER_MAX; i++) {
        // BUG: the palettes are freed as characters and the characters as palettes. Each kind's handles count
        // from 0, so the five of one kind are the five of the other
#ifdef BUGFIX
        func_0204b98c(wk->resources[GURU2_RECEIPT_RES_CHAR + i]);
        func_0204bcd0(wk->resources[GURU2_RECEIPT_RES_PLTT + i]);
#else
        func_0204b98c(wk->resources[GURU2_RECEIPT_RES_PLTT + i]);
        func_0204bcd0(wk->resources[GURU2_RECEIPT_RES_CHAR + i]);
#endif
    }
    func_0204be64(wk->resources[GURU2_RECEIPT_RES_CELL]);
    func_0204bf98(wk->actorUnit);
    func_0204b758();
}

// Where the trainers stand: the middle, then each member's place
static const Guru2ReceiptPos sActorPositions[1 + GURU2_MEMBER_MAX] = {
    { 204, 84 },
    { 204, 53 },
    { 233, 72 },
    { 223, 103 },
    { 184, 103 },
    { 174, 72 },
};

static void Guru2Receipt_CreateActors(Guru2ReceiptWork *wk) {
    ClActorSetup setup;
    int i;

    setup.bgPriority = 1;
    setup.priority = 0;
    setup.sequence = 0;
    for (i = 0; i < GURU2_MEMBER_MAX; i++) {
        setup.x = sActorPositions[i + 1].x;
        setup.y = sActorPositions[i + 1].y;
        wk->actors[i] = func_0204c040(wk->actorUnit, wk->resources[GURU2_RECEIPT_RES_CHAR + i],
                                      wk->resources[GURU2_RECEIPT_RES_PLTT + i],
                                      wk->resources[GURU2_RECEIPT_RES_CELL], &setup, 0, HEAPID_GURU2);
        func_0204c520(wk->actors[i], TRUE);
        func_0204c488(wk->actors[i], 0);
        func_0204c124(wk->actors[i], FALSE);
    }
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, FALSE);
}

static void Guru2Receipt_CreateWindows(Guru2ReceiptWork *wk) {
    BmpWin_InitAllocator(HEAPID_GURU2);
    wk->subWindow = BmpWin_CreateDynamic(5, 26, 20, 6, 2, 13, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->subWindow), 0);
    wk->msgWindow = BmpWin_CreateDynamic(0, 2, 19, 27, 4, 13, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->msgWindow), 15);
    wk->titleWindow = BmpWin_CreateDynamic(0, 3, 1, 26, 2, 15, TRUE);
    Guru2Receipt_DrawTitle(wk->titleWindow, wk->titleBuf, wk->font);
    wk->nameWindow = BmpWin_CreateDynamic(0, 2, 6, 16, 11, 15, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->nameWindow), 0);
    Guru2Receipt_DrawNames(wk->nameWindow, 0, GURU2_RECEIPT_COLOR_NAME, wk);
    wk->unkF8 = wk->titleWindow;
    wk->unkFC = 0;
}

static void Guru2Receipt_DrawTitle(BmpWin *window, StrBuf *title, Font *font) {
    int width = GFL_FontGetBlockWidth(title, font, 0);

    GFL_BitmapFill(BmpWin_GetBitmap(window), 0);
    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(window), (26 * 8 - width) / 2, 1, title, font,
                                   GURU2_RECEIPT_COLOR_NAME);
    BmpWin_TransferNow(window);
}

static void Guru2Receipt_FreeWindows(Guru2ReceiptWork *wk) {
    if (wk->yesNo != NULL) {
        ConfirmDialog_Free(wk->yesNo);
    }
    BmpWin_Free(wk->nameWindow);
    BmpWin_Free(wk->titleWindow);
    BmpWin_Free(wk->subWindow);
    BmpWin_Free(wk->msgWindow);
    BmpWin_FreeAllocator();
}

static int Guru2ReceiptSeq_Init(Guru2ReceiptWork *wk, int procSeq) {
    if (func_0203ffc4() == 0) {
        func_02042e94(FALSE);
        if (UnionApp_CountJoined(Guru2Receipt_GetUnionApp(wk)) >= 2) {
            Guru2Receipt_PrintMessage(wk, BtlPokeparam_Text_WhenGroupReadyButton, 0);
        }
    } else {
        func_02042e94(TRUE);
        Guru2Receipt_PrintMessage(wk, BtlPokeparam_Text_PleaseWaitBButton, 0);
    }
    Guru2Receipt_SetMsgWait(wk, GURU2_RECEIPT_SEQ_MAIN);
    return procSeq;
}

static int Guru2ReceiptSeq_Main(Guru2ReceiptWork *wk, int procSeq) {
    Guru2Receipt_CheckKeys(wk);
    if (func_0203ffc4() == 0) {
        if (Guru2Receipt_CountMembers(wk) != 1) {
            Guru2Receipt_Dummy(wk);
        }
    } else {
        Guru2Receipt_Dummy(wk);
    }
    return procSeq;
}

static void Guru2Receipt_CheckKeys(Guru2ReceiptWork *wk) {
    Guru2CommWork *g2c;

    if (Guru2Receipt_GetPressedKeys() & PAD_BUTTON_A) {
        if (func_0203ffc4() == 0) {
            g2c = wk->g2c;
            if (g2c->joinCount == Guru2Receipt_CountMembers2(wk) && g2c->leavingBits == 0) {
                if (UnionApp_CloseEntry(Guru2Receipt_GetUnionApp(wk))) {
                    GFL_SndSEPlay(SEQ_SE_DECIDE1);
                    Guru2Receipt_PrintMessage(wk, BtlPokeparam_Text_GroupOk, 0);
                    Guru2Receipt_SetMsgWait(wk, GURU2_RECEIPT_SEQ_START_YESNO_INIT);
                }
            } else {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
        }
    } else if (Guru2Receipt_GetPressedKeys() & PAD_BUTTON_B) {
        if (func_0203ffc4() != 0) {
            if (wk->g2c->unk28 == 0) {
                Guru2Receipt_PrintMessage(wk, BtlPokeparam_Text_CancelSpinTrade, 0);
                Guru2Receipt_SetMsgWait(wk, GURU2_RECEIPT_SEQ_CANCEL_YESNO_INIT);
            } else {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
        } else {
            g2c = wk->g2c;
            if (g2c->joinCount == UnionApp_CountJoined(Guru2Receipt_GetUnionApp(wk)) && g2c->leavingBits == 0) {
                if (UnionApp_CloseEntry(Guru2Receipt_GetUnionApp(wk))) {
                    Guru2Receipt_PrintMessage(wk, BtlPokeparam_Text_CancelSpinTrade, 0);
                    Guru2Receipt_SetMsgWait(wk, GURU2_RECEIPT_SEQ_CANCEL_YESNO_INIT);
                    GFL_SndSEPlay(SEQ_SE_CANCEL1);
                }
            } else {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
        }
    } else {
        if (!wk->entryClosed && func_0203ffc4() == 0
            && wk->g2c->joinCount == UnionApp_CountJoined(Guru2Receipt_GetUnionApp(wk))) {
            UnionApp_OpenEntry(Guru2Receipt_GetUnionApp(wk));
        }
    }
}

// Runs at the end of each sequence
static void Guru2Receipt_SeqEnd(Guru2ReceiptWork *wk) {
}

static int Guru2ReceiptSeq_1(Guru2ReceiptWork *wk, int procSeq) {
    wk->seq = GURU2_RECEIPT_SEQ_2;
    Guru2Receipt_SeqEnd(wk);
    return procSeq;
}

static void Guru2Receipt_SetMsgWait(Guru2ReceiptWork *wk, int nextSeq) {
    wk->seq = GURU2_RECEIPT_SEQ_MSG_WAIT;
    wk->nextSeq = nextSeq;
}

static int Guru2ReceiptSeq_MsgWait(Guru2ReceiptWork *wk, int procSeq) {
    if (Guru2Receipt_IsMessageDone(wk)) {
        wk->seq = wk->nextSeq;
    }
    Guru2Receipt_SeqEnd(wk);
    return procSeq;
}

static int Guru2ReceiptSeq_2(Guru2ReceiptWork *wk, int procSeq) {
    if (func_0203ffc4() == 0) {
        UnionApp_OpenEntry(Guru2Receipt_GetUnionApp(wk));
    }
    wk->seq = GURU2_RECEIPT_SEQ_MAIN;
    Guru2Receipt_SeqEnd(wk);
    return procSeq;
}

static int Guru2ReceiptSeq_CancelYesNoInit(Guru2ReceiptWork *wk, int procSeq) {
    wk->yesNo = ShopUI_CreateConfirmDialog(&sYesNoSetup, 31, 11, 0, HEAPID_GURU2);
    wk->seq = GURU2_RECEIPT_SEQ_CANCEL_YESNO;
    Guru2Receipt_SeqEnd(wk);
    return procSeq;
}

static int Guru2ReceiptSeq_CancelYesNo(Guru2ReceiptWork *wk, int procSeq) {
    u32 result;

    if (func_0203ffc4() != 0) {
        if (wk->g2c->unk28 != 0) {
            if (Guru2Receipt_GetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B | PAD_KEY_UP | PAD_KEY_DOWN)) {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
            Guru2Receipt_SeqEnd(wk);
            return procSeq;
        }
    } else {
        if (wk->g2c->leavingBits != 0) {
            if (Guru2Receipt_GetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B | PAD_KEY_UP | PAD_KEY_DOWN)) {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
            Guru2Receipt_SeqEnd(wk);
            return procSeq;
        }
    }
    if (Guru2Receipt_CountMembers2(wk) != UnionApp_CountJoined(Guru2Receipt_GetUnionApp(wk))) {
        Guru2Receipt_SeqEnd(wk);
        return procSeq;
    }
    result = ConfirmDialog_Update(wk->yesNo);
    if (result != BMPMENU_NULL) {
        if (result == BMPMENU_CANCEL) {
            if (func_0203ffc4() == 0) {
                UnionApp_OpenEntry(Guru2Receipt_GetUnionApp(wk));
            }
            wk->seq = GURU2_RECEIPT_SEQ_INIT;
        } else {
            if (func_0203ffc4() == 0) {
                Guru2Receipt_SetMsgWait(wk, GURU2_RECEIPT_SEQ_PARENT_CANCEL_YESNO_INIT);
                Guru2Receipt_PrintMessage(wk, BtlPokeparam_Text_IfLeaderDropsOut, 0);
            } else {
                Guru2CommSignal signal;

                sys_memset(&signal, 0, sizeof(signal));
                signal.type = GURU2_SIGNAL_LEAVE_REQUEST;
                signal.netId = func_0203ffc4();
                if (Guru2Comm_Send(wk->g2c, GURU2_NET_CMD_SIGNAL, &signal, sizeof(signal))) {
                    wk->cancelSent = TRUE;
                    wk->signalTimer = 0;
                    wk->seq = GURU2_RECEIPT_SEQ_CANCEL_SIGNAL_WAIT;
                } else {
                    wk->seq = GURU2_RECEIPT_SEQ_INIT;
                }
            }
        }
        wk->yesNo = NULL;
    }
    Guru2Receipt_SeqEnd(wk);
    return procSeq;
}

static int Guru2ReceiptSeq_CancelSignalWait(Guru2ReceiptWork *wk, int procSeq) {
    wk->signalTimer = 0;
    Guru2Receipt_SeqEnd(wk);
    return procSeq;
}

static int Guru2ReceiptSeq_CancelOk(Guru2ReceiptWork *wk, int procSeq) {
    if (wk->g2c->signalCount != UnionApp_CountJoined(Guru2Receipt_GetUnionApp(wk))
        || wk->g2c->signalCount != Guru2Receipt_CountMembers2(wk)) {
        wk->signalTimer = 0;
        wk->seq = GURU2_RECEIPT_SEQ_CANCEL_REFUSED;
        Guru2Receipt_SeqEnd(wk);
        return procSeq;
    }
    wk->signalTimer++;
    if (wk->signalTimer > 30) {
        Guru2CommSignal signal;

        sys_memset(&signal, 0, sizeof(signal));
        signal.type = GURU2_SIGNAL_LEAVE;
        signal.netId = func_0203ffc4();
        Guru2Comm_Send(wk->g2c, GURU2_NET_CMD_SIGNAL, &signal, sizeof(signal));
        wk->signalTimer = 0;
        wk->seq = GURU2_RECEIPT_SEQ_END_MSG;
    }
    Guru2Receipt_SeqEnd(wk);
    return procSeq;
}

static int Guru2ReceiptSeq_CancelRefused(Guru2ReceiptWork *wk, int procSeq) {
    wk->cancelSent = FALSE;
    Guru2Receipt_SetMsgWait(wk, GURU2_RECEIPT_SEQ_INIT);
    Guru2Receipt_SeqEnd(wk);
    return procSeq;
}

static int Guru2ReceiptSeq_StartYesNoInit(Guru2ReceiptWork *wk, int procSeq) {
    wk->yesNo = ShopUI_CreateConfirmDialog(&sYesNoSetup, 31, 11, 0, HEAPID_GURU2);
    wk->seq = GURU2_RECEIPT_SEQ_START_YESNO;
    Guru2Receipt_SeqEnd(wk);
    return procSeq;
}

static int Guru2ReceiptSeq_StartYesNo(Guru2ReceiptWork *wk, int procSeq) {
    Guru2CommWork *g2c = wk->g2c;
    u32 result;

    if (g2c->joinCount != Guru2Receipt_CountMembers2(wk) || g2c->leavingBits != 0) {
        if (Guru2Receipt_GetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B | PAD_KEY_UP | PAD_KEY_DOWN)) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
        Guru2Receipt_SeqEnd(wk);
        return procSeq;
    }
    result = ConfirmDialog_Update(wk->yesNo);
    if (result != BMPMENU_NULL) {
        if (result == BMPMENU_CANCEL) {
            UnionApp_OpenEntry(Guru2Receipt_GetUnionApp(wk));
            wk->seq = GURU2_RECEIPT_SEQ_INIT;
        } else if (func_0203ffc4() == 0) {
            wk->seq = GURU2_RECEIPT_SEQ_START_SEND;
            wk->startCount = Guru2Receipt_CountMembers2(wk);
        }
        wk->yesNo = NULL;
    }
    Guru2Receipt_SeqEnd(wk);
    return procSeq;
}

static int Guru2ReceiptSeq_StartSend(Guru2ReceiptWork *wk, int procSeq) {
    if (wk->unk4813 == 0) {
        if (Guru2Comm_Send(wk->g2c, GURU2_NET_CMD_START, NULL, 0) == TRUE) {
            wk->seq = GURU2_RECEIPT_SEQ_START;
            func_02042e9c(TRUE);
        }
    }
    return procSeq;
}

static int Guru2ReceiptSeq_StartWait(Guru2ReceiptWork *wk, int procSeq) {
    if (func_0203ffc4() == 0) {
        UnionApp_CountJoined(Guru2Receipt_GetUnionApp(wk));
    }
    if (wk->g2c->startCount == UnionApp_CountJoined(Guru2Receipt_GetUnionApp(wk))) {
        wk->seq = GURU2_RECEIPT_SEQ_START;
    }
    return procSeq;
}

static int Guru2ReceiptSeq_EndMsg(Guru2ReceiptWork *wk, int procSeq) {
    Guru2Receipt_PrintMessage(wk, BtlPokeparam_Text_SpinTradeWillCanceled, 0);
    Guru2Receipt_SetMsgWait(wk, GURU2_RECEIPT_SEQ_END_WAIT);
    Guru2Receipt_SeqEnd(wk);
    return procSeq;
}

static int Guru2ReceiptSeq_EndWait(Guru2ReceiptWork *wk, int procSeq) {
    wk->wait++;
    if (wk->wait > 60) {
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 16, 1,
                    HEAPID_GURU2);
        procSeq = GURU2_RECEIPT_PROC_FADE_OUT;
    }
    Guru2Receipt_SeqEnd(wk);
    return procSeq;
}

static int Guru2ReceiptSeq_ParentCancelYesNoInit(Guru2ReceiptWork *wk, int procSeq) {
    wk->yesNo = ShopUI_CreateConfirmDialog(&sYesNoSetup, 31, 11, 0, HEAPID_GURU2);
    wk->seq = GURU2_RECEIPT_SEQ_PARENT_CANCEL_YESNO;
    Guru2Receipt_SeqEnd(wk);
    return procSeq;
}

static int Guru2ReceiptSeq_ParentCancelYesNo(Guru2ReceiptWork *wk, int procSeq) {
    Guru2CommWork *g2c = wk->g2c;
    u32 result;

    if (g2c->joinCount != Guru2Receipt_CountMembers2(wk) || g2c->leavingBits != 0) {
        if (Guru2Receipt_GetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B | PAD_KEY_UP | PAD_KEY_DOWN)) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
        Guru2Receipt_SeqEnd(wk);
        return procSeq;
    }
    result = ConfirmDialog_Update(wk->yesNo);
    if (result != BMPMENU_NULL) {
        if (result == BMPMENU_CANCEL) {
            wk->seq = GURU2_RECEIPT_SEQ_INIT;
            UnionApp_OpenEntry(Guru2Receipt_GetUnionApp(wk));
        } else {
            wk->seq = GURU2_RECEIPT_SEQ_PARENT_CANCEL_MSG;
            Guru2Comm_Send(wk->g2c, GURU2_NET_CMD_CANCEL, NULL, 0);
            copyVarForText(wk->wordSet, 0, &UnionApp_GetMember(Guru2Receipt_GetUnionApp(wk), 0)->info);
        }
        wk->yesNo = NULL;
    }
    Guru2Receipt_SeqEnd(wk);
    return procSeq;
}

static int Guru2ReceiptSeq_ParentCancelMsg(Guru2ReceiptWork *wk, int procSeq) {
    if (func_0203ffc4() == 0) {
        Guru2Receipt_PrintMessage(wk, BtlPokeparam_Text_SpinTradeWillCanceled, 0);
    } else {
        copyVarForText(wk->wordSet, 0, &UnionApp_GetMember(Guru2Receipt_GetUnionApp(wk), 0)->info);
        Guru2Receipt_PrintMessage(wk, BtlPokeparam_Text_LeaderDroppedOutGroup, 0);
    }
    Guru2Receipt_SetMsgWait(wk, GURU2_RECEIPT_SEQ_PARENT_CANCEL_WAIT);
    wk->wait = 0;
    Guru2Receipt_SeqEnd(wk);
    return procSeq;
}

static int Guru2ReceiptSeq_ParentCancelWait(Guru2ReceiptWork *wk, int procSeq) {
    wk->wait++;
    if (wk->wait > 45) {
        wk->seq = GURU2_RECEIPT_SEQ_PARENT_CANCEL_SYNC;
    }
    Guru2Receipt_SeqEnd(wk);
    return procSeq;
}

static int Guru2ReceiptSeq_ParentCancelSync(Guru2ReceiptWork *wk, int procSeq) {
    func_02040440();
    wk->syncBits = wk->unk480C ^ Guru2Receipt_GetJoined(wk);
    func_02040b38(func_02040440(), 0xcf, 40, (u16)wk->syncBits);
    wk->seq = GURU2_RECEIPT_SEQ_PARENT_CANCEL_SYNC_WAIT;
    Guru2Receipt_SeqEnd(wk);
    return procSeq;
}

static int Guru2ReceiptSeq_ParentCancelSyncWait(Guru2ReceiptWork *wk, int procSeq) {
    if (func_02040664(func_02040440(), 0xcf, 40)
        || wk->syncBits != (Guru2Receipt_GetJoined(wk) ^ wk->unk480C)) {
        func_02042e94(FALSE);
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 16, 1,
                    HEAPID_GURU2);
        procSeq = GURU2_RECEIPT_PROC_FADE_OUT;
    }
    Guru2Receipt_SeqEnd(wk);
    return procSeq;
}

static int Guru2ReceiptSeq_NoMembersMsgWait(Guru2ReceiptWork *wk, int procSeq) {
    if (Guru2Receipt_IsMessageDone(wk)) {
        Guru2Receipt_PrintMessage(wk, BtlPokeparam_Text_ThereNoMembersSpin, 0);
        wk->seq = GURU2_RECEIPT_SEQ_NO_MEMBERS_END;
    }
    Guru2Receipt_SeqEnd(wk);
    return procSeq;
}

static int Guru2ReceiptSeq_NoMembersEnd(Guru2ReceiptWork *wk, int procSeq) {
    if (Guru2Receipt_IsMessageDone(wk)) {
        wk->seq = GURU2_RECEIPT_SEQ_END_WAIT;
    }
    Guru2Receipt_SeqEnd(wk);
    return procSeq;
}

static int Guru2ReceiptSeq_MemberLeft(Guru2ReceiptWork *wk, int procSeq) {
    Guru2Receipt_PrintMessage(wk, BtlPokeparam_Text_HasLeft, 1);
    wk->seq = GURU2_RECEIPT_SEQ_MEMBER_LEFT_2;
    func_0203ffc4();
    wk->leftCount = UnionApp_CountJoined(Guru2Receipt_GetUnionApp(wk));
    wk->leftTimer = 300;
    Guru2Receipt_SeqEnd(wk);
    return procSeq;
}

static int Guru2ReceiptSeq_MemberLeft2(Guru2ReceiptWork *wk, int procSeq) {
    if (wk->leftCount != 0 && wk->leftCount != UnionApp_CountJoined(Guru2Receipt_GetUnionApp(wk))) {
        wk->leftCount = 0;
    }
    wk->seq = GURU2_RECEIPT_SEQ_MEMBER_LEFT_WAIT;
    wk->wait = 0;
    Guru2Receipt_SeqEnd(wk);
    return procSeq;
}

static int Guru2ReceiptSeq_MemberLeftWait(Guru2ReceiptWork *wk, int procSeq) {
    if (wk->leftCount != 0 && wk->leftCount != UnionApp_CountJoined(Guru2Receipt_GetUnionApp(wk))) {
        wk->leftCount = 0;
    }
    wk->leftTimer--;
    if (wk->leftTimer < 0) {
        wk->leftCount = 0;
    }
    wk->wait++;
    if (wk->wait > 60 && wk->leftCount == 0) {
        wk->seq = GURU2_RECEIPT_SEQ_INIT;
        func_0203ffc4();
    }
    Guru2Receipt_SeqEnd(wk);
    return procSeq;
}

static int Guru2ReceiptSeq_Start(Guru2ReceiptWork *wk, int procSeq) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 16, 1, HEAPID_GURU2);
    wk->receiptDone = TRUE;
    Guru2Receipt_SeqEnd(wk);
    return GURU2_RECEIPT_PROC_FADE_OUT;
}

void Guru2Receipt_SetSeq(Guru2ReceiptWork *wk, int seq, u8 netId) {
    switch (seq) {
    case GURU2_RECEIPT_SEQ_2:
        Guru2Receipt_ClearMessage(wk);
        break;
    case GURU2_RECEIPT_SEQ_PARENT_CANCEL_MSG:
        if (wk->yesNo != NULL) {
            ConfirmDialog_Free(wk->yesNo);
            wk->yesNo = NULL;
        }
        break;
    case GURU2_RECEIPT_SEQ_START_WAIT:
        Guru2Receipt_PrintMessage(wk, BtlPokeparam_Text_Empty_2, 0);
        if (wk->yesNo != NULL) {
            ConfirmDialog_Free(wk->yesNo);
            wk->yesNo = NULL;
        }
        break;
    case GURU2_RECEIPT_SEQ_MEMBER_LEFT:
        if (wk->cancelSent == TRUE) {
            return;
        }
        copyVarForText(wk->wordSet, 0, &UnionApp_GetMember(Guru2Receipt_GetUnionApp(wk), netId)->info);
        if (netId == func_0203ffc4()) {
            return;
        }
        if (wk->yesNo != NULL) {
            ConfirmDialog_Free(wk->yesNo);
            wk->yesNo = NULL;
        }
        if (func_0203ffc4() == 0) {
            wk->g2c->leavingBits &= 0xffff ^ netId;
        }
        break;
    case GURU2_RECEIPT_SEQ_START:
        if (!Guru2Receipt_IsMessageDone(wk)) {
            func_02022390(wk->stream);
        }
        if (wk->yesNo != NULL) {
            ConfirmDialog_Free(wk->yesNo);
            wk->yesNo = NULL;
        }
        break;
    case GURU2_RECEIPT_SEQ_CANCEL_OK:
    case GURU2_RECEIPT_SEQ_CANCEL_REFUSED:
        break;
    default:
        return;
    }
    wk->seq = seq;
}

void Guru2Receipt_SetSeqOnJoin(Guru2ReceiptWork *wk, int seq, u8 netId) {
    if (wk->seq == GURU2_RECEIPT_SEQ_MAIN) {
        switch (seq) {
        case GURU2_RECEIPT_SEQ_1: {
            UnionAppMember *member = UnionApp_GetMember(Guru2Receipt_GetUnionApp(wk), netId);

            if (member != NULL) {
                copyVarForText(wk->wordSet, 0, &member->info);
            }
            wk->seq = seq;
            wk->g2c->joinCount = UnionApp_CountJoined(Guru2Receipt_GetUnionApp(wk));
            wk->g2c->leavingBits = 0;
            break;
        }
        case GURU2_RECEIPT_SEQ_MEMBER_LEFT:
            copyVarForText(wk->wordSet, 0, &UnionApp_GetMember(Guru2Receipt_GetUnionApp(wk), netId)->info);
            if (netId == func_0203ffc4()) {
                return;
            }
            if (func_0203ffc4() == 0) {
                wk->g2c->leavingBits &= 0xffff ^ netId;
            }
            wk->seq = seq;
            break;
        }
    } else if (seq == GURU2_RECEIPT_SEQ_1) {
        wk->g2c->leavingBits = 0;
        wk->g2c->joinCount = UnionApp_CountJoined(Guru2Receipt_GetUnionApp(wk));
    }
}

static int Guru2Receipt_CountMembers(Guru2ReceiptWork *wk) {
    int i;
    int count = 0;

    for (i = 0; i < GURU2_MEMBER_MAX; i++) {
        if (UnionApp_GetMember(Guru2Receipt_GetUnionApp(wk), i) != NULL) {
            count++;
        }
    }
    return count;
}

// Draws the members' names and IDs again if the members changed, and returns whether they did
static BOOL Guru2Receipt_DrawNames(BmpWin *window, int unused, u16 color, Guru2ReceiptWork *wk) {
    int self = func_0203ffc4();
    int i;

    if (!Guru2Receipt_UpdateMembers(wk)) {
        return FALSE;
    }
    GFL_BitmapFill(BmpWin_GetBitmap(window), 0);
    for (i = 0; i < GURU2_MEMBER_MAX; i++) {
        if (wk->members[i].member != NULL) {
            u16 id = getTrainerID(&wk->members[i].member->info);
            StrBuf *idBuf = GFL_StrBufCreate(16, HEAPID_GURU2);
            StrBuf *format;
            u16 nameColor;

            textCopy(wk->members[i].member->info.name, wk->names[i]);
            WordSetNumber(wk->wordSet, 0, id, 5, 2, TRUE);
            format = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlPokeparam_Text_Id);
            GFL_WordSetFormatStrbuf(wk->wordSet, idBuf, format);
            if (self == i) {
                nameColor = GURU2_RECEIPT_COLOR_SELF;
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

static void Guru2Receipt_Dummy(Guru2ReceiptWork *wk) {
}

static int Guru2Receipt_CountMembers2(Guru2ReceiptWork *wk) {
    int i;
    int count = 0;

    for (i = 0; i < GURU2_MEMBER_MAX; i++) {
        if (UnionApp_GetMember(Guru2Receipt_GetUnionApp(wk), i) != NULL) {
            count++;
        }
    }
    return count;
}

static u32 Guru2Receipt_GetJoined(Guru2ReceiptWork *wk) {
    return UnionApp_GetJoined(Guru2Receipt_GetUnionApp(wk));
}

// Reads the members, sets what the trainer of each one that changed does, and returns whether any changed
static BOOL Guru2Receipt_UpdateMembers(Guru2ReceiptWork *wk) {
    BOOL changed = FALSE;
    int i;

    for (i = 0; i < GURU2_MEMBER_MAX; i++) {
        wk->members[i].prev = wk->members[i].member;
        wk->members[i].member = UnionApp_GetMember(Guru2Receipt_GetUnionApp(wk), i);
        wk->ids[i].prev.parts = wk->ids[i].id.parts;
        if (wk->members[i].member != NULL) {
            wk->ids[i].id.parts.id = getIDAsUInt(&wk->members[i].member->info);
            wk->ids[i].id.parts.present = TRUE;
        } else {
            wk->ids[i].id.value = 0;
        }
    }
    for (i = 0; i < GURU2_MEMBER_MAX; i++) {
        if (wk->members[i].prev != wk->members[i].member) {
            changed = TRUE;
            if (wk->members[i].member == NULL) {
                wk->actorStates[i] = GURU2_RECEIPT_ACTOR_LEAVE;
            } else {
                wk->actorStates[i] = GURU2_RECEIPT_ACTOR_APPEAR;
            }
        } else if (wk->ids[i].id.value != wk->ids[i].prev.value) {
            changed = TRUE;
            if (!wk->ids[i].id.parts.present) {
                wk->actorStates[i] = GURU2_RECEIPT_ACTOR_LEAVE;
            } else {
                wk->actorStates[i] = GURU2_RECEIPT_ACTOR_APPEAR;
            }
        }
    }
    return changed;
}

static void Guru2Receipt_PrintMessage(Guru2ReceiptWork *wk, u32 messageId, int unused) {
    StrBuf *format = GFL_StrBufCreate(180, HEAPID_GURU2);

    GFL_MsgDataLoadStrbuf(wk->msgData, messageId, format);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->msgBuf, format);
    GFL_StrBufFree(format);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->msgWindow), 15);
    BmpWin_DrawFrame(wk->msgWindow, FALSE, 1, 10);
    if (wk->stream != NULL) {
        func_020223cc(wk->stream);
    }
    wk->stream = func_02022268(wk->msgWindow, 0, 0, wk->msgBuf, wk->font, 0, wk->tcbManager, 0,
                               HEAPID_GURU2, 15);
    BmpWin_TransferNow(wk->msgWindow);
}

static BOOL Guru2Receipt_IsMessageDone(Guru2ReceiptWork *wk) {
    u32 state;

    if (wk->stream == NULL) {
        return TRUE;
    }
    state = func_020223b4(wk->stream);
    if (state == PRINT_STREAM_RUNNING) {
        if (GCTX_HIDGetHeldKeys() & PAD_BUTTON_A) {
            func_020223e0(wk->stream, 0);
        }
    } else if (state == PRINT_STREAM_PAUSED) {
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            func_020223bc(wk->stream);
        }
    } else if (state == PRINT_STREAM_DONE) {
        func_020223cc(wk->stream);
        wk->stream = NULL;
        return TRUE;
    }
    return FALSE;
}

static void Guru2Receipt_ClearMessage(Guru2ReceiptWork *wk) {
    BmpWin_ClearFrame(wk->msgWindow, FALSE);
}

static void Guru2Receipt_UpdateActors(Guru2ReceiptWork *wk) {
    int i;
    BOOL appeared = FALSE;

    for (i = 0; i < GURU2_MEMBER_MAX; i++) {
        switch (wk->actorStates[i]) {
        case GURU2_RECEIPT_ACTOR_APPEAR: {
            int gender = getTrainerGender(&wk->members[i].member->info);
            int look = func_02008bf4(&wk->members[i].member->info);

            if (i == func_0203ffc4()) {
                func_0204c488(wk->actors[i], 4);
            } else {
                Guru2Receipt_SetActorLook(wk, wk->trainerPalette, i, look, gender);
                func_0204c488(wk->actors[i], 4);
            }
            func_0204c124(wk->actors[i], TRUE);
            wk->actorStates[i] = GURU2_RECEIPT_ACTOR_SHOWN;
            appeared = TRUE;
            break;
        }
        case GURU2_RECEIPT_ACTOR_LEAVE:
            if (i == func_0203ffc4()) {
                getTrainerGender(&wk->members[i].member->info);
                func_0204c488(wk->actors[i], 5);
            } else {
                func_0204c488(wk->actors[i], 5);
            }
            wk->actorStates[i] = GURU2_RECEIPT_ACTOR_NONE;
            break;
        case GURU2_RECEIPT_ACTOR_NONE:
        case GURU2_RECEIPT_ACTOR_SHOWN:
            break;
        }
    }
    if (appeared) {
        GFL_SndSEPlay(SEQ_SE_FLD_05);
    }
}

static void Guru2Receipt_LoadTrainerPalette(Guru2ReceiptWork *wk, ArcTool *arc) {
    wk->trainerPaletteFile =
        GFL_G2DIOReadNCLR(GURU2_RECEIPT_TRAINER_ARCID, 0, &wk->trainerPalette, HEAPID_GURU2);
}

static void Guru2Receipt_FreeTrainerPalette(Guru2ReceiptWork *wk) {
    GFL_HeapFree(wk->trainerPaletteFile);
}

// The row of the trainer palette file for each appearance (func_02008bf4)
static const u8 sLookPalettes[16] = { 2, 3, 6, 5, 4, 5, 2, 0, 1, 3, 6, 5, 4, 7, 7, 0 };

// Gives a member's actor the palette and characters of the member's appearance
static void Guru2Receipt_SetActorLook(Guru2ReceiptWork *wk, NNSG2dPaletteData *palette, int id, int look, int gender) {
    NNSG2dCharacterData *character;
    void *file;

    gfxUploadStdPaletteObjA((u8 *)palette->rawData + sLookPalettes[look] * 0x20, id * 0x20, 0x20);
    file = GFL_G2DIOReadOBJNCGR(GURU2_RECEIPT_TRAINER_ARCID, look + 1, FALSE, &character, HEAPID_GURU2);
    func_0204ba40(wk->resources[GURU2_RECEIPT_RES_CHAR + id], character);
    GFL_HeapFree(file);
}

// The parent's check of the members each frame, which returns the proc's next sequence
static int Guru2Receipt_CheckMembers(Guru2ReceiptWork *wk, int unused) {
    int count = Guru2Receipt_CountMembers2(wk);

    if (count > wk->g2c->joinCount) {
        if (UnionApp_CloseEntry(Guru2Receipt_GetUnionApp(wk))) {
            wk->entryClosed = TRUE;
        }
    } else {
        wk->entryClosed = FALSE;
    }
    if (count == wk->memberCount) {
        return GURU2_RECEIPT_PROC_MAIN;
    }
    switch (count) {
    case 1:
        if (UnionApp_CountJoined(Guru2Receipt_GetUnionApp(wk)) > 1
            || UnionApp_GetJoined(Guru2Receipt_GetUnionApp(wk)) > 1) {
            return GURU2_RECEIPT_PROC_MAIN;
        }
        wk->seq = GURU2_RECEIPT_SEQ_NO_MEMBERS_MSG_WAIT;
        if (wk->yesNo != NULL) {
            ConfirmDialog_Free(wk->yesNo);
            wk->yesNo = NULL;
        }
        return GURU2_RECEIPT_PROC_MAIN_NO_UPDATE;
    case 2:
    case 3:
    case 4:
        if (count < wk->memberCount) {
            wk->g2c->joinCount = UnionApp_CountJoined(Guru2Receipt_GetUnionApp(wk));
        }
        break;
    case 0:
    case 5:
        break;
    }
    wk->memberCount = Guru2Receipt_CountMembers2(wk);
    return GURU2_RECEIPT_PROC_MAIN;
}

static u32 Guru2Receipt_GetPressedKeys(void) {
    return GCTX_HIDGetPressedKeys();
}
