#include "types.h"
#include "app/oekaki.h"
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
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/math_util.h"
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
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/app_menu_common.h"
#include "system/app_taskmenu.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/event_mapchange.h"

// oekaki.c, which the ROM names: the drawing board of the Union Room. The parent collects what every member draws
// in a frame and sends it back to all of them, so that each machine draws the same lines; a newcomer gets the
// picture so far in chunks. Names are ours

// The screen's graphics
#define OEKAKI_ARCID 188

#define OEKAKI_COLOR_TEXT PRINT_COLOR(1, 2, 15)
#define OEKAKI_COLOR_NAME_MINE PRINT_COLOR(3, 4, 0)

// Where the top screen's actors are. The SDK's FX32_ONE is a long, which keeps the sum it is in from folding
#define OEKAKI_SUB_SURFACE_Y (256 * 0x1000L)

// The brushes are 6 by 6 pixels, in 8 pixel wide rows
#define OEKAKI_BRUSH_SIZE 6
#define OEKAKI_BRUSH_COUNT 3
#define OEKAKI_COLOR_COUNT 8

typedef struct {
    int (*func)(OekakiWork *wk, int seq);
    // Whether the sequence still runs in OEKAKI_PROC_SEQ_END
    BOOL runsAtEnd;
} OekakiSeq;

typedef struct {
    u16 x;
    u16 y;
    u16 sequence;
} OekakiButtonData;

typedef struct {
    u32 messageId;
    u32 type;
} OekakiMenuItem;

static BOOL Oekaki_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL Oekaki_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL Oekaki_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void Oekaki_VBlank(TCB *tcb, void *data);
static void Oekaki_OnJoin(u8 netId, UnionAppMember *member, void *work);
static void Oekaki_OnLeave(u8 netId, UnionAppMember *member, void *work);
static void Oekaki_SetVRAMBanks(void);
static void Oekaki_InitBG(void);
static void Oekaki_InitWork(OekakiWork *wk);
static void Oekaki_FreeWork(OekakiWork *wk);
static void Oekaki_FreeBG(void);
static void Oekaki_LoadGraphics(OekakiWork *wk, ArcTool *arc);
static void Oekaki_LoadActorResources(OekakiWork *wk, ArcTool *arc);
static void Oekaki_CreateActors(OekakiWork *wk);
static void Oekaki_CreateWindows(OekakiWork *wk, GameProc *proc);
static void Oekaki_FreeWindows(OekakiWork *wk);
static void Oekaki_SetCursorPos(ClActor *actor, int x, int y);
static void Oekaki_MakeEmptyTouch(OekakiTouchInfo *info, int color, int brush);
static void Oekaki_UpdateTouch(OekakiWork *wk);
static void Oekaki_SetColorButtons(ClActor **buttons, int color);
static void Oekaki_SetEndButton(ClActor *actor, BOOL pressed);
static int Oekaki_SeqMain(OekakiWork *wk, int seq);
static void Oekaki_DrawOthers(OekakiWork *wk);
static int Oekaki_SeqNewcomer(OekakiWork *wk, int seq);
static int Oekaki_SeqNewcomerWait(OekakiWork *wk, int seq);
static int Oekaki_SeqNewcomerEnd(OekakiWork *wk, int seq);
static int Oekaki_SeqEndSelectMsgWait(OekakiWork *wk, int seq);
static int Oekaki_SeqEndSelect(OekakiWork *wk, int seq);
static int Oekaki_SeqEndRequest(OekakiWork *wk, int seq);
static int Oekaki_SeqEndAnswerWait(OekakiWork *wk, int seq);
static int Oekaki_SeqEndOk(OekakiWork *wk, int seq);
static int Oekaki_SeqEndLeave(OekakiWork *wk, int seq);
static int Oekaki_SeqEndRefused(OekakiWork *wk, int seq);
static int Oekaki_SeqEndMsg(OekakiWork *wk, int seq);
static int Oekaki_SeqEndMsgWait(OekakiWork *wk, int seq);
static int Oekaki_SeqEndWait(OekakiWork *wk, int seq);
static int Oekaki_SeqParentEndSelectMsgWait(OekakiWork *wk, int seq);
static int Oekaki_SeqParentEndSelect(OekakiWork *wk, int seq);
static void Oekaki_SetPlayerName(OekakiWork *wk, int index, int netId);
static int Oekaki_SeqParentEndSend(OekakiWork *wk, int seq);
static int Oekaki_SeqParentEndMsg(OekakiWork *wk, int seq);
static int Oekaki_SeqParentEndMsgWait(OekakiWork *wk, int seq);
static int Oekaki_SeqParentEndSyncWait(OekakiWork *wk, int seq);
static int Oekaki_Seq21(OekakiWork *wk, int seq);
static int Oekaki_SeqNoMembersMsg(OekakiWork *wk, int seq);
static int Oekaki_SeqNoMembersMsgWait(OekakiWork *wk, int seq);
static int Oekaki_SeqMemberLeft(OekakiWork *wk, int seq);
static int Oekaki_SeqMemberLeftMsgWait(OekakiWork *wk, int seq);
static int Oekaki_SeqMemberLeftWait(OekakiWork *wk, int seq);
static void Oekaki_DrawBrush(BmpWin *window, const u8 *brush, int srcX, int srcY, int width, int height, int x, int y,
                             int brushWidth, int brushHeight);
static void Oekaki_DrawLine(BmpWin *window, const u8 *brush, int x, int y, int *oldX, int *oldY, int penDown,
                            int oldSize);
static void Oekaki_SaveOldTouch(OekakiTouchInfo *touch, OekakiOldTouch *old);
static void Oekaki_DrawTouches(BmpWin *window, OekakiTouchInfo *touch, OekakiOldTouch *old, BOOL transfer);
static void Oekaki_UpdateCursors(OekakiWork *wk);
static void Oekaki_UpdatePalette(u16 *angle);
static int Oekaki_CountMembers(OekakiWork *wk);
static void Oekaki_PrintNames(BmpWin **windows, u16 color, OekakiWork *wk);
static void Oekaki_SendTouch(OekakiWork *wk);
static int Oekaki_GetMemberCount(OekakiWork *wk);
static BOOL Oekaki_UpdateMembers(OekakiWork *wk);
static void Oekaki_PrintMessage(OekakiWork *wk, int messageId, BOOL stream);
static BOOL Oekaki_IsMessageDone(OekakiWork *wk);
static void Oekaki_ClearMessage(OekakiWork *wk);
static void Oekaki_SetEntry(OekakiWork *wk, u8 limit);
static int Oekaki_UpdateParent(OekakiWork *wk);
static void Oekaki_SetNextSeq(OekakiWork *wk, int seq);
static void Oekaki_UpdateSeq(OekakiWork *wk, int procSeq);
static void Oekaki_CloseMenu(OekakiWork *wk);
static BOOL Oekaki_IsMenuAreaTouched(OekakiWork *wk);
static void Oekaki_MakeTouchInfo(OekakiTouchInfo *info, OekakiTouchData *touchData, int color, int brush);
static int Oekaki_CountJoined(OekakiWork *wk);
static u32 Oekaki_GetJoined(OekakiWork *wk);
static void Oekaki_ShowScreens(void);
static AppTaskMenu *Oekaki_CreateMenu(OekakiWork *wk);
static int Oekaki_UpdateMenu(OekakiWork *wk);
static void Oekaki_FreeMenu(OekakiWork *wk);
static void Oekaki_RecordMetTrainers(OekakiWork *wk);
static void Oekaki_FlushPrint(OekakiWork *wk);
static void Oekaki_SendRequest(OekakiWork *wk);
static int Oekaki_QueuePrev(OekakiQueue *queue);
static void Oekaki_QueueAdvanceChild(OekakiQueue *queue);
static void Oekaki_QueueAdvanceParent(OekakiQueue *queue);
static void Oekaki_ClearTouchSizes(OekakiTouchInfo *touch);
static void Oekaki_QueueNext(OekakiQueue *queue);
static void Oekaki_QueuePop(OekakiTouchInfo *touch, OekakiQueue *queue);
static void Oekaki_QueuePopAll(OekakiTouchInfo *touch, OekakiQueue *queue);

// The sequences, by OEKAKI_SEQ_*
static OekakiSeq sSeqTable[OEKAKI_SEQ_COUNT] = {
    { NULL, TRUE },
    { Oekaki_SeqNewcomer, FALSE },
    { Oekaki_SeqNewcomerWait, FALSE },
    { Oekaki_SeqNewcomerEnd, FALSE },
    { Oekaki_SeqMain, FALSE },
    { Oekaki_SeqEndSelectMsgWait, TRUE },
    { Oekaki_SeqEndSelect, TRUE },
    { Oekaki_SeqEndAnswerWait, TRUE },
    { Oekaki_SeqEndRequest, TRUE },
    { Oekaki_SeqEndOk, TRUE },
    { Oekaki_SeqEndLeave, TRUE },
    { Oekaki_SeqEndRefused, TRUE },
    { Oekaki_SeqEndMsg, TRUE },
    { Oekaki_SeqEndMsgWait, TRUE },
    { Oekaki_SeqEndWait, TRUE },
    { Oekaki_SeqParentEndSelectMsgWait, TRUE },
    { Oekaki_SeqParentEndSelect, TRUE },
    { Oekaki_SeqParentEndSend, TRUE },
    { Oekaki_SeqParentEndMsg, TRUE },
    { Oekaki_SeqParentEndMsgWait, TRUE },
    { Oekaki_SeqParentEndSyncWait, TRUE },
    { Oekaki_Seq21, TRUE },
    { Oekaki_SeqNoMembersMsg, TRUE },
    { Oekaki_SeqNoMembersMsgWait, TRUE },
    { Oekaki_SeqMemberLeft, TRUE },
    { Oekaki_SeqMemberLeftMsgWait, TRUE },
    { Oekaki_SeqMemberLeftWait, TRUE },
};

const GameProcFunctions OEKAKI_PROC_FUNCTIONS = { Oekaki_Init, Oekaki_Main, Oekaki_Exit };

static BOOL Oekaki_Init(GameProc *proc, u32 *state, void *param, void *work) {
    OekakiWork *wk;
    ArcTool *arc;

    switch (*state) {
    case 0:
        GFL_BGSysDisableAllA();
        GFL_BGSysDisableAllB();
        GFL_HeapCreateChild(HEAPID_USER, HEAPID_OEKAKI, 0x50000);
        wk = GFL_ProcInitSubsystem(proc, sizeof(OekakiWork), HEAPID_OEKAKI);
        sys_memset(wk, 0, sizeof(OekakiWork));
        GFL_BGSysCreate(HEAPID_OEKAKI);
        wk->wordSet = GFL_WordSetSystemCreateDefault(HEAPID_OEKAKI);
        wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVENT_MAPCHANGE, HEAPID_OEKAKI);
        wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_OEKAKI);
        Oekaki_SetVRAMBanks();
        Oekaki_InitBG();
        GFL_WipeSet(WIPE_MODE_BOTH, 9, 9, WIPE_COLOR_BLACK, 16, 1, HEAPID_OEKAKI);
        wk->param = param;
        arc = GFL_ArcSysCreateFileHandle(OEKAKI_ARCID, HEAPID_OEKAKI);
        Oekaki_LoadGraphics(wk, arc);
        wk->vblankTcb = GFL_VBlankTCBAdd(Oekaki_VBlank, wk, 0);
        Oekaki_InitWork(wk);
        Oekaki_LoadActorResources(wk, arc);
        Oekaki_CreateActors(wk);
        Oekaki_CreateWindows(wk, proc);
        Oekaki_ShowScreens();
        OekakiComm_AddCommands(wk);
        Oekaki_RecordMetTrainers(wk);
        func_02042ba8(TRUE, HEAPID_OEKAKI);
        GFL_ArcToolFree(arc);
        (*state)++;
        break;
    case 1:
        *state = 0;
        return TRUE;
    }
    return FALSE;
}

static BOOL Oekaki_Main(GameProc *proc, u32 *state, void *param, void *work) {
    OekakiWork *wk = work;

    // The parent forgets the leaving children that are gone
    if (func_0203ffc4() == 0 && wk->leavingBits != 0) {
        wk->leavingBits &= Oekaki_GetJoined(wk);
    }
    wk->procSeq = *state;
    Oekaki_UpdateSeq(wk, *state);
    switch (*state) {
    case OEKAKI_PROC_SEQ_FADE_IN:
        if (GFL_WipeIsFinished()) {
            UnionApp_SetCallbacks(wk->param->app, Oekaki_OnJoin, Oekaki_OnLeave, wk);
            if (func_0203ffc4() == 0) {
                OekakiComm_LimitEntry(wk, 1);
            }
            *state = OEKAKI_PROC_SEQ_MAIN;
        }
        break;
    case OEKAKI_PROC_SEQ_MAIN:
        if (sSeqTable[wk->seq].func != NULL) {
            *state = sSeqTable[wk->seq].func(wk, *state);
        }
        Oekaki_PrintNames(wk->nameWindows, OEKAKI_COLOR_TEXT, wk);
        if (func_0203ffc4() == 0) {
            int next = Oekaki_UpdateParent(wk);

            if (*state == OEKAKI_PROC_SEQ_MAIN) {
                *state = next;
            }
        }
        Oekaki_UpdatePalette(&wk->paletteAngle);
        break;
    case OEKAKI_PROC_SEQ_END:
        if (sSeqTable[wk->seq].func != NULL && sSeqTable[wk->seq].runsAtEnd) {
            *state = sSeqTable[wk->seq].func(wk, *state);
        }
        break;
    case OEKAKI_PROC_SEQ_FADE_OUT:
        if (GFL_WipeIsFinished()) {
            func_02040c64(OEKAKI_NET_CMD_BASE);
            UnionApp_RequestStart(wk->param->app);
            *state = OEKAKI_PROC_SEQ_START_WAIT;
        }
        break;
    case OEKAKI_PROC_SEQ_START_WAIT:
        if (UnionApp_IsStarted(wk->param->app)) {
            return TRUE;
        }
        break;
    }
    Oekaki_SendRequest(wk);
    func_0204b794();
    GFL_TCBExMgrUpdate(wk->tcbManager);
    Oekaki_FlushPrint(wk);
    if (GFL_WipeIsFinished() && GFL_NetErrCheck()) {
        return TRUE;
    }
    return FALSE;
}

static BOOL Oekaki_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    OekakiWork *wk = work;

    switch (*state) {
    case 0:
        GFL_TCBRemove(wk->vblankTcb);
        func_0204b98c(wk->resources[OEKAKI_RES_SUB].chars);
        func_0204bcd0(wk->resources[OEKAKI_RES_SUB].palette);
        func_0204be64(wk->resources[OEKAKI_RES_SUB].cellAnims);
        func_0204b98c(wk->resources[OEKAKI_RES_COMMON].chars);
        func_0204bcd0(wk->resources[OEKAKI_RES_COMMON].palette);
        func_0204be64(wk->resources[OEKAKI_RES_COMMON].cellAnims);
        func_0204b98c(wk->resources[OEKAKI_RES_MAIN].chars);
        func_0204bcd0(wk->resources[OEKAKI_RES_MAIN].palette);
        func_0204be64(wk->resources[OEKAKI_RES_MAIN].cellAnims);
        func_0204bf98(wk->actorUnit);
        func_0204b758();
        if (wk->menu != NULL) {
            Oekaki_FreeMenu(wk);
        }
        Oekaki_FreeWindows(wk);
        Oekaki_FreeBG();
        GFL_MsgDataFree(wk->msgData);
        GFL_WordSetSystemFree(wk->wordSet);
        GFL_FontFree(wk->font);
        (*state)++;
        break;
    case 1:
        (*state)++;
        break;
    case 2:
        if (wk->waitAlone) {
            if (Oekaki_GetJoined(wk) == 1) {
                (*state)++;
            }
        } else {
            (*state)++;
        }
        break;
    case 3:
        Oekaki_FreeWork(wk);
        GFL_ProcReleaseSubsystem(proc);
        GFL_HeapDelete(HEAPID_OEKAKI);
        return TRUE;
    }
    return FALSE;
}

static void Oekaki_VBlank(TCB *tcb, void *data) {
    func_0204b7c8();
    GFL_BGSysUpdate();
}

static void Oekaki_OnJoin(u8 netId, UnionAppMember *member, void *work) {
    OekakiWork *wk = work;

    if (func_0203ffc4() == 0) {
        Oekaki_RequestSend(wk, OEKAKI_NET_CMD_NEWCOMER, netId);
    }
    Oekaki_RecordMetTrainers(wk);
}

static void Oekaki_OnLeave(u8 netId, UnionAppMember *member, void *work) {
}

static const BGSysVRAMConfig sOekakiVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static void Oekaki_SetVRAMBanks(void) {
    GFL_BGSysSetVRAMBanks(&sOekakiVRAMConfig);
}

static void Oekaki_InitBG(void) {
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
            GX_BG_SCRBASE(0xf800),
            GX_BG_CHARBASE(0),
            0x8000,
            GX_BG_EXTPLTT_01,
            1,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(1, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(1);
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
            GX_BG_CHARBASE(0x8000),
            0x4000,
            GX_BG_EXTPLTT_01,
            2,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(2, &setup, BGMODE_TEXT);
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
            GX_BG_CHARBASE(0),
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
            GX_BG_SCRBASE(0xf000),
            GX_BG_CHARBASE(0x8000),
            0x4000,
            GX_BG_EXTPLTT_01,
            1,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(5, &setup, BGMODE_TEXT);
    }
    GFL_BGSysClearCharCore(0, 0x20, 0, HEAPID_OEKAKI);
    GFL_BGSysClearCharCore(4, 0x20, 0, HEAPID_OEKAKI);
    GFL_BGSysClearCharCore(1, 0x20, 0, HEAPID_OEKAKI);
    GFL_BGSysClearCharCore(5, 0x20, 0, HEAPID_OEKAKI);
}

static void Oekaki_InitWork(OekakiWork *wk) {
    int i;

    for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
        wk->nameBufs[i] = GFL_StrBufCreate(8, HEAPID_OEKAKI);
        wk->allTouch[i].size = 0;
        wk->oldTouch[i].size = 0;
        wk->members[i].member = NULL;
        wk->members[i].old = NULL;
    }
    wk->endBuf = GFL_StrBufCreate(10, HEAPID_OEKAKI);
    wk->msgBuf = GFL_StrBufCreate(80, HEAPID_OEKAKI);
    wk->color = 0;
    wk->brush = 1;
    wk->banFlag = OEKAKI_BAN_NONE;
    wk->menuOpen = FALSE;
    wk->shareNum = 2;
    wk->shareBit = Oekaki_GetJoined(wk);
    wk->unk9E74 = 0;
    wk->waitAlone = FALSE;
    wk->seq = OEKAKI_SEQ_NONE;
    wk->joining = FALSE;
    wk->joiningBits = 0;
    wk->menu = NULL;
    func_0203ffc4();
    Oekaki_SetNextSeq(wk, OEKAKI_SEQ_MAIN);
    GFL_MsgDataLoadStrbuf(wk->msgData, EventMapchange_Text_Empty, wk->endBuf);
    wk->buffer = GFL_HeapAllocate(HEAPID_OEKAKI, 0x3c00, FALSE, "oekaki.c", 0x31d);
    wk->memberCount = 0;
}

static void Oekaki_FreeWork(OekakiWork *wk) {
    int i;

    GFL_HeapFree(wk->buffer);
    for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
        GFL_StrBufFree(wk->nameBufs[i]);
    }
    GFL_StrBufFree(wk->msgBuf);
    GFL_StrBufFree(wk->endBuf);
}

static void Oekaki_FreeBG(void) {
    GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, -16);
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, -16);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(0);
    GFL_BGSysFree();
}

static void Oekaki_LoadGraphics(OekakiWork *wk, ArcTool *arc) {
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, PALTYPE_MAIN_BG, 0, 12 * 0x20, HEAPID_OEKAKI);
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, PALTYPE_SUB_BG, 0, 12 * 0x20, HEAPID_OEKAKI);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 13 * 0x20, 0x20, HEAPID_OEKAKI);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 13 * 0x20, 0x20, HEAPID_OEKAKI);
    GFL_BGSysLoadArcNCGRStatic(arc, 2, 2, 0, 0x2000, FALSE, HEAPID_OEKAKI);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 6, 2, 0, 0x600, FALSE, HEAPID_OEKAKI);
    GFL_BGSysLoadArcNCGRStatic(arc, 4, 5, 0, 0x2000, FALSE, HEAPID_OEKAKI);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 7, 5, 0, 0x600, FALSE, HEAPID_OEKAKI);
    LoadSysMsgBox(0, 1, 10, 0, HEAPID_OEKAKI);
    LoadSysMsgBox(0, 31, 11, 0, HEAPID_OEKAKI);
}

static void Oekaki_LoadActorResources(OekakiWork *wk, ArcTool *arc) {
    ArcTool *commonArc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_OEKAKI);

    ClActSys_Create(&data_02093f08, &sOekakiVRAMConfig, HEAPID_OEKAKI);
    wk->actorUnit = func_0204bf1c(53, 1, HEAPID_OEKAKI);
    wk->resources[OEKAKI_RES_MAIN].chars = func_0204b81c(arc, 3, FALSE, 0, HEAPID_OEKAKI);
    wk->resources[OEKAKI_RES_MAIN].palette = func_0204bba0(arc, 0, 0, 0, HEAPID_OEKAKI);
    wk->resources[OEKAKI_RES_MAIN].cellAnims = func_0204bde0(arc, 5, 8, HEAPID_OEKAKI);
    wk->resources[OEKAKI_RES_COMMON].chars = func_0204b81c(commonArc, func_0202d814(), FALSE, 0, HEAPID_OEKAKI);
    wk->resources[OEKAKI_RES_COMMON].palette =
        func_0204bbb8(commonArc, func_0202d810(), 0, 11 * 0x20, 0, 3, HEAPID_OEKAKI);
    wk->resources[OEKAKI_RES_COMMON].cellAnims =
        func_0204bde0(commonArc, func_0202d818(0), func_0202d81c(0), HEAPID_OEKAKI);
    wk->resources[OEKAKI_RES_SUB].chars = func_0204b81c(arc, 3, FALSE, 1, HEAPID_OEKAKI);
    wk->resources[OEKAKI_RES_SUB].palette = func_0204bba0(arc, 0, 1, 0, HEAPID_OEKAKI);
    wk->resources[OEKAKI_RES_SUB].cellAnims = func_0204bde0(arc, 5, 8, HEAPID_OEKAKI);
    GFL_ArcToolFree(commonArc);
}

// Where the buttons are, and their animations
static const OekakiButtonData sButtonData[OEKAKI_BUTTON_COUNT] = {
    { 12, 172, 5 },   { 36, 172, 7 },   { 60, 172, 9 },   { 84, 172, 11 }, { 108, 172, 13 }, { 132, 172, 15 },
    { 156, 172, 17 }, { 180, 172, 19 }, { 224, 171, 21 }, { 20, 9, 29 },   { 36, 9, 32 },    { 52, 9, 33 },
};

static void Oekaki_CreateActors(OekakiWork *wk) {
    ClActorSetup setup;
    int i;

    setup.x = 0;
    setup.y = 0;
    setup.sequence = 0;
    setup.priority = 1;
    setup.bgPriority = 1;
    for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
        setup.x = i * 40 + 24;
        setup.y = 64;
        wk->cursors[i] =
            func_0204c040(wk->actorUnit, wk->resources[OEKAKI_RES_MAIN].chars, wk->resources[OEKAKI_RES_MAIN].palette,
                          wk->resources[OEKAKI_RES_MAIN].cellAnims, &setup, 0, HEAPID_OEKAKI);
        func_0204c520(wk->cursors[i], TRUE);
        func_0204c488(wk->cursors[i], i);
        func_0204c124(wk->cursors[i], FALSE);
    }
    for (i = 0; i < OEKAKI_BUTTON_COUNT; i++) {
        setup.x = sButtonData[i].x;
        setup.y = sButtonData[i].y;
        wk->buttons[i] =
            func_0204c040(wk->actorUnit, wk->resources[OEKAKI_RES_MAIN].chars, wk->resources[OEKAKI_RES_MAIN].palette,
                          wk->resources[OEKAKI_RES_MAIN].cellAnims, &setup, 0, HEAPID_OEKAKI);
        func_0204c520(wk->buttons[i], TRUE);
        func_0204c488(wk->buttons[i], sButtonData[i].sequence);
        if (i >= OEKAKI_BUTTON_END) {
            func_0204c468(wk->buttons[i], 2);
        }
    }
    func_0204c488(wk->buttons[0], 6);
    for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
        setup.x = 24;
        setup.y = i * 32 + 32 + OEKAKI_SUB_SURFACE_Y;
        wk->nameActors[i] =
            func_0204c040(wk->actorUnit, wk->resources[OEKAKI_RES_SUB].chars, wk->resources[OEKAKI_RES_SUB].palette,
                          wk->resources[OEKAKI_RES_SUB].cellAnims, &setup, 1, HEAPID_OEKAKI);
        func_0204c520(wk->nameActors[i], TRUE);
        func_0204c488(wk->nameActors[i], i);
        func_0204c438(wk->nameActors[i], 1);
        func_0204c124(wk->nameActors[i], FALSE);
    }
    setup.x = 212;
    setup.y = 160;
    wk->endButton =
        func_0204c040(wk->actorUnit, wk->resources[OEKAKI_RES_COMMON].chars, wk->resources[OEKAKI_RES_COMMON].palette,
                      wk->resources[OEKAKI_RES_COMMON].cellAnims, &setup, 0, HEAPID_OEKAKI);
    func_0204c520(wk->endButton, TRUE);
    func_0204c488(wk->endButton, 1);
    func_0204c124(wk->endButton, TRUE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void Oekaki_CreateWindows(OekakiWork *wk, GameProc *proc) {
    int i;

    BmpWin_InitAllocator(HEAPID_OEKAKI);
    wk->msgWindow = BmpWin_CreateDynamic(0, 2, 1, 27, 4, 13, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->msgWindow), 15);
    PrintWindow_Init(&wk->msgPrint, wk->msgWindow);
    wk->drawWindow = BmpWin_CreateDynamic(1, 1, 2, 30, 15, 0, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->drawWindow), 2);
    wk->tcbManager = GFL_TCBExMgrCreate(HEAPID_OEKAKI, HEAPID_OEKAKI, 32, 32);
    wk->nameQueue = func_02021998(HEAPID_OEKAKI);
    wk->printQueue = func_02021998(HEAPID_OEKAKI);
    for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
        wk->nameWindows[i] = BmpWin_CreateDynamic(4, 5, i * 4 + 3, 10, 2, 13, TRUE);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->nameWindows[i]), 0);
        PrintWindow_Init(&wk->namePrint[i], wk->nameWindows[i]);
        BmpWin_TransferNow(wk->nameWindows[i]);
    }
    Oekaki_PrintNames(wk->nameWindows, OEKAKI_COLOR_TEXT, wk);
}

static void Oekaki_FreeWindows(OekakiWork *wk) {
    int i;

    GFL_TCBExMgrFreeTasks(wk->tcbManager);
    GFL_TCBExMgrFree(wk->tcbManager);
    func_02021c44(wk->printQueue);
    func_02021a18(wk->printQueue);
    func_02021c44(wk->nameQueue);
    func_02021a18(wk->nameQueue);
    for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
        BmpWin_Free(wk->nameWindows[i]);
    }
    BmpWin_Free(wk->drawWindow);
    BmpWin_Free(wk->msgWindow);
    BmpWin_FreeAllocator();
}

// Puts a pen's tip at a point
static void Oekaki_SetCursorPos(ClActor *actor, int x, int y) {
    ClActorPos pos;

    pos.x = x;
    pos.y = y - 8;
    func_0204c210(actor, &pos);
}

// The touch of a frame in which the pen is up
static void Oekaki_MakeEmptyTouch(OekakiTouchInfo *info, int color, int brush) {
    OekakiTouchData touchData;

    touchData.size = 0;
    touchData.points[0].x = 0;
    touchData.points[0].y = 0;
    Oekaki_MakeTouchInfo(info, &touchData, color, brush);
}

// The picture
static const TouchRect sDrawAreaRects[] = {
    { 16, 136, 8, 248 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

// The buttons, in the order of OEKAKI_BUTTON_*
static const TouchRect sOekakiButtonRects[] = {
    { 150, 189, 0, 23 },         { 150, 189, 24, 47 },   { 150, 189, 48, 71 },   { 150, 189, 72, 95 },
    { 150, 189, 96, 119 },       { 150, 189, 120, 143 }, { 150, 189, 144, 167 }, { 150, 189, 168, 191 },
    { 150, 189, 192, 255 },      { 1, 13, 14, 26 },      { 1, 13, 30, 42 },      { 1, 13, 46, 58 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

// Handles the buttons, and queues what the player draws this frame
static void Oekaki_UpdateTouch(OekakiWork *wk) {
    OekakiTouchData touchData;
    u32 cursorX;
    u32 cursorY;
    OekakiTouchInfo info;
    u32 x;
    u32 y;
    BOOL endPressed = FALSE;
    int button;
    int drawArea;

    button = func_0203da0c(sOekakiButtonRects);
    if (button != TOUCH_RECT_NONE) {
        switch (button) {
        case OEKAKI_BUTTON_COLOR:
        case OEKAKI_BUTTON_COLOR + 1:
        case OEKAKI_BUTTON_COLOR + 2:
        case OEKAKI_BUTTON_COLOR + 3:
        case OEKAKI_BUTTON_COLOR + 4:
        case OEKAKI_BUTTON_COLOR + 5:
        case OEKAKI_BUTTON_COLOR + 6:
        case OEKAKI_BUTTON_COLOR + 7:
            if (wk->color != button) {
                wk->color = button;
                Oekaki_SetColorButtons(wk->buttons, button);
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
            break;
        case OEKAKI_BUTTON_END:
            if (wk->seq == OEKAKI_SEQ_MAIN) {
                if (func_0203ffc4() == 0) {
                    // Not while a member that joined has yet to get the picture
                    if (wk->shareBit != Oekaki_GetJoined(wk)) {
                        GFL_SndSEPlay(SEQ_SE_SELECT4);
                    } else {
                        wk->banFlag = OEKAKI_BAN_ON;
                        endPressed = TRUE;
                        Oekaki_PrintMessage(wk, EventMapchange_Text_WouldLikeStopDrawing, TRUE);
                        Oekaki_SetNextSeq(wk, OEKAKI_SEQ_END_SELECT_MSG_WAIT);
                        Oekaki_SetEndButton(wk->endButton, TRUE);
                        GFL_SndSEPlay(SEQ_SE_CANCEL1);
                    }
                } else {
                    // Nor while the parent has its menu open
                    if (wk->allTouch[0].banFlag == OEKAKI_BAN_ON) {
                        GFL_SndSEPlay(SEQ_SE_SELECT4);
                    } else {
                        endPressed = TRUE;
                        Oekaki_PrintMessage(wk, EventMapchange_Text_WouldLikeStopDrawing, TRUE);
                        Oekaki_SetNextSeq(wk, OEKAKI_SEQ_END_SELECT_MSG_WAIT);
                        Oekaki_SetEndButton(wk->endButton, TRUE);
                        GFL_SndSEPlay(SEQ_SE_CANCEL1);
                    }
                }
            }
            break;
        case OEKAKI_BUTTON_BRUSH:
        case OEKAKI_BUTTON_BRUSH + 1:
        case OEKAKI_BUTTON_BRUSH + 2: {
            int i;

            for (i = 0; i < OEKAKI_BRUSH_COUNT; i++) {
                if (button - OEKAKI_BUTTON_BRUSH == i) {
                    func_0204c488(wk->buttons[OEKAKI_BUTTON_BRUSH + i], i * 2 + 30);
                } else {
                    func_0204c488(wk->buttons[OEKAKI_BUTTON_BRUSH + i], i * 2 + 29);
                }
            }
            if (wk->brush != button - OEKAKI_BUTTON_BRUSH) {
                wk->brush = button - OEKAKI_BUTTON_BRUSH;
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
            break;
        }
        }
    }

    // The player's own pen
    drawArea = func_0203d9c8(sDrawAreaRects);
    func_0204c124(wk->cursors[func_0203ffc4()], FALSE);
    if (drawArea != TOUCH_RECT_NONE) {
        func_0203da84(&cursorX, &cursorY);
        Oekaki_SetCursorPos(wk->cursors[func_0203ffc4()], cursorX, cursorY);
        func_0204c124(wk->cursors[func_0203ffc4()], TRUE);
    }

    x = 0;
    y = 0;
    if (func_0203da2c() == TRUE) {
        // A line from the point of the frame before
        func_0203da84(&x, &y);
        touchData.size = 2;
        touchData.points[0].x = wk->touchData.points[1].x;
        touchData.points[0].y = wk->touchData.points[1].y;
        touchData.points[1].x = x;
        touchData.points[1].y = y;
        Oekaki_MakeTouchInfo(&info, &touchData, wk->color, wk->brush);
        if (endPressed == TRUE) {
            info.size = 0;
        }
    } else {
        touchData.points[1].x = 0;
        touchData.points[1].y = 0;
        Oekaki_MakeEmptyTouch(&info, wk->color, wk->brush);
    }
    Oekaki_QueuePush(&info, &wk->queue, 0);
    if (func_0203ffc4() == 0) {
        Oekaki_QueueAdvanceParent(&wk->queue);
    } else {
        Oekaki_QueueAdvanceChild(&wk->queue);
    }
    wk->touchData = touchData;
}

// Shows the picked color's button as pressed
static void Oekaki_SetColorButtons(ClActor **buttons, int color) {
    int i;

    for (i = 0; i < OEKAKI_COLOR_COUNT; i++) {
        if (i == color) {
            func_0204c488(buttons[i], sButtonData[i].sequence + 1);
        } else {
            func_0204c488(buttons[i], sButtonData[i].sequence);
        }
    }
}

static void Oekaki_SetEndButton(ClActor *actor, BOOL pressed) {
    if (pressed == TRUE) {
        func_0204c488(actor, 9);
    } else {
        func_0204c488(actor, 1);
    }
}

static int Oekaki_SeqMain(OekakiWork *wk, int seq) {
    Oekaki_UpdateTouch(wk);
    if (func_0203ffc4() == 0) {
        // A parent with no children has nobody to send to
        if (Oekaki_CountMembers(wk) != 1) {
            Oekaki_SendTouch(wk);
        }
    } else {
        Oekaki_SendTouch(wk);
    }
    Oekaki_UpdateCursors(wk);
    Oekaki_DrawTouches(wk->drawWindow, wk->allTouch, wk->oldTouch, TRUE);
    return seq;
}

// Draws what has arrived while the player cannot draw
static void Oekaki_DrawOthers(OekakiWork *wk) {
    Oekaki_UpdateCursors(wk);
    Oekaki_DrawTouches(wk->drawWindow, wk->allTouch, wk->oldTouch, FALSE);
}

static int Oekaki_SeqNewcomer(OekakiWork *wk, int seq) {
    if (wk->newcomerId == func_0203ffc4()) {
        Oekaki_PrintMessage(wk, EventMapchange_Text_JoinedDrawingSession, FALSE);
    } else {
        Oekaki_PrintMessage(wk, EventMapchange_Text_HasJoined, FALSE);
    }
    Oekaki_SetNextSeq(wk, OEKAKI_SEQ_NEWCOMER_WAIT);
    Oekaki_CloseMenu(wk);
    GFL_SndSEPlay(SEQ_SE_DECIDE1);
    gfxRegSetBrightnessBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3,
                             -6);
    Oekaki_DrawOthers(wk);
    return seq;
}

// Waits for OEKAKI_NET_CMD_PICTURE_END
static int Oekaki_SeqNewcomerWait(OekakiWork *wk, int seq) {
    int i;

    for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
        func_0204c124(wk->cursors[i], FALSE);
    }
    return seq;
}

static int Oekaki_SeqNewcomerEnd(OekakiWork *wk, int seq) {
    reg_G2_BLDCNT = 0;
    Oekaki_SetEntry(wk, 1);
    Oekaki_SetNextSeq(wk, OEKAKI_SEQ_MAIN);
    Oekaki_DrawOthers(wk);
    wk->joining = FALSE;
    return seq;
}

static int Oekaki_SeqEndSelectMsgWait(OekakiWork *wk, int seq) {
    if (Oekaki_IsMessageDone(wk)) {
        if (wk->menu == NULL) {
            wk->menu = Oekaki_CreateMenu(wk);
        }
        Oekaki_SetNextSeq(wk, OEKAKI_SEQ_END_SELECT);
    }
    Oekaki_DrawOthers(wk);
    return seq;
}

static int Oekaki_SeqEndSelect(OekakiWork *wk, int seq) {
    wk->myTouch.size = 0;
    // The parent has its own menu open
    if (wk->allTouch[0].banFlag == OEKAKI_BAN_ON && func_0203ffc4() != 0) {
        Oekaki_DrawOthers(wk);
        AppTaskMenu_UpdateText(wk->menu);
        if (Oekaki_IsMenuAreaTouched(wk)) {
            GFL_SndSEPlay(SEQ_SE_SELECT4);
        }
        return seq;
    }
    if (func_0203ffc4() == 0 && wk->leavingBits != 0) {
        Oekaki_DrawOthers(wk);
        AppTaskMenu_UpdateText(wk->menu);
        return seq;
    }
    switch (Oekaki_UpdateMenu(wk)) {
    case 0:
        if (func_0203ffc4() == 0) {
            Oekaki_SetNextSeq(wk, OEKAKI_SEQ_PARENT_END_SELECT_MSG_WAIT);
            Oekaki_PrintMessage(wk, EventMapchange_Text_IfLeaderDropsOut, TRUE);
        } else {
            Oekaki_SetNextSeq(wk, OEKAKI_SEQ_END_REQUEST);
            BmpWin_ClearFrame(wk->msgWindow, 2);
        }
        BmpWin_TransferNow(wk->drawWindow);
        break;
    case 1:
        Oekaki_SetNextSeq(wk, OEKAKI_SEQ_MAIN);
        Oekaki_SetEndButton(wk->endButton, FALSE);
        BmpWin_ClearFrame(wk->msgWindow, 2);
        BmpWin_TransferNow(wk->drawWindow);
        if (func_0203ffc4() == 0) {
            OekakiComm_LimitEntry(wk, 1);
            wk->banFlag = OEKAKI_BAN_NONE;
        }
        break;
    }
    Oekaki_DrawOthers(wk);
    return seq;
}

// Asks the parent to leave
static int Oekaki_SeqEndRequest(OekakiWork *wk, int seq) {
    OekakiSignal signal;

    sys_memset(&signal, 0, sizeof(signal));
    signal.type = OEKAKI_SIGNAL_LEAVE_REQUEST;
    signal.netId = func_0203ffc4();
    if (func_02042be8(func_02040440(), OEKAKI_NET_CMD_SIGNAL, sizeof(signal), &signal)) {
        Oekaki_SetNextSeq(wk, OEKAKI_SEQ_END_ANSWER_WAIT);
    }
    Oekaki_DrawOthers(wk);
    return seq;
}

static int Oekaki_SeqEndAnswerWait(OekakiWork *wk, int seq) {
    wk->myTouch.size = 0;
    wk->signalWait = 0;
    Oekaki_DrawOthers(wk);
    return seq;
}

static int Oekaki_SeqEndOk(OekakiWork *wk, int seq) {
    wk->myTouch.size = 0;
    wk->signalWait++;
    if (wk->signalWait > 30) {
        Oekaki_SetNextSeq(wk, OEKAKI_SEQ_END_LEAVE);
        wk->signalWait = 0;
    }
    Oekaki_DrawOthers(wk);
    return seq;
}

static int Oekaki_SeqEndLeave(OekakiWork *wk, int seq) {
    OekakiSignal signal;

    sys_memset(&signal, 0, sizeof(signal));
    signal.type = OEKAKI_SIGNAL_LEAVE;
    signal.netId = func_0203ffc4();
    if (func_02042be8(func_02040440(), OEKAKI_NET_CMD_SIGNAL, sizeof(signal), &signal)) {
        Oekaki_SetNextSeq(wk, OEKAKI_SEQ_END_MSG);
    }
    Oekaki_DrawOthers(wk);
    return seq;
}

static int Oekaki_SeqEndRefused(OekakiWork *wk, int seq) {
    wk->myTouch.size = 0;
    wk->leaving = FALSE;
    Oekaki_SetNextSeq(wk, OEKAKI_SEQ_MAIN);
    Oekaki_SetEndButton(wk->endButton, FALSE);
    Oekaki_DrawOthers(wk);
    return seq;
}

static int Oekaki_SeqEndMsg(OekakiWork *wk, int seq) {
    Oekaki_PrintMessage(wk, EventMapchange_Text_DrawingSessionEnded, TRUE);
    Oekaki_SetNextSeq(wk, OEKAKI_SEQ_END_MSG_WAIT);
    Oekaki_DrawOthers(wk);
    return seq;
}

static int Oekaki_SeqEndMsgWait(OekakiWork *wk, int seq) {
    if (Oekaki_IsMessageDone(wk)) {
        wk->wait = 0;
        Oekaki_SetNextSeq(wk, OEKAKI_SEQ_END_WAIT);
    }
    Oekaki_DrawOthers(wk);
    return seq;
}

static int Oekaki_SeqEndWait(OekakiWork *wk, int seq) {
    wk->wait++;
    if (wk->wait > 60) {
        GFL_WipeSet(WIPE_MODE_BOTH, 8, 8, WIPE_COLOR_BLACK, 16, 1, HEAPID_OEKAKI);
        seq = OEKAKI_PROC_SEQ_FADE_OUT;
    }
    Oekaki_DrawOthers(wk);
    return seq;
}

static int Oekaki_SeqParentEndSelectMsgWait(OekakiWork *wk, int seq) {
    if (Oekaki_IsMessageDone(wk)) {
        if (wk->menu == NULL) {
            wk->menu = Oekaki_CreateMenu(wk);
        }
        Oekaki_SetNextSeq(wk, OEKAKI_SEQ_PARENT_END_SELECT);
    }
    Oekaki_DrawOthers(wk);
    return seq;
}

static int Oekaki_SeqParentEndSelect(OekakiWork *wk, int seq) {
    int result = Oekaki_UpdateMenu(wk);

    if (wk->shareNum != Oekaki_GetMemberCount(wk) || wk->leavingBits != 0) {
        Oekaki_DrawOthers(wk);
        return seq;
    }
    switch (result) {
    case 0:
        Oekaki_SetNextSeq(wk, OEKAKI_SEQ_PARENT_END_SEND);
        UnionApp_CloseEntry(wk->param->app);
        break;
    case 1:
        Oekaki_SetNextSeq(wk, OEKAKI_SEQ_MAIN);
        Oekaki_SetEndButton(wk->endButton, FALSE);
        BmpWin_ClearFrame(wk->msgWindow, 2);
        if (func_0203ffc4() == 0) {
            OekakiComm_LimitEntry(wk, 1);
            wk->banFlag = OEKAKI_BAN_NONE;
        }
        break;
    }
    Oekaki_DrawOthers(wk);
    return seq;
}

// Puts a member's name in a slot of the messages
static void Oekaki_SetPlayerName(OekakiWork *wk, int index, int netId) {
    UnionAppMember *member = UnionApp_GetMember(wk->param->app, netId);

    if (member != NULL) {
        copyVarForText(wk->wordSet, index, &member->info);
    }
}

static int Oekaki_SeqParentEndSend(OekakiWork *wk, int seq) {
    if (func_02042be8(func_02040414(GFL_NET_NETID_SERVER), OEKAKI_NET_CMD_PARENT_END, 0, NULL)) {
        Oekaki_SetNextSeq(wk, OEKAKI_SEQ_PARENT_END_MSG);
        Oekaki_SetPlayerName(wk, 0, 0);
        seq = OEKAKI_PROC_SEQ_END;
    }
    Oekaki_DrawOthers(wk);
    return seq;
}

static int Oekaki_SeqParentEndMsg(OekakiWork *wk, int seq) {
    Oekaki_SetPlayerName(wk, 0, 0);
    Oekaki_PrintMessage(wk, EventMapchange_Text_LeaderHasLeftGroup, TRUE);
    Oekaki_SetNextSeq(wk, OEKAKI_SEQ_PARENT_END_MSG_WAIT);
    Oekaki_DrawOthers(wk);
    return seq;
}

static int Oekaki_SeqParentEndMsgWait(OekakiWork *wk, int seq) {
    if (Oekaki_IsMessageDone(wk)) {
        Oekaki_SetNextSeq(wk, OEKAKI_SEQ_PARENT_END_SYNC_WAIT);
        func_02040b38(func_02040440(), 200, 0x29, (u16)wk->shareBit);
    }
    Oekaki_DrawOthers(wk);
    return seq;
}

// Waits until every member is at the same point, or gone
static int Oekaki_SeqParentEndSyncWait(OekakiWork *wk, int seq) {
    NetHandle *handle = func_02040440();
    int i;
    u32 bits = 0;

    for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
        if (func_02042a80((u8)i) == TRUE) {
            bits |= 1 << i;
        }
    }
    if (func_02040664(handle, 200, 0x29) || Oekaki_CountJoined(wk) == 1 || bits != wk->shareBit) {
        GFL_WipeSet(WIPE_MODE_BOTH, 8, 8, WIPE_COLOR_BLACK, 16, 1, HEAPID_OEKAKI);
        seq = OEKAKI_PROC_SEQ_FADE_OUT;
    }
    Oekaki_DrawOthers(wk);
    return seq;
}

static int Oekaki_Seq21(OekakiWork *wk, int seq) {
    return seq;
}

static int Oekaki_SeqNoMembersMsg(OekakiWork *wk, int seq) {
    Oekaki_PrintMessage(wk, EventMapchange_Text_ThereNoMembersGroup, TRUE);
    Oekaki_SetNextSeq(wk, OEKAKI_SEQ_NO_MEMBERS_MSG_WAIT);
    Oekaki_DrawOthers(wk);
    return seq;
}

static int Oekaki_SeqNoMembersMsgWait(OekakiWork *wk, int seq) {
    if (Oekaki_IsMessageDone(wk)) {
        Oekaki_SetNextSeq(wk, OEKAKI_SEQ_END_WAIT);
    }
    Oekaki_DrawOthers(wk);
    return seq;
}

static int Oekaki_SeqMemberLeft(OekakiWork *wk, int seq) {
    Oekaki_PrintMessage(wk, EventMapchange_Text_HasLeft, TRUE);
    Oekaki_CloseMenu(wk);
    Oekaki_SetNextSeq(wk, OEKAKI_SEQ_MEMBER_LEFT_MSG_WAIT);
    GFL_SndSEPlay(SEQ_SE_DECIDE1);
    if (func_0203ffc4() == 0) {
        Oekaki_SetEntry(wk, 0);
    }
    wk->leftCount = Oekaki_CountJoined(wk);
    Oekaki_DrawOthers(wk);
    return seq;
}

static int Oekaki_SeqMemberLeftMsgWait(OekakiWork *wk, int seq) {
    if ((wk->leftCount != 0 && wk->leftCount != Oekaki_CountJoined(wk)) || !(Oekaki_GetJoined(wk) & wk->leavingBits)) {
        wk->leftCount = 0;
    }
    if (Oekaki_IsMessageDone(wk)) {
        Oekaki_SetNextSeq(wk, OEKAKI_SEQ_MEMBER_LEFT_WAIT);
        wk->wait = 0;
    }
    Oekaki_DrawOthers(wk);
    return seq;
}

static int Oekaki_SeqMemberLeftWait(OekakiWork *wk, int seq) {
    if ((wk->leftCount != 0 && wk->leftCount != Oekaki_CountJoined(wk)) || !(Oekaki_GetJoined(wk) & wk->leavingBits)) {
        wk->leftCount = 0;
    }
    wk->wait++;
    if (wk->wait > 60 && wk->leftCount == 0) {
        Oekaki_ClearMessage(wk);
        Oekaki_SetNextSeq(wk, OEKAKI_SEQ_MAIN);
        if (func_0203ffc4() == 0) {
            wk->banFlag = OEKAKI_BAN_NONE;
            Oekaki_SetEntry(wk, 1);
        }
        wk->shareNum = UnionApp_CountJoined(wk->param->app);
        wk->shareBit = UnionApp_GetJoined(wk->param->app);
    }
    Oekaki_DrawOthers(wk);
    return seq;
}

void Oekaki_SetStatus(OekakiWork *wk, int seq, int netId) {
    switch (seq) {
    case OEKAKI_SEQ_NEWCOMER:
        if (wk->leaving == TRUE) {
            return;
        }
        if (wk->seq == OEKAKI_SEQ_END_SELECT || wk->seq == OEKAKI_SEQ_PARENT_END_SELECT) {
            Oekaki_CloseMenu(wk);
        }
        Oekaki_SetEndButton(wk->endButton, FALSE);
        Oekaki_SetPlayerName(wk, 0, netId);
        wk->newcomerId = netId;
        wk->leavingBits = 0;
        break;
    case OEKAKI_SEQ_NEWCOMER_END:
        Oekaki_ClearMessage(wk);
        break;
    case OEKAKI_SEQ_MEMBER_LEFT:
        if (wk->leaving == TRUE) {
            return;
        }
        Oekaki_SetPlayerName(wk, 0, netId);
        if (netId == func_0203ffc4()) {
            return;
        }
        if (func_0203ffc4() == 0) {
            wk->leavingBits &= 0xffff ^ netId;
        }
        if (wk->seq == OEKAKI_SEQ_END_SELECT || wk->seq == OEKAKI_SEQ_PARENT_END_SELECT) {
            Oekaki_CloseMenu(wk);
            BmpWin_TransferNow(wk->drawWindow);
        }
        Oekaki_SetEndButton(wk->endButton, FALSE);
        break;
    case OEKAKI_SEQ_END_OK:
        Oekaki_RequestSend(wk, OEKAKI_NET_CMD_LEAVING, func_0203ffc4());
        break;
    case OEKAKI_SEQ_END_REFUSED:
    case OEKAKI_SEQ_PARENT_END_MSG:
        break;
    default:
        return;
    }
    Oekaki_SetNextSeq(wk, seq);
}

// The brushes, as rows of 4 bits per pixel. C is the color in each pixel that is set
// clang-format off
#define C(color, pixels) ((color) * (pixels))
#define BRUSH_SMALL(color) { \
    0x00,           0x00,           0x00,           0x00, \
    0x00,           0x00,           0x00,           0x00, \
    0x00,           C(color, 0x11), 0x00,           0x00, \
    0x00,           C(color, 0x11), 0x00,           0x00, \
    0x00,           0x00,           0x00,           0x00, \
    0x00,           0x00,           0x00,           0x00, \
}
#define BRUSH_MEDIUM(color) { \
    0x00,           0x00,           0x00,           0x00, \
    0x00,           C(color, 0x11), 0x00,           0x00, \
    C(color, 0x10), C(color, 0x11), C(color, 0x01), 0x00, \
    C(color, 0x10), C(color, 0x11), C(color, 0x01), 0x00, \
    0x00,           C(color, 0x11), 0x00,           0x00, \
    0x00,           0x00,           0x00,           0x00, \
}
#define BRUSH_LARGE(color) { \
    C(color, 0x10), C(color, 0x11), C(color, 0x01), 0x00, \
    C(color, 0x11), C(color, 0x11), C(color, 0x11), 0x00, \
    C(color, 0x11), C(color, 0x11), C(color, 0x11), 0x00, \
    C(color, 0x11), C(color, 0x11), C(color, 0x11), 0x00, \
    C(color, 0x11), C(color, 0x11), C(color, 0x11), 0x00, \
    C(color, 0x10), C(color, 0x11), C(color, 0x01), 0x00, \
}
// clang-format on

// The pixels of each brush in each color
static const u8 sBrushPatterns[OEKAKI_BRUSH_COUNT][OEKAKI_COLOR_COUNT][OEKAKI_BRUSH_SIZE * 4] = {
    {
        BRUSH_SMALL(1),
        BRUSH_SMALL(2),
        BRUSH_SMALL(3),
        BRUSH_SMALL(4),
        BRUSH_SMALL(5),
        BRUSH_SMALL(6),
        BRUSH_SMALL(7),
        BRUSH_SMALL(8),
    },
    {
        BRUSH_MEDIUM(1),
        BRUSH_MEDIUM(2),
        BRUSH_MEDIUM(3),
        BRUSH_MEDIUM(4),
        BRUSH_MEDIUM(5),
        BRUSH_MEDIUM(6),
        BRUSH_MEDIUM(7),
        BRUSH_MEDIUM(8),
    },
    {
        BRUSH_LARGE(1),
        BRUSH_LARGE(2),
        BRUSH_LARGE(3),
        BRUSH_LARGE(4),
        BRUSH_LARGE(5),
        BRUSH_LARGE(6),
        BRUSH_LARGE(7),
        BRUSH_LARGE(8),
    },
};

// Draws a brush into the picture, clipped at the left and the top
static void Oekaki_DrawBrush(BmpWin *window, const u8 *brush, int srcX, int srcY, int width, int height, int x, int y,
                             int brushWidth, int brushHeight) {
    GFLBitmap *bitmap;

    if (x < 0) {
        int clip = -x;

        if (clip > brushWidth) {
            clip = brushWidth;
        }
        srcX += clip;
        x = 0;
        width -= clip;
    }
    if (y < 0) {
        int clip = -y;

        if (clip > brushHeight) {
            clip = brushHeight;
        }
        srcY += clip;
        y = 0;
        height -= clip;
    }
    bitmap = GFL_BitmapWrap((void *)brush, 8, 8, 0x20, HEAPID_OEKAKI);
    GFL_BitmapCopyArea(bitmap, BmpWin_GetBitmap(window), srcX, srcY, x, y, width, height, 0);
    GFL_BitmapFree(bitmap);
}

// Draws a line with a brush from *oldX, *oldY to x, y, and makes that the old point
static void Oekaki_DrawLine(BmpWin *window, const u8 *brush, int x, int y, int *oldX, int *oldY, int penDown,
                            int oldSize) {
    int x0 = *oldX;
    int y0 = *oldY;
    int xEnd = x;
    int dx;
    int yEnd = y;
    int dy;
    int step;
    int error;

    if (!penDown && !oldSize) {
        *oldX = x;
        *oldY = y;
        return;
    }
    dx = x - x0;
    if (dx < 0) {
        dx = -dx;
    }
    dy = y - y0;
    if (dy < 0) {
        dy = -dy;
    }
    if (dx > dy) {
        if (x0 > x) {
            if (y0 > y) {
                step = 1;
            } else {
                step = -1;
            }
            xEnd = x0;
            x0 = x;
            y0 = y;
        } else {
            if (y0 < y) {
                step = 1;
            } else {
                step = -1;
            }
        }
        Oekaki_DrawBrush(window, brush, 0, 0, OEKAKI_BRUSH_SIZE, OEKAKI_BRUSH_SIZE, x0, y0, OEKAKI_BRUSH_SIZE,
                         OEKAKI_BRUSH_SIZE);
        error = dx >> 1;
        for (x0++; x0 <= xEnd; x0++) {
            error -= dy;
            if (error < 0) {
                error += dx;
                y0 += step;
            }
            Oekaki_DrawBrush(window, brush, 0, 0, OEKAKI_BRUSH_SIZE, OEKAKI_BRUSH_SIZE, x0, y0, OEKAKI_BRUSH_SIZE,
                             OEKAKI_BRUSH_SIZE);
        }
    } else {
        if (y0 > y) {
            if (x0 > x) {
                step = 1;
            } else {
                step = -1;
            }
            yEnd = y0;
            y0 = y;
            x0 = x;
        } else {
            if (x0 < x) {
                step = 1;
            } else {
                step = -1;
            }
        }
        Oekaki_DrawBrush(window, brush, 0, 0, OEKAKI_BRUSH_SIZE, OEKAKI_BRUSH_SIZE, x0, y0, OEKAKI_BRUSH_SIZE,
                         OEKAKI_BRUSH_SIZE);
        error = dy >> 1;
        for (y0++; y0 <= yEnd; y0++) {
            error -= dx;
            if (error < 0) {
                error += dy;
                x0 += step;
            }
            Oekaki_DrawBrush(window, brush, 0, 0, OEKAKI_BRUSH_SIZE, OEKAKI_BRUSH_SIZE, x0, y0, OEKAKI_BRUSH_SIZE,
                             OEKAKI_BRUSH_SIZE);
        }
    }
    *oldX = x;
    *oldY = y;
}

// Keeps where each member's line ended
static void Oekaki_SaveOldTouch(OekakiTouchInfo *touch, OekakiOldTouch *old) {
    int i;

    for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
        old[i].size = touch[i].size;
        if (touch[i].size != 0) {
            old[i].x = touch[i].x[touch[i].size - 1];
            old[i].y = touch[i].y[touch[i].size - 1];
        }
    }
}

// Draws every member's line of the frame, and clears the touches
static void Oekaki_DrawTouches(BmpWin *window, OekakiTouchInfo *touch, OekakiOldTouch *old, BOOL transfer) {
    int i;
    BOOL drawn = FALSE;
    int x;
    int y;

    for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
        if (touch[i].size == 2) {
            x = touch[i].x[0] - 9;
            y = touch[i].y[0] - 17;
            Oekaki_DrawLine(window, sBrushPatterns[touch[i].brush][touch[i].color], touch[i].x[1] - 9,
                            touch[i].y[1] - 17, &x, &y, TRUE, old[i].size);
            drawn = TRUE;
        }
    }
    if (drawn && transfer) {
        BmpWin_TransferNow(window);
    }
    Oekaki_SaveOldTouch(touch, old);
    for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
        touch[i].size = 0;
    }
}

// Shows the other members' pens where they draw
static void Oekaki_UpdateCursors(OekakiWork *wk) {
    OekakiTouchInfo *touch = wk->allTouch;
    int i;

    for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
        if (i != func_0203ffc4()) {
            if (touch[i].size != 0 && UnionApp_GetMember(wk->param->app, i) != NULL) {
                func_0204c124(wk->cursors[i], TRUE);
                Oekaki_SetCursorPos(wk->cursors[i], touch[i].x[touch[i].size - 1], touch[i].y[touch[i].size - 1]);
            } else {
                func_0204c124(wk->cursors[i], FALSE);
            }
        }
    }
}

// Pulses the green of a color of the actors' palette
static void Oekaki_UpdatePalette(u16 *angle) {
    u16 color;

    *angle += 20;
    if (*angle > 360) {
        *angle = 0;
    }
    color = GX_RGB(29, func_02044360(*angle) * 10 / FX16_ONE + 15, 0);
    gfxUploadStdPaletteObjA(&color, 12 * sizeof(u16), sizeof(u16));
}

static int Oekaki_CountMembers(OekakiWork *wk) {
    int i;
    int count = 0;

    for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
        if (UnionApp_GetMember(wk->param->app, i) != NULL) {
            count++;
        }
    }
    return count;
}

// Prints the members' names on the top screen when the members have changed
static void Oekaki_PrintNames(BmpWin **windows, u16 color, OekakiWork *wk) {
    int myId = func_0203ffc4();
    int i;

    if (!Oekaki_UpdateMembers(wk)) {
        return;
    }
    for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
        if (wk->members[i].member == NULL) {
            func_0204c124(wk->nameActors[i], FALSE);
        } else {
            func_0204c124(wk->nameActors[i], TRUE);
        }
    }
    func_02021c44(wk->nameQueue);
    for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
        GFL_BitmapFillArea(BmpWin_GetBitmap(windows[i]), 0, 0, 80, 16, 0);
    }
    for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
        if (wk->members[i].member != NULL) {
            textCopy(wk->members[i].member->info.name, wk->nameBufs[i]);
            if (myId == i) {
                PrintWindow_Print(&wk->namePrint[i], wk->nameQueue, 0, 0, wk->nameBufs[i], wk->font,
                                  OEKAKI_COLOR_NAME_MINE);
            } else {
                PrintWindow_Print(&wk->namePrint[i], wk->nameQueue, 0, 0, wk->nameBufs[i], wk->font, color);
            }
        }
        BmpWin_TransferNow(windows[i]);
    }
}

// Sends the oldest frame of the queue: the parent every member's touch, a child its own
static void Oekaki_SendTouch(OekakiWork *wk) {
    NetHandle *handle;

    if (func_0203ffc4() == 0) {
        handle = func_02040440();
        if (func_02042cfc(handle)) {
            Oekaki_QueuePopAll(wk->sendTouch, &wk->queue);
            wk->sendTouch[0].banFlag = wk->banFlag;
            if (func_02042be8(handle, OEKAKI_NET_CMD_ALL_TOUCH, sizeof(wk->sendTouch), wk->sendTouch) == TRUE) {
                Oekaki_QueueNext(&wk->queue);
            }
        }
    } else {
        handle = func_02040440();
        Oekaki_QueuePop(&wk->myTouch, &wk->queue);
        if (func_02042cfc(handle)) {
            if (func_02042be8(func_02040440(), OEKAKI_NET_CMD_TOUCH, sizeof(wk->myTouch), &wk->myTouch) == TRUE) {
                Oekaki_QueueNext(&wk->queue);
            }
        }
    }
}

static int Oekaki_GetMemberCount(OekakiWork *wk) {
    return UnionApp_CountJoined(wk->param->app);
}

// Reads the members, and returns whether any has changed
static BOOL Oekaki_UpdateMembers(OekakiWork *wk) {
    BOOL changed = FALSE;
    int i;

    for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
        wk->members[i].old = wk->members[i].member;
        wk->members[i].member = UnionApp_GetMember(wk->param->app, i);
    }
    for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
        if (wk->members[i].old != wk->members[i].member) {
            changed = TRUE;
        }
    }
    return changed;
}

// Prints a message in the message window, at once or a character at a time
static void Oekaki_PrintMessage(OekakiWork *wk, int messageId, BOOL stream) {
    StrBuf *format = GFL_StrBufCreate(80, HEAPID_OEKAKI);

    GFL_MsgDataLoadStrbuf(wk->msgData, messageId, format);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->msgBuf, format);
    GFL_StrBufFree(format);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->msgWindow), 15);
    BmpWin_DrawFrame(wk->msgWindow, FALSE, 1, 10);
    if (wk->stream != NULL) {
        func_020223cc(wk->stream);
        wk->stream = NULL;
    }
    if (!stream) {
        PrintWindow_Print(&wk->msgPrint, wk->printQueue, 0, 0, wk->msgBuf, wk->font, OEKAKI_COLOR_TEXT);
    } else {
        wk->stream = func_02022268(wk->msgWindow, 0, 0, wk->msgBuf, wk->font, func_02017bcc(), wk->tcbManager, 1,
                                   HEAPID_OEKAKI, 0xf0f);
    }
    BmpWin_TransferNow(wk->msgWindow);
}

static BOOL Oekaki_IsMessageDone(OekakiWork *wk) {
    u32 state = func_020223b4(wk->stream);

    if (state == PRINT_STREAM_PAUSED) {
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            func_020223bc(wk->stream);
        }
        return FALSE;
    }
    if (state == PRINT_STREAM_DONE) {
        func_020223cc(wk->stream);
        wk->stream = NULL;
        return TRUE;
    }
    if (wk->stream == NULL) {
        return TRUE;
    }
    return FALSE;
}

static void Oekaki_ClearMessage(OekakiWork *wk) {
    BmpWin_ClearFrame(wk->msgWindow, 0);
}

// The parent's: lets up to `limit` more players enter
static void Oekaki_SetEntry(OekakiWork *wk, u8 limit) {
    if (func_0203ffc4() == 0) {
        Oekaki_CountJoined(wk);
        OekakiComm_LimitEntry(wk, limit);
    }
}

// The parent's update: follows the members that come and go, and returns the proc's next sequence
static int Oekaki_UpdateParent(OekakiWork *wk) {
    int count = Oekaki_GetMemberCount(wk);

    switch (count) {
    case 1:
        // Everyone else has left
        if (wk->seq <= OEKAKI_SEQ_PARENT_END_SELECT) {
            if (Oekaki_GetJoined(wk) != 1) {
                wk->waitAlone = TRUE;
            }
            wk->nextSeq = OEKAKI_SEQ_NO_MEMBERS_MSG;
            if (wk->menuOpen) {
                Oekaki_CloseMenu(wk);
                func_0204c488(wk->buttons[OEKAKI_BUTTON_END], 21);
            }
            return OEKAKI_PROC_SEQ_END;
        }
        break;
    case 2:
    case 3:
    case 4:
        if (count < wk->memberCount) {
            if (wk->banFlag == OEKAKI_BAN_ON) {
                Oekaki_SetEntry(wk, 0);
            } else if (!wk->joining) {
                Oekaki_SetEntry(wk, 1);
            } else {
                Oekaki_SetEntry(wk, 0);
            }
        }
        break;
    case 0:
    case 5:
        break;
    }
    if (count < wk->memberCount) {
        wk->shareNum = Oekaki_CountJoined(wk);
        wk->shareBit = Oekaki_GetJoined(wk);
        // The member that was joining is the one that left
        if (wk->joining && !(wk->shareBit & wk->joiningBits)) {
            Oekaki_SetEntry(wk, 1);
            wk->banFlag = OEKAKI_BAN_NONE;
            wk->joining = FALSE;
            wk->joiningBits = 0;
        }
    }
    wk->memberCount = Oekaki_GetMemberCount(wk);
    if (wk->shareNum < Oekaki_CountJoined(wk)) {
        wk->banFlag = OEKAKI_BAN_ON;
        wk->joining = TRUE;
        wk->joiningBits = Oekaki_GetJoined(wk) ^ wk->shareBit;
    }
    return OEKAKI_PROC_SEQ_MAIN;
}

static void Oekaki_SetNextSeq(OekakiWork *wk, int seq) {
    wk->nextSeq = seq;
}

// Changes to the next sequence, which while the proc ends has to be one that runs then
static void Oekaki_UpdateSeq(OekakiWork *wk, int procSeq) {
    if (wk->seq != wk->nextSeq) {
        if (procSeq != OEKAKI_PROC_SEQ_END) {
            wk->seq = wk->nextSeq;
        } else if (sSeqTable[wk->nextSeq].runsAtEnd) {
            wk->seq = wk->nextSeq;
        }
    }
}

static void Oekaki_CloseMenu(OekakiWork *wk) {
    if (wk->menuOpen) {
        if (wk->menu != NULL) {
            Oekaki_FreeMenu(wk);
        }
        wk->menuOpen = FALSE;
    }
}

// Where the menu comes up
static const TouchRect sMenuAreaRects[] = {
    { 48, 112, 200, 248 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static BOOL Oekaki_IsMenuAreaTouched(OekakiWork *wk) {
    if (func_0203da0c(sMenuAreaRects) != TOUCH_RECT_NONE) {
        return TRUE;
    }
    return FALSE;
}

// Makes what is sent of the touch screen's points
static void Oekaki_MakeTouchInfo(OekakiTouchInfo *info, OekakiTouchData *touchData, int color, int brush) {
    int last;
    int i;

    if (touchData->size != 0) {
        info->x[0] = touchData->points[0].x;
        info->y[0] = touchData->points[0].y;
        last = touchData->size - 1;
        info->x[1] = touchData->points[last].x;
        info->y[1] = touchData->points[last].y;
        // A point at the origin is no point
        for (i = 0; i < 2; i++) {
            if (info->x[i] + info->y[i] == 0) {
                touchData->size = 0;
            }
        }
    }
    if (touchData->size >= 2) {
        info->size = 2;
    } else {
        info->size = touchData->size;
    }
    info->color = color;
    info->brush = brush;
}

static int Oekaki_CountJoined(OekakiWork *wk) {
    return UnionApp_CountJoined(wk->param->app);
}

static u32 Oekaki_GetJoined(OekakiWork *wk) {
    return UnionApp_GetJoined(wk->param->app);
}

static void Oekaki_ShowScreens(void) {
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabled(0, TRUE);
    GFL_BGSysSetBGEnabled(1, TRUE);
    GFL_BGSysSetBGEnabled(2, TRUE);
    GFL_BGSysSetBGEnabled(4, TRUE);
    GFL_BGSysSetBGEnabled(5, TRUE);
    GX_SetDispSelect(GX_DISP_SELECT_SUB_MAIN);
}

static const OekakiMenuItem sOekakiMenuItems[] = {
    { EventMapchange_Text_Yes, 0 },
    { EventMapchange_Text_No, 0 },
};

// The Yes and No of the question whether to leave
static AppTaskMenu *Oekaki_CreateMenu(OekakiWork *wk) {
    AppTaskMenuInit init;
    AppTaskMenu *menu;
    int i;

    wk->menuRes = AppTaskMenuRes_Create(0, 2, wk->font, wk->printQueue, HEAPID_OEKAKI);
    for (i = 0; i < 2; i++) {
        wk->menuItems[i].str = GFL_MsgDataLoadStrbufNew(wk->msgData, sOekakiMenuItems[i].messageId);
        wk->menuItems[i].color = PRINT_COLOR(14, 15, 3);
        wk->menuItems[i].type = sOekakiMenuItems[i].type;
    }
    init.heapId = HEAPID_OEKAKI;
    init.itemCount = 2;
    init.items = wk->menuItems;
    init.posType = APP_TASKMENU_POS_TOP_LEFT;
    init.x = 21;
    init.y = 7;
    init.width = 10;
    init.height = 3;
    menu = AppTaskMenu_Create(&init, wk->menuRes);
    wk->menuOpen = TRUE;
    return menu;
}

// Returns the item picked, or 2 while none is
static int Oekaki_UpdateMenu(OekakiWork *wk) {
    int result = 2;

    AppTaskMenu_Update(wk->menu);
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        if (AppTaskMenu_GetCursorPos(wk->menu) == 0) {
            result = 0;
        } else {
            result = 1;
        }
        Oekaki_FreeMenu(wk);
    }
    return result;
}

static void Oekaki_FreeMenu(OekakiWork *wk) {
    AppTaskMenu_Free(wk->menu);
    GFL_StrBufFree(wk->menuItems[1].str);
    GFL_StrBufFree(wk->menuItems[0].str);
    AppTaskMenuRes_Free(wk->menuRes);
    wk->menu = NULL;
}

// Records the other members as trainers the player has met
static void Oekaki_RecordMetTrainers(OekakiWork *wk) {
    RivalDataSave *rivalData = getHollow_RivalData(GameData_GetSaveControl(wk->param->gameData));
    int i;

    for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
        UnionAppMember *member = UnionApp_GetMember(wk->param->app, i);

        if (member != NULL && i != func_0203ffc4()) {
            Misc_AddMetTrainer(rivalData, getIDAsUInt(&member->info));
        }
    }
}

static void Oekaki_FlushPrint(OekakiWork *wk) {
    int i;

    func_02021a3c(wk->nameQueue);
    func_02021a3c(wk->printQueue);
    for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
        PrintWindow_Flush(&wk->namePrint[i], wk->nameQueue);
    }
    PrintWindow_Flush(&wk->msgPrint, wk->printQueue);
}

// Sends the command that waits, until it goes out
static void Oekaki_SendRequest(OekakiWork *wk) {
    switch (wk->sendCommand) {
    case OEKAKI_NET_CMD_NEWCOMER:
        if (func_02042be8(func_02040414(GFL_NET_NETID_SERVER), OEKAKI_NET_CMD_NEWCOMER, 1, &wk->sendArg)) {
            wk->sendCommand = 0;
        }
        break;
    case OEKAKI_NET_CMD_PICTURE_END:
        if (func_02042be8(func_02040414(GFL_NET_NETID_SERVER), OEKAKI_NET_CMD_PICTURE_END, 0, NULL)) {
            wk->sendCommand = 0;
        }
        break;
    case OEKAKI_NET_CMD_SIGNAL:
        if (func_02042be8(func_02040414(GFL_NET_NETID_SERVER), OEKAKI_NET_CMD_SIGNAL, sizeof(OekakiSignal),
                          &wk->signal)) {
            wk->sendCommand = 0;
        }
        break;
    case OEKAKI_NET_CMD_LEAVING:
        if (func_02042be8(func_02040440(), OEKAKI_NET_CMD_LEAVING, 0, NULL)) {
            wk->sendCommand = 0;
        }
        break;
    }
}

void Oekaki_RequestSend(OekakiWork *wk, int command, int arg) {
    if (wk->sendCommand == 0) {
        wk->sendCommand = command;
        wk->sendArg = arg;
    }
}

// The frame before the one that is being filled
static int Oekaki_QueuePrev(OekakiQueue *queue) {
    int index = queue->end - 1;

    if (index < 0) {
        index = OEKAKI_QUEUE_SIZE - 1;
    }
    return index;
}

// A child's: goes on to the next frame, unless this one and the one before are both empty
static void Oekaki_QueueAdvanceChild(OekakiQueue *queue) {
    if (queue->fifo[queue->end][0].size != 0 || queue->fifo[Oekaki_QueuePrev(queue)][0].size != 0) {
        queue->end++;
        if (queue->end >= OEKAKI_QUEUE_SIZE) {
            queue->end = 0;
        }
    }
}

// The parent's: the same for the touches of every member
static void Oekaki_QueueAdvanceParent(OekakiQueue *queue) {
    int i;
    int empty = 0;
    int emptyPrev = 0;

    for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
        if (queue->fifo[queue->end][i].size == 0) {
            empty++;
        }
    }
    if (empty == OEKAKI_MEMBER_MAX) {
        int prev = Oekaki_QueuePrev(queue);

        for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
            if (queue->fifo[prev][i].size == 0) {
                emptyPrev++;
            }
        }
    }
    if (emptyPrev != OEKAKI_MEMBER_MAX) {
        queue->end++;
        if (queue->end >= OEKAKI_QUEUE_SIZE) {
            queue->end = 0;
        }
    }
}

static void Oekaki_ClearTouchSizes(OekakiTouchInfo *touch) {
    int i;

    for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
        touch[i].size = 0;
    }
}

// Drops the oldest frame, once it has been sent
static void Oekaki_QueueNext(OekakiQueue *queue) {
    if (queue->start != queue->end) {
        Oekaki_ClearTouchSizes(queue->fifo[queue->start]);
        queue->start++;
        if (queue->start >= OEKAKI_QUEUE_SIZE) {
            queue->start = 0;
        }
    }
}

void Oekaki_QueuePush(OekakiTouchInfo *touch, OekakiQueue *queue, int netId) {
    queue->fifo[queue->end][netId] = *touch;
}

// A child's touch of the oldest frame, or an empty one
static void Oekaki_QueuePop(OekakiTouchInfo *touch, OekakiQueue *queue) {
    if (queue->start != queue->end) {
        *touch = queue->fifo[queue->start][0];
    } else {
        touch->size = 0;
    }
}

// Every member's touch of the oldest frame, or empty ones
static void Oekaki_QueuePopAll(OekakiTouchInfo *touch, OekakiQueue *queue) {
    int i;

    if (queue->start != queue->end) {
        for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
            touch[i] = queue->fifo[queue->start][i];
        }
    } else {
        for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
            touch[i].size = 0;
        }
    }
}
