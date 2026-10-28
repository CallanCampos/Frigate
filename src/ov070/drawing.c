// The ROM names this file oekaki.c.
#include "types.h"
#include "app/drawing.h"
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

// drawing.c, which the ROM names: the drawing board of the Union Room. The parent collects what every member draws
// in a frame and sends it back to all of them, so that each machine draws the same lines; a newcomer gets the
// picture so far in chunks. Names are ours

#define DRAWING_COLOR_TEXT PRINT_COLOR(1, 2, 15)
#define DRAWING_COLOR_NAME_MINE PRINT_COLOR(3, 4, 0)

// Where the top screen's actors are. The SDK's FX32_ONE is a long, which keeps the sum it is in from folding
#define DRAWING_SUB_SURFACE_Y (256 * 0x1000L)

// The brushes are 6 by 6 pixels, in 8 pixel wide rows
#define DRAWING_BRUSH_SIZE 6
#define DRAWING_BRUSH_COUNT 3
#define DRAWING_COLOR_COUNT 8

typedef struct {
    int (*func)(DrawingWork *wk, int seq);
    // Whether the sequence still runs in DRAWING_PROC_SEQ_END
    BOOL runsAtEnd;
} DrawingSeq;

typedef struct {
    u16 x;
    u16 y;
    u16 sequence;
} DrawingButtonData;

typedef struct {
    u32 messageId;
    u32 type;
} DrawingMenuItem;

static BOOL Drawing_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL Drawing_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL Drawing_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void Drawing_VBlank(TCB *tcb, void *data);
static void Drawing_OnJoin(u8 netId, UnionAppMember *member, void *work);
static void Drawing_OnLeave(u8 netId, UnionAppMember *member, void *work);
static void Drawing_SetVRAMBanks(void);
static void Drawing_InitBG(void);
static void Drawing_InitWork(DrawingWork *wk);
static void Drawing_FreeWork(DrawingWork *wk);
static void Drawing_FreeBG(void);
static void Drawing_LoadGraphics(DrawingWork *wk, ArcTool *arc);
static void Drawing_LoadActorResources(DrawingWork *wk, ArcTool *arc);
static void Drawing_CreateActors(DrawingWork *wk);
static void Drawing_CreateWindows(DrawingWork *wk, GameProc *proc);
static void Drawing_FreeWindows(DrawingWork *wk);
static void Drawing_SetCursorPos(ClActor *actor, int x, int y);
static void Drawing_MakeEmptyTouch(DrawingTouchInfo *info, int color, int brush);
static void Drawing_UpdateTouch(DrawingWork *wk);
static void Drawing_SetColorButtons(ClActor **buttons, int color);
static void Drawing_SetEndButton(ClActor *actor, BOOL pressed);
static int Drawing_SeqMain(DrawingWork *wk, int seq);
static void Drawing_DrawOthers(DrawingWork *wk);
static int Drawing_SeqNewcomer(DrawingWork *wk, int seq);
static int Drawing_SeqNewcomerWait(DrawingWork *wk, int seq);
static int Drawing_SeqNewcomerEnd(DrawingWork *wk, int seq);
static int Drawing_SeqEndSelectMsgWait(DrawingWork *wk, int seq);
static int Drawing_SeqEndSelect(DrawingWork *wk, int seq);
static int Drawing_SeqEndRequest(DrawingWork *wk, int seq);
static int Drawing_SeqEndAnswerWait(DrawingWork *wk, int seq);
static int Drawing_SeqEndOk(DrawingWork *wk, int seq);
static int Drawing_SeqEndLeave(DrawingWork *wk, int seq);
static int Drawing_SeqEndRefused(DrawingWork *wk, int seq);
static int Drawing_SeqEndMsg(DrawingWork *wk, int seq);
static int Drawing_SeqEndMsgWait(DrawingWork *wk, int seq);
static int Drawing_SeqEndWait(DrawingWork *wk, int seq);
static int Drawing_SeqParentEndSelectMsgWait(DrawingWork *wk, int seq);
static int Drawing_SeqParentEndSelect(DrawingWork *wk, int seq);
static void Drawing_SetPlayerName(DrawingWork *wk, int index, int netId);
static int Drawing_SeqParentEndSend(DrawingWork *wk, int seq);
static int Drawing_SeqParentEndMsg(DrawingWork *wk, int seq);
static int Drawing_SeqParentEndMsgWait(DrawingWork *wk, int seq);
static int Drawing_SeqParentEndSyncWait(DrawingWork *wk, int seq);
static int Drawing_Seq21(DrawingWork *wk, int seq);
static int Drawing_SeqNoMembersMsg(DrawingWork *wk, int seq);
static int Drawing_SeqNoMembersMsgWait(DrawingWork *wk, int seq);
static int Drawing_SeqMemberLeft(DrawingWork *wk, int seq);
static int Drawing_SeqMemberLeftMsgWait(DrawingWork *wk, int seq);
static int Drawing_SeqMemberLeftWait(DrawingWork *wk, int seq);
static void Drawing_DrawBrush(BmpWin *window, const u8 *brush, int srcX, int srcY, int width, int height, int x, int y,
                              int brushWidth, int brushHeight);
static void Drawing_DrawLine(BmpWin *window, const u8 *brush, int x, int y, int *oldX, int *oldY, int penDown,
                             int oldSize);
static void Drawing_SaveOldTouch(DrawingTouchInfo *touch, DrawingOldTouch *old);
static void Drawing_DrawTouches(BmpWin *window, DrawingTouchInfo *touch, DrawingOldTouch *old, BOOL transfer);
static void Drawing_UpdateCursors(DrawingWork *wk);
static void Drawing_UpdatePalette(u16 *angle);
static int Drawing_CountMembers(DrawingWork *wk);
static void Drawing_PrintNames(BmpWin **windows, u16 color, DrawingWork *wk);
static void Drawing_SendTouch(DrawingWork *wk);
static int Drawing_GetMemberCount(DrawingWork *wk);
static BOOL Drawing_UpdateMembers(DrawingWork *wk);
static void Drawing_PrintMessage(DrawingWork *wk, int messageId, BOOL stream);
static BOOL Drawing_IsMessageDone(DrawingWork *wk);
static void Drawing_ClearMessage(DrawingWork *wk);
static void Drawing_SetEntry(DrawingWork *wk, u8 limit);
static int Drawing_UpdateParent(DrawingWork *wk);
static void Drawing_SetNextSeq(DrawingWork *wk, int seq);
static void Drawing_UpdateSeq(DrawingWork *wk, int procSeq);
static void Drawing_CloseMenu(DrawingWork *wk);
static BOOL Drawing_IsMenuAreaTouched(DrawingWork *wk);
static void Drawing_MakeTouchInfo(DrawingTouchInfo *info, DrawingTouchData *touchData, int color, int brush);
static int Drawing_CountJoined(DrawingWork *wk);
static u32 Drawing_GetJoined(DrawingWork *wk);
static void Drawing_ShowScreens(void);
static AppTaskMenu *Drawing_CreateMenu(DrawingWork *wk);
static int Drawing_UpdateMenu(DrawingWork *wk);
static void Drawing_FreeMenu(DrawingWork *wk);
static void Drawing_RecordMetTrainers(DrawingWork *wk);
static void Drawing_FlushPrint(DrawingWork *wk);
static void Drawing_SendRequest(DrawingWork *wk);
static int Drawing_QueuePrev(DrawingQueue *queue);
static void Drawing_QueueAdvanceChild(DrawingQueue *queue);
static void Drawing_QueueAdvanceParent(DrawingQueue *queue);
static void Drawing_ClearTouchSizes(DrawingTouchInfo *touch);
static void Drawing_QueueNext(DrawingQueue *queue);
static void Drawing_QueuePop(DrawingTouchInfo *touch, DrawingQueue *queue);
static void Drawing_QueuePopAll(DrawingTouchInfo *touch, DrawingQueue *queue);

// The sequences, by DRAWING_SEQ_*
static DrawingSeq sSeqTable[DRAWING_SEQ_COUNT] = {
    { NULL, TRUE },
    { Drawing_SeqNewcomer, FALSE },
    { Drawing_SeqNewcomerWait, FALSE },
    { Drawing_SeqNewcomerEnd, FALSE },
    { Drawing_SeqMain, FALSE },
    { Drawing_SeqEndSelectMsgWait, TRUE },
    { Drawing_SeqEndSelect, TRUE },
    { Drawing_SeqEndAnswerWait, TRUE },
    { Drawing_SeqEndRequest, TRUE },
    { Drawing_SeqEndOk, TRUE },
    { Drawing_SeqEndLeave, TRUE },
    { Drawing_SeqEndRefused, TRUE },
    { Drawing_SeqEndMsg, TRUE },
    { Drawing_SeqEndMsgWait, TRUE },
    { Drawing_SeqEndWait, TRUE },
    { Drawing_SeqParentEndSelectMsgWait, TRUE },
    { Drawing_SeqParentEndSelect, TRUE },
    { Drawing_SeqParentEndSend, TRUE },
    { Drawing_SeqParentEndMsg, TRUE },
    { Drawing_SeqParentEndMsgWait, TRUE },
    { Drawing_SeqParentEndSyncWait, TRUE },
    { Drawing_Seq21, TRUE },
    { Drawing_SeqNoMembersMsg, TRUE },
    { Drawing_SeqNoMembersMsgWait, TRUE },
    { Drawing_SeqMemberLeft, TRUE },
    { Drawing_SeqMemberLeftMsgWait, TRUE },
    { Drawing_SeqMemberLeftWait, TRUE },
};

const GameProcFunctions DRAWING_PROC_FUNCTIONS = { Drawing_Init, Drawing_Main, Drawing_Exit };

static BOOL Drawing_Init(GameProc *proc, u32 *state, void *param, void *work) {
    DrawingWork *wk;
    ArcTool *arc;

    switch (*state) {
    case 0:
        GFL_BGSysDisableAllA();
        GFL_BGSysDisableAllB();
        GFL_HeapCreateChild(HEAPID_USER, HEAPID_DRAWING, 0x50000);
        wk = GFL_ProcInitSubsystem(proc, sizeof(DrawingWork), HEAPID_DRAWING);
        MI_CpuFill8(wk, 0, sizeof(DrawingWork));
        GFL_BGSysCreate(HEAPID_DRAWING);
        wk->wordSet = GFL_WordSetSystemCreateDefault(HEAPID_DRAWING);
        wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVENT_MAPCHANGE, HEAPID_DRAWING);
        wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_DRAWING);
        Drawing_SetVRAMBanks();
        Drawing_InitBG();
        GFL_WipeSet(WIPE_MODE_BOTH, 9, 9, WIPE_COLOR_BLACK, 16, 1, HEAPID_DRAWING);
        wk->param = param;
        arc = GFL_ArcSysCreateFileHandle(ARCID_OEKAKI, HEAPID_DRAWING);
        Drawing_LoadGraphics(wk, arc);
        wk->vblankTcb = GFL_VBlankTCBAdd(Drawing_VBlank, wk, 0);
        Drawing_InitWork(wk);
        Drawing_LoadActorResources(wk, arc);
        Drawing_CreateActors(wk);
        Drawing_CreateWindows(wk, proc);
        Drawing_ShowScreens();
        DrawingComm_AddCommands(wk);
        Drawing_RecordMetTrainers(wk);
        GFL_NetSetIconScreen(TRUE, HEAPID_DRAWING);
        GFL_ArcToolFree(arc);
        (*state)++;
        break;
    case 1:
        *state = 0;
        return TRUE;
    }
    return FALSE;
}

static BOOL Drawing_Main(GameProc *proc, u32 *state, void *param, void *work) {
    DrawingWork *wk = work;

    // The parent forgets the leaving children that are gone
    if (NetSystem_GetNetId() == 0 && wk->leavingBits != 0) {
        wk->leavingBits &= Drawing_GetJoined(wk);
    }
    wk->procSeq = *state;
    Drawing_UpdateSeq(wk, *state);
    switch (*state) {
    case DRAWING_PROC_SEQ_FADE_IN:
        if (GFL_WipeIsFinished()) {
            UnionApp_SetCallbacks(wk->param->app, Drawing_OnJoin, Drawing_OnLeave, wk);
            if (NetSystem_GetNetId() == 0) {
                DrawingComm_LimitEntry(wk, 1);
            }
            *state = DRAWING_PROC_SEQ_MAIN;
        }
        break;
    case DRAWING_PROC_SEQ_MAIN:
        if (sSeqTable[wk->seq].func != NULL) {
            *state = sSeqTable[wk->seq].func(wk, *state);
        }
        Drawing_PrintNames(wk->nameWindows, DRAWING_COLOR_TEXT, wk);
        if (NetSystem_GetNetId() == 0) {
            int next = Drawing_UpdateParent(wk);

            if (*state == DRAWING_PROC_SEQ_MAIN) {
                *state = next;
            }
        }
        Drawing_UpdatePalette(&wk->paletteAngle);
        break;
    case DRAWING_PROC_SEQ_END:
        if (sSeqTable[wk->seq].func != NULL && sSeqTable[wk->seq].runsAtEnd) {
            *state = sSeqTable[wk->seq].func(wk, *state);
        }
        break;
    case DRAWING_PROC_SEQ_FADE_OUT:
        if (GFL_WipeIsFinished()) {
            NetCommand_RemoveTable(DRAWING_NET_CMD_BASE);
            UnionApp_RequestStart(wk->param->app);
            *state = DRAWING_PROC_SEQ_START_WAIT;
        }
        break;
    case DRAWING_PROC_SEQ_START_WAIT:
        if (UnionApp_IsStarted(wk->param->app)) {
            return TRUE;
        }
        break;
    }
    Drawing_SendRequest(wk);
    ClActSys_Draw();
    GFL_TCBExMgrUpdate(wk->tcbManager);
    Drawing_FlushPrint(wk);
    if (GFL_WipeIsFinished() && GFL_NetErrCheck()) {
        return TRUE;
    }
    return FALSE;
}

static BOOL Drawing_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    DrawingWork *wk = work;

    switch (*state) {
    case 0:
        GFL_TCBRemove(wk->vblankTcb);
        ClActSys_FreeChars(wk->resources[DRAWING_RES_SUB].chars);
        ClActSys_FreePltt(wk->resources[DRAWING_RES_SUB].palette);
        ClActSys_FreeCellAnims(wk->resources[DRAWING_RES_SUB].cellAnims);
        ClActSys_FreeChars(wk->resources[DRAWING_RES_COMMON].chars);
        ClActSys_FreePltt(wk->resources[DRAWING_RES_COMMON].palette);
        ClActSys_FreeCellAnims(wk->resources[DRAWING_RES_COMMON].cellAnims);
        ClActSys_FreeChars(wk->resources[DRAWING_RES_MAIN].chars);
        ClActSys_FreePltt(wk->resources[DRAWING_RES_MAIN].palette);
        ClActSys_FreeCellAnims(wk->resources[DRAWING_RES_MAIN].cellAnims);
        ClActUnit_Delete(wk->actorUnit);
        ClActSys_Delete();
        if (wk->menu != NULL) {
            Drawing_FreeMenu(wk);
        }
        Drawing_FreeWindows(wk);
        Drawing_FreeBG();
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
            if (Drawing_GetJoined(wk) == 1) {
                (*state)++;
            }
        } else {
            (*state)++;
        }
        break;
    case 3:
        Drawing_FreeWork(wk);
        GFL_ProcReleaseSubsystem(proc);
        GFL_HeapDelete(HEAPID_DRAWING);
        return TRUE;
    }
    return FALSE;
}

static void Drawing_VBlank(TCB *tcb, void *data) {
    ClActSys_VBlank();
    GFL_BGSysUpdate();
}

static void Drawing_OnJoin(u8 netId, UnionAppMember *member, void *work) {
    DrawingWork *wk = work;

    if (NetSystem_GetNetId() == 0) {
        Drawing_RequestSend(wk, DRAWING_NET_CMD_NEWCOMER, netId);
    }
    Drawing_RecordMetTrainers(wk);
}

static void Drawing_OnLeave(u8 netId, UnionAppMember *member, void *work) {
}

static const BGSysVRAMConfig sDrawingVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static void Drawing_SetVRAMBanks(void) {
    GFL_BGSysSetVRAMBanks(&sDrawingVRAMConfig);
}

static void Drawing_InitBG(void) {
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
    GFL_BGSysClearCharCore(0, 0x20, 0, HEAPID_DRAWING);
    GFL_BGSysClearCharCore(4, 0x20, 0, HEAPID_DRAWING);
    GFL_BGSysClearCharCore(1, 0x20, 0, HEAPID_DRAWING);
    GFL_BGSysClearCharCore(5, 0x20, 0, HEAPID_DRAWING);
}

static void Drawing_InitWork(DrawingWork *wk) {
    int i;

    for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
        wk->nameBufs[i] = GFL_StrBufCreate(8, HEAPID_DRAWING);
        wk->allTouch[i].size = 0;
        wk->oldTouch[i].size = 0;
        wk->members[i].member = NULL;
        wk->members[i].old = NULL;
    }
    wk->endBuf = GFL_StrBufCreate(10, HEAPID_DRAWING);
    wk->msgBuf = GFL_StrBufCreate(80, HEAPID_DRAWING);
    wk->color = 0;
    wk->brush = 1;
    wk->banFlag = DRAWING_BAN_NONE;
    wk->menuOpen = FALSE;
    wk->shareNum = 2;
    wk->shareBit = Drawing_GetJoined(wk);
    wk->unk9E74 = 0;
    wk->waitAlone = FALSE;
    wk->seq = DRAWING_SEQ_NONE;
    wk->joining = FALSE;
    wk->joiningBits = 0;
    wk->menu = NULL;
    NetSystem_GetNetId();
    Drawing_SetNextSeq(wk, DRAWING_SEQ_MAIN);
    GFL_MsgDataLoadStrbuf(wk->msgData, EventMapchange_Text_Empty, wk->endBuf);
    wk->buffer = GFL_HeapAllocate(HEAPID_DRAWING, 0x3c00, FALSE, "oekaki.c", 0x31d);
    wk->memberCount = 0;
}

static void Drawing_FreeWork(DrawingWork *wk) {
    int i;

    GFL_HeapFree(wk->buffer);
    for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
        GFL_StrBufFree(wk->nameBufs[i]);
    }
    GFL_StrBufFree(wk->msgBuf);
    GFL_StrBufFree(wk->endBuf);
}

static void Drawing_FreeBG(void) {
    GX_SetMasterBrightness(-16);
    GXS_SetMasterBrightness(-16);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(0);
    GFL_BGSysFree();
}

static void Drawing_LoadGraphics(DrawingWork *wk, ArcTool *arc) {
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, PALTYPE_MAIN_BG, 0, 12 * 0x20, HEAPID_DRAWING);
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, PALTYPE_SUB_BG, 0, 12 * 0x20, HEAPID_DRAWING);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 13 * 0x20, 0x20, HEAPID_DRAWING);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 13 * 0x20, 0x20, HEAPID_DRAWING);
    GFL_BGSysLoadArcNCGRStatic(arc, 2, 2, 0, 0x2000, FALSE, HEAPID_DRAWING);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 6, 2, 0, 0x600, FALSE, HEAPID_DRAWING);
    GFL_BGSysLoadArcNCGRStatic(arc, 4, 5, 0, 0x2000, FALSE, HEAPID_DRAWING);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 7, 5, 0, 0x600, FALSE, HEAPID_DRAWING);
    LoadSysMsgBox(0, 1, 10, 0, HEAPID_DRAWING);
    LoadSysMsgBox(0, 31, 11, 0, HEAPID_DRAWING);
}

static void Drawing_LoadActorResources(DrawingWork *wk, ArcTool *arc) {
    ArcTool *commonArc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_DRAWING);

    ClActSys_Create(&data_02093f08, &sDrawingVRAMConfig, HEAPID_DRAWING);
    wk->actorUnit = ClActUnit_Create(53, 1, HEAPID_DRAWING);
    wk->resources[DRAWING_RES_MAIN].chars = ClActSys_LoadChars(arc, 3, FALSE, 0, HEAPID_DRAWING);
    wk->resources[DRAWING_RES_MAIN].palette = ClActSys_LoadPltt(arc, 0, 0, 0, HEAPID_DRAWING);
    wk->resources[DRAWING_RES_MAIN].cellAnims = ClActSys_LoadCellAnims(arc, 5, 8, HEAPID_DRAWING);
    wk->resources[DRAWING_RES_COMMON].chars =
        ClActSys_LoadChars(commonArc, AppMenuCommon_GetTouchBarCharFile(), FALSE, 0, HEAPID_DRAWING);
    wk->resources[DRAWING_RES_COMMON].palette =
        ClActSys_LoadPlttRange(commonArc, AppMenuCommon_GetTouchBarPaletteFile(), 0, 11 * 0x20, 0, 3, HEAPID_DRAWING);
    wk->resources[DRAWING_RES_COMMON].cellAnims = ClActSys_LoadCellAnims(
        commonArc, AppMenuCommon_GetTouchBarCellFile(0), AppMenuCommon_GetTouchBarAnimFile(0), HEAPID_DRAWING);
    wk->resources[DRAWING_RES_SUB].chars = ClActSys_LoadChars(arc, 3, FALSE, 1, HEAPID_DRAWING);
    wk->resources[DRAWING_RES_SUB].palette = ClActSys_LoadPltt(arc, 0, 1, 0, HEAPID_DRAWING);
    wk->resources[DRAWING_RES_SUB].cellAnims = ClActSys_LoadCellAnims(arc, 5, 8, HEAPID_DRAWING);
    GFL_ArcToolFree(commonArc);
}

// Where the buttons are, and their animations
static const DrawingButtonData sButtonData[DRAWING_BUTTON_COUNT] = {
    { 12, 172, 5 },   { 36, 172, 7 },   { 60, 172, 9 },   { 84, 172, 11 }, { 108, 172, 13 }, { 132, 172, 15 },
    { 156, 172, 17 }, { 180, 172, 19 }, { 224, 171, 21 }, { 20, 9, 29 },   { 36, 9, 32 },    { 52, 9, 33 },
};

static void Drawing_CreateActors(DrawingWork *wk) {
    ClActorSetup setup;
    int i;

    setup.x = 0;
    setup.y = 0;
    setup.sequence = 0;
    setup.priority = 1;
    setup.bgPriority = 1;
    for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
        setup.x = i * 40 + 24;
        setup.y = 64;
        wk->cursors[i] = ClActor_Create(wk->actorUnit, wk->resources[DRAWING_RES_MAIN].chars,
                                        wk->resources[DRAWING_RES_MAIN].palette,
                                        wk->resources[DRAWING_RES_MAIN].cellAnims, &setup, 0, HEAPID_DRAWING);
        ClActor_SetAutoAnim(wk->cursors[i], TRUE);
        ClActor_SetSequence(wk->cursors[i], i);
        ClActor_SetVisible(wk->cursors[i], FALSE);
    }
    for (i = 0; i < DRAWING_BUTTON_COUNT; i++) {
        setup.x = sButtonData[i].x;
        setup.y = sButtonData[i].y;
        wk->buttons[i] = ClActor_Create(wk->actorUnit, wk->resources[DRAWING_RES_MAIN].chars,
                                        wk->resources[DRAWING_RES_MAIN].palette,
                                        wk->resources[DRAWING_RES_MAIN].cellAnims, &setup, 0, HEAPID_DRAWING);
        ClActor_SetAutoAnim(wk->buttons[i], TRUE);
        ClActor_SetSequence(wk->buttons[i], sButtonData[i].sequence);
        if (i >= DRAWING_BUTTON_END) {
            ClActor_SetBgPriority(wk->buttons[i], 2);
        }
    }
    ClActor_SetSequence(wk->buttons[0], 6);
    for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
        setup.x = 24;
        setup.y = i * 32 + 32 + DRAWING_SUB_SURFACE_Y;
        wk->nameActors[i] =
            ClActor_Create(wk->actorUnit, wk->resources[DRAWING_RES_SUB].chars, wk->resources[DRAWING_RES_SUB].palette,
                           wk->resources[DRAWING_RES_SUB].cellAnims, &setup, 1, HEAPID_DRAWING);
        ClActor_SetAutoAnim(wk->nameActors[i], TRUE);
        ClActor_SetSequence(wk->nameActors[i], i);
        ClActor_SetPriority(wk->nameActors[i], 1);
        ClActor_SetVisible(wk->nameActors[i], FALSE);
    }
    setup.x = 212;
    setup.y = 160;
    wk->endButton = ClActor_Create(wk->actorUnit, wk->resources[DRAWING_RES_COMMON].chars,
                                   wk->resources[DRAWING_RES_COMMON].palette,
                                   wk->resources[DRAWING_RES_COMMON].cellAnims, &setup, 0, HEAPID_DRAWING);
    ClActor_SetAutoAnim(wk->endButton, TRUE);
    ClActor_SetSequence(wk->endButton, 1);
    ClActor_SetVisible(wk->endButton, TRUE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void Drawing_CreateWindows(DrawingWork *wk, GameProc *proc) {
    int i;

    BmpWin_InitAllocator(HEAPID_DRAWING);
    wk->msgWindow = BmpWin_CreateDynamic(0, 2, 1, 27, 4, 13, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->msgWindow), 15);
    PrintWindow_Init(&wk->msgPrint, wk->msgWindow);
    wk->drawWindow = BmpWin_CreateDynamic(1, 1, 2, 30, 15, 0, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->drawWindow), 2);
    wk->tcbManager = GFL_TCBExMgrCreate(HEAPID_DRAWING, HEAPID_DRAWING, 32, 32);
    wk->nameQueue = PrintQueue_CreateDefault(HEAPID_DRAWING);
    wk->printQueue = PrintQueue_CreateDefault(HEAPID_DRAWING);
    for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
        wk->nameWindows[i] = BmpWin_CreateDynamic(4, 5, i * 4 + 3, 10, 2, 13, TRUE);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->nameWindows[i]), 0);
        PrintWindow_Init(&wk->namePrint[i], wk->nameWindows[i]);
        BmpWin_TransferNow(wk->nameWindows[i]);
    }
    Drawing_PrintNames(wk->nameWindows, DRAWING_COLOR_TEXT, wk);
}

static void Drawing_FreeWindows(DrawingWork *wk) {
    int i;

    GFL_TCBExMgrFreeTasks(wk->tcbManager);
    GFL_TCBExMgrFree(wk->tcbManager);
    PrintQueue_Clear(wk->printQueue);
    PrintQueue_Free(wk->printQueue);
    PrintQueue_Clear(wk->nameQueue);
    PrintQueue_Free(wk->nameQueue);
    for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
        BmpWin_Free(wk->nameWindows[i]);
    }
    BmpWin_Free(wk->drawWindow);
    BmpWin_Free(wk->msgWindow);
    BmpWin_FreeAllocator();
}

// Puts a pen's tip at a point
static void Drawing_SetCursorPos(ClActor *actor, int x, int y) {
    ClActorPos pos;

    pos.x = x;
    pos.y = y - 8;
    ClActor_SetPos(actor, &pos);
}

// The touch of a frame in which the pen is up
static void Drawing_MakeEmptyTouch(DrawingTouchInfo *info, int color, int brush) {
    DrawingTouchData touchData;

    touchData.size = 0;
    touchData.points[0].x = 0;
    touchData.points[0].y = 0;
    Drawing_MakeTouchInfo(info, &touchData, color, brush);
}

// The picture
static const TouchRect sDrawAreaRects[] = {
    { 16, 136, 8, 248 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

// The buttons, in the order of DRAWING_BUTTON_*
static const TouchRect sDrawingButtonRects[] = {
    { 150, 189, 0, 23 },         { 150, 189, 24, 47 },   { 150, 189, 48, 71 },   { 150, 189, 72, 95 },
    { 150, 189, 96, 119 },       { 150, 189, 120, 143 }, { 150, 189, 144, 167 }, { 150, 189, 168, 191 },
    { 150, 189, 192, 255 },      { 1, 13, 14, 26 },      { 1, 13, 30, 42 },      { 1, 13, 46, 58 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

// Handles the buttons, and queues what the player draws this frame
static void Drawing_UpdateTouch(DrawingWork *wk) {
    DrawingTouchData touchData;
    u32 cursorX;
    u32 cursorY;
    DrawingTouchInfo info;
    u32 x;
    u32 y;
    BOOL endPressed = FALSE;
    int button;
    int drawArea;

    button = GFL_HIDGetTouchPressedRect(sDrawingButtonRects);
    if (button != TOUCH_RECT_NONE) {
        switch (button) {
        case DRAWING_BUTTON_COLOR:
        case DRAWING_BUTTON_COLOR + 1:
        case DRAWING_BUTTON_COLOR + 2:
        case DRAWING_BUTTON_COLOR + 3:
        case DRAWING_BUTTON_COLOR + 4:
        case DRAWING_BUTTON_COLOR + 5:
        case DRAWING_BUTTON_COLOR + 6:
        case DRAWING_BUTTON_COLOR + 7:
            if (wk->color != button) {
                wk->color = button;
                Drawing_SetColorButtons(wk->buttons, button);
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
            break;
        case DRAWING_BUTTON_END:
            if (wk->seq == DRAWING_SEQ_MAIN) {
                if (NetSystem_GetNetId() == 0) {
                    // Not while a member that joined has yet to get the picture
                    if (wk->shareBit != Drawing_GetJoined(wk)) {
                        GFL_SndSEPlay(SEQ_SE_SELECT4);
                    } else {
                        wk->banFlag = DRAWING_BAN_ON;
                        Drawing_PrintMessage(wk, EventMapchange_Text_WouldLikeStopDrawing, TRUE);
                        endPressed = TRUE;
                        Drawing_SetNextSeq(wk, DRAWING_SEQ_END_SELECT_MSG_WAIT);
                        Drawing_SetEndButton(wk->endButton, TRUE);
                        GFL_SndSEPlay(SEQ_SE_CANCEL1);
                    }
                } else {
                    // Nor while the parent has its menu open
                    if (wk->allTouch[0].banFlag == DRAWING_BAN_ON) {
                        GFL_SndSEPlay(SEQ_SE_SELECT4);
                    } else {
                        Drawing_PrintMessage(wk, EventMapchange_Text_WouldLikeStopDrawing, TRUE);
                        endPressed = TRUE;
                        Drawing_SetNextSeq(wk, DRAWING_SEQ_END_SELECT_MSG_WAIT);
                        Drawing_SetEndButton(wk->endButton, TRUE);
                        GFL_SndSEPlay(SEQ_SE_CANCEL1);
                    }
                }
            }
            break;
        case DRAWING_BUTTON_BRUSH:
        case DRAWING_BUTTON_BRUSH + 1:
        case DRAWING_BUTTON_BRUSH + 2: {
            int i;

            for (i = 0; i < DRAWING_BRUSH_COUNT; i++) {
                if (button - DRAWING_BUTTON_BRUSH == i) {
                    ClActor_SetSequence(wk->buttons[DRAWING_BUTTON_BRUSH + i], i * 2 + 30);
                } else {
                    ClActor_SetSequence(wk->buttons[DRAWING_BUTTON_BRUSH + i], i * 2 + 29);
                }
            }
            if (wk->brush != button - DRAWING_BUTTON_BRUSH) {
                wk->brush = button - DRAWING_BUTTON_BRUSH;
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
            break;
        }
        }
    }

    // The player's own pen
    drawArea = GFL_HIDGetTouchHeldRect(sDrawAreaRects);
    ClActor_SetVisible(wk->cursors[NetSystem_GetNetId()], FALSE);
    if (drawArea != TOUCH_RECT_NONE) {
        GFL_HIDGetTouchHeldPoint(&cursorX, &cursorY);
        Drawing_SetCursorPos(wk->cursors[NetSystem_GetNetId()], cursorX, cursorY);
        ClActor_SetVisible(wk->cursors[NetSystem_GetNetId()], TRUE);
    }

    x = 0;
    y = 0;
    if (GFL_HIDIsTouchHeld() == TRUE) {
        // A line from the point of the frame before
        GFL_HIDGetTouchHeldPoint(&x, &y);
        touchData.size = 2;
        touchData.points[0].x = wk->touchData.points[1].x;
        touchData.points[0].y = wk->touchData.points[1].y;
        touchData.points[1].x = x;
        touchData.points[1].y = y;
        Drawing_MakeTouchInfo(&info, &touchData, wk->color, wk->brush);
        if (endPressed == TRUE) {
            info.size = 0;
        }
    } else {
        touchData.points[1].x = 0;
        touchData.points[1].y = 0;
        Drawing_MakeEmptyTouch(&info, wk->color, wk->brush);
    }
    Drawing_QueuePush(&info, &wk->queue, 0);
    if (NetSystem_GetNetId() == 0) {
        Drawing_QueueAdvanceParent(&wk->queue);
    } else {
        Drawing_QueueAdvanceChild(&wk->queue);
    }
    wk->touchData = touchData;
}

// Shows the picked color's button as pressed
static void Drawing_SetColorButtons(ClActor **buttons, int color) {
    int i;

    for (i = 0; i < DRAWING_COLOR_COUNT; i++) {
        if (i == color) {
            ClActor_SetSequence(buttons[i], sButtonData[i].sequence + 1);
        } else {
            ClActor_SetSequence(buttons[i], sButtonData[i].sequence);
        }
    }
}

static void Drawing_SetEndButton(ClActor *actor, BOOL pressed) {
    if (pressed == TRUE) {
        ClActor_SetSequence(actor, 9);
    } else {
        ClActor_SetSequence(actor, 1);
    }
}

static int Drawing_SeqMain(DrawingWork *wk, int seq) {
    Drawing_UpdateTouch(wk);
    if (NetSystem_GetNetId() == 0) {
        // A parent with no children has nobody to send to
        if (Drawing_CountMembers(wk) != 1) {
            Drawing_SendTouch(wk);
        }
    } else {
        Drawing_SendTouch(wk);
    }
    Drawing_UpdateCursors(wk);
    Drawing_DrawTouches(wk->drawWindow, wk->allTouch, wk->oldTouch, TRUE);
    return seq;
}

// Draws what has arrived while the player cannot draw
static void Drawing_DrawOthers(DrawingWork *wk) {
    Drawing_UpdateCursors(wk);
    Drawing_DrawTouches(wk->drawWindow, wk->allTouch, wk->oldTouch, FALSE);
}

static int Drawing_SeqNewcomer(DrawingWork *wk, int seq) {
    if (wk->newcomerId == NetSystem_GetNetId()) {
        Drawing_PrintMessage(wk, EventMapchange_Text_JoinedDrawingSession, FALSE);
    } else {
        Drawing_PrintMessage(wk, EventMapchange_Text_HasJoined, FALSE);
    }
    Drawing_SetNextSeq(wk, DRAWING_SEQ_NEWCOMER_WAIT);
    Drawing_CloseMenu(wk);
    GFL_SndSEPlay(SEQ_SE_DECIDE1);
    G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3,
                            -6);
    Drawing_DrawOthers(wk);
    return seq;
}

// Waits for DRAWING_NET_CMD_PICTURE_END
static int Drawing_SeqNewcomerWait(DrawingWork *wk, int seq) {
    int i;

    for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
        ClActor_SetVisible(wk->cursors[i], FALSE);
    }
    return seq;
}

static int Drawing_SeqNewcomerEnd(DrawingWork *wk, int seq) {
    reg_G2_BLDCNT = 0;
    Drawing_SetEntry(wk, 1);
    Drawing_SetNextSeq(wk, DRAWING_SEQ_MAIN);
    Drawing_DrawOthers(wk);
    wk->joining = FALSE;
    return seq;
}

static int Drawing_SeqEndSelectMsgWait(DrawingWork *wk, int seq) {
    if (Drawing_IsMessageDone(wk)) {
        if (wk->menu == NULL) {
            wk->menu = Drawing_CreateMenu(wk);
        }
        Drawing_SetNextSeq(wk, DRAWING_SEQ_END_SELECT);
    }
    Drawing_DrawOthers(wk);
    return seq;
}

static int Drawing_SeqEndSelect(DrawingWork *wk, int seq) {
    wk->myTouch.size = 0;
    // The parent has its own menu open
    if (wk->allTouch[0].banFlag == DRAWING_BAN_ON && NetSystem_GetNetId() != 0) {
        Drawing_DrawOthers(wk);
        AppTaskMenu_UpdateText(wk->menu);
        if (Drawing_IsMenuAreaTouched(wk)) {
            GFL_SndSEPlay(SEQ_SE_SELECT4);
        }
        return seq;
    }
    if (NetSystem_GetNetId() == 0 && wk->leavingBits != 0) {
        Drawing_DrawOthers(wk);
        AppTaskMenu_UpdateText(wk->menu);
        return seq;
    }
    switch (Drawing_UpdateMenu(wk)) {
    case 0:
        if (NetSystem_GetNetId() == 0) {
            Drawing_SetNextSeq(wk, DRAWING_SEQ_PARENT_END_SELECT_MSG_WAIT);
            Drawing_PrintMessage(wk, EventMapchange_Text_IfLeaderDropsOut, TRUE);
        } else {
            Drawing_SetNextSeq(wk, DRAWING_SEQ_END_REQUEST);
            BmpWin_ClearFrame(wk->msgWindow, 2);
        }
        BmpWin_TransferNow(wk->drawWindow);
        break;
    case 1:
        Drawing_SetNextSeq(wk, DRAWING_SEQ_MAIN);
        Drawing_SetEndButton(wk->endButton, FALSE);
        BmpWin_ClearFrame(wk->msgWindow, 2);
        BmpWin_TransferNow(wk->drawWindow);
        if (NetSystem_GetNetId() == 0) {
            DrawingComm_LimitEntry(wk, 1);
            wk->banFlag = DRAWING_BAN_NONE;
        }
        break;
    }
    Drawing_DrawOthers(wk);
    return seq;
}

// Asks the parent to leave
static int Drawing_SeqEndRequest(DrawingWork *wk, int seq) {
    DrawingSignal signal;

    MI_CpuFill8(&signal, 0, sizeof(signal));
    signal.type = DRAWING_SIGNAL_LEAVE_REQUEST;
    signal.netId = NetSystem_GetNetId();
    if (GFL_NetSendCommand(NetHandle_GetOwn(), DRAWING_NET_CMD_SIGNAL, sizeof(signal), &signal)) {
        Drawing_SetNextSeq(wk, DRAWING_SEQ_END_ANSWER_WAIT);
    }
    Drawing_DrawOthers(wk);
    return seq;
}

static int Drawing_SeqEndAnswerWait(DrawingWork *wk, int seq) {
    wk->myTouch.size = 0;
    wk->signalWait = 0;
    Drawing_DrawOthers(wk);
    return seq;
}

static int Drawing_SeqEndOk(DrawingWork *wk, int seq) {
    wk->myTouch.size = 0;
    wk->signalWait++;
    if (wk->signalWait > 30) {
        Drawing_SetNextSeq(wk, DRAWING_SEQ_END_LEAVE);
        wk->signalWait = 0;
    }
    Drawing_DrawOthers(wk);
    return seq;
}

static int Drawing_SeqEndLeave(DrawingWork *wk, int seq) {
    DrawingSignal signal;

    MI_CpuFill8(&signal, 0, sizeof(signal));
    signal.type = DRAWING_SIGNAL_LEAVE;
    signal.netId = NetSystem_GetNetId();
    if (GFL_NetSendCommand(NetHandle_GetOwn(), DRAWING_NET_CMD_SIGNAL, sizeof(signal), &signal)) {
        Drawing_SetNextSeq(wk, DRAWING_SEQ_END_MSG);
    }
    Drawing_DrawOthers(wk);
    return seq;
}

static int Drawing_SeqEndRefused(DrawingWork *wk, int seq) {
    wk->myTouch.size = 0;
    wk->leaving = FALSE;
    Drawing_SetNextSeq(wk, DRAWING_SEQ_MAIN);
    Drawing_SetEndButton(wk->endButton, FALSE);
    Drawing_DrawOthers(wk);
    return seq;
}

static int Drawing_SeqEndMsg(DrawingWork *wk, int seq) {
    Drawing_PrintMessage(wk, EventMapchange_Text_DrawingSessionEnded, TRUE);
    Drawing_SetNextSeq(wk, DRAWING_SEQ_END_MSG_WAIT);
    Drawing_DrawOthers(wk);
    return seq;
}

static int Drawing_SeqEndMsgWait(DrawingWork *wk, int seq) {
    if (Drawing_IsMessageDone(wk)) {
        wk->wait = 0;
        Drawing_SetNextSeq(wk, DRAWING_SEQ_END_WAIT);
    }
    Drawing_DrawOthers(wk);
    return seq;
}

static int Drawing_SeqEndWait(DrawingWork *wk, int seq) {
    wk->wait++;
    if (wk->wait > 60) {
        GFL_WipeSet(WIPE_MODE_BOTH, 8, 8, WIPE_COLOR_BLACK, 16, 1, HEAPID_DRAWING);
        seq = DRAWING_PROC_SEQ_FADE_OUT;
    }
    Drawing_DrawOthers(wk);
    return seq;
}

static int Drawing_SeqParentEndSelectMsgWait(DrawingWork *wk, int seq) {
    if (Drawing_IsMessageDone(wk)) {
        if (wk->menu == NULL) {
            wk->menu = Drawing_CreateMenu(wk);
        }
        Drawing_SetNextSeq(wk, DRAWING_SEQ_PARENT_END_SELECT);
    }
    Drawing_DrawOthers(wk);
    return seq;
}

static int Drawing_SeqParentEndSelect(DrawingWork *wk, int seq) {
    int result = Drawing_UpdateMenu(wk);

    if (wk->shareNum != Drawing_GetMemberCount(wk) || wk->leavingBits != 0) {
        Drawing_DrawOthers(wk);
        return seq;
    }
    switch (result) {
    case 0:
        Drawing_SetNextSeq(wk, DRAWING_SEQ_PARENT_END_SEND);
        UnionApp_CloseEntry(wk->param->app);
        break;
    case 1:
        Drawing_SetNextSeq(wk, DRAWING_SEQ_MAIN);
        Drawing_SetEndButton(wk->endButton, FALSE);
        BmpWin_ClearFrame(wk->msgWindow, 2);
        if (NetSystem_GetNetId() == 0) {
            DrawingComm_LimitEntry(wk, 1);
            wk->banFlag = DRAWING_BAN_NONE;
        }
        break;
    }
    Drawing_DrawOthers(wk);
    return seq;
}

// Puts a member's name in a slot of the messages
static void Drawing_SetPlayerName(DrawingWork *wk, int index, int netId) {
    UnionAppMember *member = UnionApp_GetMember(wk->param->app, netId);

    if (member != NULL) {
        copyVarForText(wk->wordSet, index, &member->info);
    }
}

static int Drawing_SeqParentEndSend(DrawingWork *wk, int seq) {
    if (GFL_NetSendCommand(NetHandle_Get(GFL_NET_NETID_SERVER), DRAWING_NET_CMD_PARENT_END, 0, NULL)) {
        Drawing_SetNextSeq(wk, DRAWING_SEQ_PARENT_END_MSG);
        Drawing_SetPlayerName(wk, 0, 0);
        seq = DRAWING_PROC_SEQ_END;
    }
    Drawing_DrawOthers(wk);
    return seq;
}

static int Drawing_SeqParentEndMsg(DrawingWork *wk, int seq) {
    Drawing_SetPlayerName(wk, 0, 0);
    Drawing_PrintMessage(wk, EventMapchange_Text_LeaderHasLeftGroup, TRUE);
    Drawing_SetNextSeq(wk, DRAWING_SEQ_PARENT_END_MSG_WAIT);
    Drawing_DrawOthers(wk);
    return seq;
}

static int Drawing_SeqParentEndMsgWait(DrawingWork *wk, int seq) {
    if (Drawing_IsMessageDone(wk)) {
        Drawing_SetNextSeq(wk, DRAWING_SEQ_PARENT_END_SYNC_WAIT);
        NetHandle_SendTimingPairToMachines(NetHandle_GetOwn(), 200, 0x29, (u16)wk->shareBit);
    }
    Drawing_DrawOthers(wk);
    return seq;
}

// Waits until every member is at the same point, or gone
static int Drawing_SeqParentEndSyncWait(DrawingWork *wk, int seq) {
    NetHandle *handle = NetHandle_GetOwn();
    int i;
    u32 bits = 0;

    for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
        if (GFL_NetIsNegotiated((u8)i) == TRUE) {
            bits |= 1 << i;
        }
    }
    if (NetHandle_IsTimingPairRecv(handle, 200, 0x29) || Drawing_CountJoined(wk) == 1 || bits != wk->shareBit) {
        GFL_WipeSet(WIPE_MODE_BOTH, 8, 8, WIPE_COLOR_BLACK, 16, 1, HEAPID_DRAWING);
        seq = DRAWING_PROC_SEQ_FADE_OUT;
    }
    Drawing_DrawOthers(wk);
    return seq;
}

static int Drawing_Seq21(DrawingWork *wk, int seq) {
    return seq;
}

static int Drawing_SeqNoMembersMsg(DrawingWork *wk, int seq) {
    Drawing_PrintMessage(wk, EventMapchange_Text_ThereNoMembersGroup, TRUE);
    Drawing_SetNextSeq(wk, DRAWING_SEQ_NO_MEMBERS_MSG_WAIT);
    Drawing_DrawOthers(wk);
    return seq;
}

static int Drawing_SeqNoMembersMsgWait(DrawingWork *wk, int seq) {
    if (Drawing_IsMessageDone(wk)) {
        Drawing_SetNextSeq(wk, DRAWING_SEQ_END_WAIT);
    }
    Drawing_DrawOthers(wk);
    return seq;
}

static int Drawing_SeqMemberLeft(DrawingWork *wk, int seq) {
    Drawing_PrintMessage(wk, EventMapchange_Text_HasLeft, TRUE);
    Drawing_CloseMenu(wk);
    Drawing_SetNextSeq(wk, DRAWING_SEQ_MEMBER_LEFT_MSG_WAIT);
    GFL_SndSEPlay(SEQ_SE_DECIDE1);
    if (NetSystem_GetNetId() == 0) {
        Drawing_SetEntry(wk, 0);
    }
    wk->leftCount = Drawing_CountJoined(wk);
    Drawing_DrawOthers(wk);
    return seq;
}

static int Drawing_SeqMemberLeftMsgWait(DrawingWork *wk, int seq) {
    if ((wk->leftCount != 0 && wk->leftCount != Drawing_CountJoined(wk)) ||
        !(Drawing_GetJoined(wk) & wk->leavingBits)) {
        wk->leftCount = 0;
    }
    if (Drawing_IsMessageDone(wk)) {
        Drawing_SetNextSeq(wk, DRAWING_SEQ_MEMBER_LEFT_WAIT);
        wk->wait = 0;
    }
    Drawing_DrawOthers(wk);
    return seq;
}

static int Drawing_SeqMemberLeftWait(DrawingWork *wk, int seq) {
    if ((wk->leftCount != 0 && wk->leftCount != Drawing_CountJoined(wk)) ||
        !(Drawing_GetJoined(wk) & wk->leavingBits)) {
        wk->leftCount = 0;
    }
    wk->wait++;
    if (wk->wait > 60 && wk->leftCount == 0) {
        Drawing_ClearMessage(wk);
        Drawing_SetNextSeq(wk, DRAWING_SEQ_MAIN);
        if (NetSystem_GetNetId() == 0) {
            wk->banFlag = DRAWING_BAN_NONE;
            Drawing_SetEntry(wk, 1);
        }
        wk->shareNum = UnionApp_CountJoined(wk->param->app);
        wk->shareBit = UnionApp_GetJoined(wk->param->app);
    }
    Drawing_DrawOthers(wk);
    return seq;
}

void Drawing_SetStatus(DrawingWork *wk, int seq, int netId) {
    switch (seq) {
    case DRAWING_SEQ_NEWCOMER:
        if (wk->leaving == TRUE) {
            return;
        }
        if (wk->seq == DRAWING_SEQ_END_SELECT || wk->seq == DRAWING_SEQ_PARENT_END_SELECT) {
            Drawing_CloseMenu(wk);
        }
        Drawing_SetEndButton(wk->endButton, FALSE);
        Drawing_SetPlayerName(wk, 0, netId);
        wk->newcomerId = netId;
        wk->leavingBits = 0;
        break;
    case DRAWING_SEQ_NEWCOMER_END:
        Drawing_ClearMessage(wk);
        break;
    case DRAWING_SEQ_MEMBER_LEFT:
        if (wk->leaving == TRUE) {
            return;
        }
        Drawing_SetPlayerName(wk, 0, netId);
        if (netId == NetSystem_GetNetId()) {
            return;
        }
        if (NetSystem_GetNetId() == 0) {
            wk->leavingBits &= 0xffff ^ netId;
        }
        if (wk->seq == DRAWING_SEQ_END_SELECT || wk->seq == DRAWING_SEQ_PARENT_END_SELECT) {
            Drawing_CloseMenu(wk);
            BmpWin_TransferNow(wk->drawWindow);
        }
        Drawing_SetEndButton(wk->endButton, FALSE);
        break;
    case DRAWING_SEQ_END_OK:
        Drawing_RequestSend(wk, DRAWING_NET_CMD_LEAVING, NetSystem_GetNetId());
        break;
    case DRAWING_SEQ_END_REFUSED:
    case DRAWING_SEQ_PARENT_END_MSG:
        break;
    default:
        return;
    }
    Drawing_SetNextSeq(wk, seq);
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
static const u8 sBrushPatterns[DRAWING_BRUSH_COUNT][DRAWING_COLOR_COUNT][DRAWING_BRUSH_SIZE * 4] = {
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
static void Drawing_DrawBrush(BmpWin *window, const u8 *brush, int srcX, int srcY, int width, int height, int x, int y,
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
    bitmap = GFL_BitmapWrap((void *)brush, 8, 8, 0x20, HEAPID_DRAWING);
    GFL_BitmapCopyArea(bitmap, BmpWin_GetBitmap(window), srcX, srcY, x, y, width, height, 0);
    GFL_BitmapFree(bitmap);
}

// Draws a line with a brush from *oldX, *oldY to x, y, and makes that the old point
static void Drawing_DrawLine(BmpWin *window, const u8 *brush, int x, int y, int *oldX, int *oldY, int penDown,
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
        Drawing_DrawBrush(window, brush, 0, 0, DRAWING_BRUSH_SIZE, DRAWING_BRUSH_SIZE, x0, y0, DRAWING_BRUSH_SIZE,
                          DRAWING_BRUSH_SIZE);
        error = dx >> 1;
        for (x0++; x0 <= xEnd; x0++) {
            error -= dy;
            if (error < 0) {
                error += dx;
                y0 += step;
            }
            Drawing_DrawBrush(window, brush, 0, 0, DRAWING_BRUSH_SIZE, DRAWING_BRUSH_SIZE, x0, y0, DRAWING_BRUSH_SIZE,
                              DRAWING_BRUSH_SIZE);
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
        Drawing_DrawBrush(window, brush, 0, 0, DRAWING_BRUSH_SIZE, DRAWING_BRUSH_SIZE, x0, y0, DRAWING_BRUSH_SIZE,
                          DRAWING_BRUSH_SIZE);
        error = dy >> 1;
        for (y0++; y0 <= yEnd; y0++) {
            error -= dx;
            if (error < 0) {
                error += dy;
                x0 += step;
            }
            Drawing_DrawBrush(window, brush, 0, 0, DRAWING_BRUSH_SIZE, DRAWING_BRUSH_SIZE, x0, y0, DRAWING_BRUSH_SIZE,
                              DRAWING_BRUSH_SIZE);
        }
    }
    *oldX = x;
    *oldY = y;
}

// Keeps where each member's line ended
static void Drawing_SaveOldTouch(DrawingTouchInfo *touch, DrawingOldTouch *old) {
    int i;

    for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
        old[i].size = touch[i].size;
        if (touch[i].size != 0) {
            old[i].x = touch[i].x[touch[i].size - 1];
            old[i].y = touch[i].y[touch[i].size - 1];
        }
    }
}

// Draws every member's line of the frame, and clears the touches
static void Drawing_DrawTouches(BmpWin *window, DrawingTouchInfo *touch, DrawingOldTouch *old, BOOL transfer) {
    int i;
    BOOL drawn = FALSE;
    int x;
    int y;

    for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
        if (touch[i].size == 2) {
            x = touch[i].x[0] - 9;
            y = touch[i].y[0] - 17;
            Drawing_DrawLine(window, sBrushPatterns[touch[i].brush][touch[i].color], touch[i].x[1] - 9,
                             touch[i].y[1] - 17, &x, &y, TRUE, old[i].size);
            drawn = TRUE;
        }
    }
    if (drawn && transfer) {
        BmpWin_TransferNow(window);
    }
    Drawing_SaveOldTouch(touch, old);
    for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
        touch[i].size = 0;
    }
}

// Shows the other members' pens where they draw
static void Drawing_UpdateCursors(DrawingWork *wk) {
    DrawingTouchInfo *touch = wk->allTouch;
    int i;

    for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
        if (i != NetSystem_GetNetId()) {
            if (touch[i].size != 0 && UnionApp_GetMember(wk->param->app, i) != NULL) {
                ClActor_SetVisible(wk->cursors[i], TRUE);
                Drawing_SetCursorPos(wk->cursors[i], touch[i].x[touch[i].size - 1], touch[i].y[touch[i].size - 1]);
            } else {
                ClActor_SetVisible(wk->cursors[i], FALSE);
            }
        }
    }
}

// Pulses the green of a color of the actors' palette
static void Drawing_UpdatePalette(u16 *angle) {
    u16 color;

    *angle += 20;
    if (*angle > 360) {
        *angle = 0;
    }
    color = GX_RGB(29, GFL_MathSinDegWrap(*angle) * 10 / FX16_ONE + 15, 0);
    GX_LoadOBJPltt(&color, 12 * sizeof(u16), sizeof(u16));
}

static int Drawing_CountMembers(DrawingWork *wk) {
    int i;
    int count = 0;

    for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
        if (UnionApp_GetMember(wk->param->app, i) != NULL) {
            count++;
        }
    }
    return count;
}

// Prints the members' names on the top screen when the members have changed
static void Drawing_PrintNames(BmpWin **windows, u16 color, DrawingWork *wk) {
    int myId = NetSystem_GetNetId();
    int i;

    if (!Drawing_UpdateMembers(wk)) {
        return;
    }
    for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
        if (wk->members[i].member == NULL) {
            ClActor_SetVisible(wk->nameActors[i], FALSE);
        } else {
            ClActor_SetVisible(wk->nameActors[i], TRUE);
        }
    }
    PrintQueue_Clear(wk->nameQueue);
    for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
        GFL_BitmapFillArea(BmpWin_GetBitmap(windows[i]), 0, 0, 80, 16, 0);
    }
    for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
        if (wk->members[i].member != NULL) {
            textCopy(wk->members[i].member->info.name, wk->nameBufs[i]);
            if (myId == i) {
                PrintWindow_Print(&wk->namePrint[i], wk->nameQueue, 0, 0, wk->nameBufs[i], wk->font,
                                  DRAWING_COLOR_NAME_MINE);
            } else {
                PrintWindow_Print(&wk->namePrint[i], wk->nameQueue, 0, 0, wk->nameBufs[i], wk->font, color);
            }
        }
        BmpWin_TransferNow(windows[i]);
    }
}

// Sends the oldest frame of the queue: the parent every member's touch, a child its own
static void Drawing_SendTouch(DrawingWork *wk) {
    NetHandle *handle;

    if (NetSystem_GetNetId() == 0) {
        handle = NetHandle_GetOwn();
        if (GFL_NetIsSendQueueEmpty(handle)) {
            Drawing_QueuePopAll(wk->sendTouch, &wk->queue);
            wk->sendTouch[0].banFlag = wk->banFlag;
            if (GFL_NetSendCommand(handle, DRAWING_NET_CMD_ALL_TOUCH, sizeof(wk->sendTouch), wk->sendTouch) == TRUE) {
                Drawing_QueueNext(&wk->queue);
            }
        }
    } else {
        handle = NetHandle_GetOwn();
        Drawing_QueuePop(&wk->myTouch, &wk->queue);
        if (GFL_NetIsSendQueueEmpty(handle)) {
            if (GFL_NetSendCommand(NetHandle_GetOwn(), DRAWING_NET_CMD_TOUCH, sizeof(wk->myTouch), &wk->myTouch) ==
                TRUE) {
                Drawing_QueueNext(&wk->queue);
            }
        }
    }
}

static int Drawing_GetMemberCount(DrawingWork *wk) {
    return UnionApp_CountJoined(wk->param->app);
}

// Reads the members, and returns whether any has changed
static BOOL Drawing_UpdateMembers(DrawingWork *wk) {
    BOOL changed = FALSE;
    int i;

    for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
        wk->members[i].old = wk->members[i].member;
        wk->members[i].member = UnionApp_GetMember(wk->param->app, i);
    }
    for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
        if (wk->members[i].old != wk->members[i].member) {
            changed = TRUE;
        }
    }
    return changed;
}

// Prints a message in the message window, at once or a character at a time
static void Drawing_PrintMessage(DrawingWork *wk, int messageId, BOOL stream) {
    StrBuf *format = GFL_StrBufCreate(80, HEAPID_DRAWING);

    GFL_MsgDataLoadStrbuf(wk->msgData, messageId, format);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->msgBuf, format);
    GFL_StrBufFree(format);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->msgWindow), 15);
    BmpWin_DrawFrame(wk->msgWindow, FALSE, 1, 10);
    if (wk->stream != NULL) {
        PrintStream_Delete(wk->stream);
        wk->stream = NULL;
    }
    if (!stream) {
        PrintWindow_Print(&wk->msgPrint, wk->printQueue, 0, 0, wk->msgBuf, wk->font, DRAWING_COLOR_TEXT);
    } else {
        wk->stream = PrintStream_Create(wk->msgWindow, 0, 0, wk->msgBuf, wk->font, TextSpeed_GetCurrentWait(),
                                        wk->tcbManager, 1, HEAPID_DRAWING, 0xf0f);
    }
    BmpWin_TransferNow(wk->msgWindow);
}

static BOOL Drawing_IsMessageDone(DrawingWork *wk) {
    u32 state = PrintStream_GetState(wk->stream);

    if (state == PRINT_STREAM_PAUSED) {
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            PrintStream_Continue(wk->stream);
        }
        return FALSE;
    }
    if (state == PRINT_STREAM_DONE) {
        PrintStream_Delete(wk->stream);
        wk->stream = NULL;
        return TRUE;
    }
    if (wk->stream == NULL) {
        return TRUE;
    }
    return FALSE;
}

static void Drawing_ClearMessage(DrawingWork *wk) {
    BmpWin_ClearFrame(wk->msgWindow, 0);
}

// The parent's: lets up to `limit` more players enter
static void Drawing_SetEntry(DrawingWork *wk, u8 limit) {
    if (NetSystem_GetNetId() == 0) {
        Drawing_CountJoined(wk);
        DrawingComm_LimitEntry(wk, limit);
    }
}

// The parent's update: follows the members that come and go, and returns the proc's next sequence
static int Drawing_UpdateParent(DrawingWork *wk) {
    int count = Drawing_GetMemberCount(wk);

    switch (count) {
    case 1:
        // Everyone else has left
        if (wk->seq <= DRAWING_SEQ_PARENT_END_SELECT) {
            if (Drawing_GetJoined(wk) != 1) {
                wk->waitAlone = TRUE;
            }
            wk->nextSeq = DRAWING_SEQ_NO_MEMBERS_MSG;
            if (wk->menuOpen) {
                Drawing_CloseMenu(wk);
                ClActor_SetSequence(wk->buttons[DRAWING_BUTTON_END], 21);
            }
            return DRAWING_PROC_SEQ_END;
        }
        break;
    case 2:
    case 3:
    case 4:
        if (count < wk->memberCount) {
            if (wk->banFlag == DRAWING_BAN_ON) {
                Drawing_SetEntry(wk, 0);
            } else if (!wk->joining) {
                Drawing_SetEntry(wk, 1);
            } else {
                Drawing_SetEntry(wk, 0);
            }
        }
        break;
    case 0:
    case 5:
        break;
    }
    if (count < wk->memberCount) {
        wk->shareNum = Drawing_CountJoined(wk);
        wk->shareBit = Drawing_GetJoined(wk);
        // The member that was joining is the one that left
        if (wk->joining && !(wk->shareBit & wk->joiningBits)) {
            Drawing_SetEntry(wk, 1);
            wk->banFlag = DRAWING_BAN_NONE;
            wk->joining = FALSE;
            wk->joiningBits = 0;
        }
    }
    wk->memberCount = Drawing_GetMemberCount(wk);
    if (wk->shareNum < Drawing_CountJoined(wk)) {
        wk->banFlag = DRAWING_BAN_ON;
        wk->joining = TRUE;
        wk->joiningBits = Drawing_GetJoined(wk) ^ wk->shareBit;
    }
    return DRAWING_PROC_SEQ_MAIN;
}

static void Drawing_SetNextSeq(DrawingWork *wk, int seq) {
    wk->nextSeq = seq;
}

// Changes to the next sequence, which while the proc ends has to be one that runs then
static void Drawing_UpdateSeq(DrawingWork *wk, int procSeq) {
    if (wk->seq != wk->nextSeq) {
        if (procSeq != DRAWING_PROC_SEQ_END) {
            wk->seq = wk->nextSeq;
        } else if (sSeqTable[wk->nextSeq].runsAtEnd) {
            wk->seq = wk->nextSeq;
        }
    }
}

static void Drawing_CloseMenu(DrawingWork *wk) {
    if (wk->menuOpen) {
        if (wk->menu != NULL) {
            Drawing_FreeMenu(wk);
        }
        wk->menuOpen = FALSE;
    }
}

// Where the menu comes up
static const TouchRect sMenuAreaRects[] = {
    { 48, 112, 200, 248 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static BOOL Drawing_IsMenuAreaTouched(DrawingWork *wk) {
    if (GFL_HIDGetTouchPressedRect(sMenuAreaRects) != TOUCH_RECT_NONE) {
        return TRUE;
    }
    return FALSE;
}

// Makes what is sent of the touch screen's points
static void Drawing_MakeTouchInfo(DrawingTouchInfo *info, DrawingTouchData *touchData, int color, int brush) {
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

static int Drawing_CountJoined(DrawingWork *wk) {
    return UnionApp_CountJoined(wk->param->app);
}

static u32 Drawing_GetJoined(DrawingWork *wk) {
    return UnionApp_GetJoined(wk->param->app);
}

static void Drawing_ShowScreens(void) {
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabled(0, TRUE);
    GFL_BGSysSetBGEnabled(1, TRUE);
    GFL_BGSysSetBGEnabled(2, TRUE);
    GFL_BGSysSetBGEnabled(4, TRUE);
    GFL_BGSysSetBGEnabled(5, TRUE);
    GX_SetDispSelect(GX_DISP_SELECT_SUB_MAIN);
}

static const DrawingMenuItem sDrawingMenuItems[] = {
    { EventMapchange_Text_Yes, 0 },
    { EventMapchange_Text_No, 0 },
};

// The Yes and No of the question whether to leave
static AppTaskMenu *Drawing_CreateMenu(DrawingWork *wk) {
    AppTaskMenuInit init;
    AppTaskMenu *menu;
    int i;

    wk->menuRes = AppTaskMenuRes_Create(0, 2, wk->font, wk->printQueue, HEAPID_DRAWING);
    for (i = 0; i < 2; i++) {
        wk->menuItems[i].str = GFL_MsgDataLoadStrbufNew(wk->msgData, sDrawingMenuItems[i].messageId);
        wk->menuItems[i].color = PRINT_COLOR(14, 15, 3);
        wk->menuItems[i].type = sDrawingMenuItems[i].type;
    }
    init.heapId = HEAPID_DRAWING;
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
static int Drawing_UpdateMenu(DrawingWork *wk) {
    int result = 2;

    AppTaskMenu_Update(wk->menu);
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        if (AppTaskMenu_GetCursorPos(wk->menu) == 0) {
            result = 0;
        } else {
            result = 1;
        }
        Drawing_FreeMenu(wk);
    }
    return result;
}

static void Drawing_FreeMenu(DrawingWork *wk) {
    AppTaskMenu_Free(wk->menu);
    GFL_StrBufFree(wk->menuItems[1].str);
    GFL_StrBufFree(wk->menuItems[0].str);
    AppTaskMenuRes_Free(wk->menuRes);
    wk->menu = NULL;
}

// Records the other members as trainers the player has met
static void Drawing_RecordMetTrainers(DrawingWork *wk) {
    RivalDataSave *rivalData = getHollow_RivalData(GameData_GetSaveControl(wk->param->gameData));
    int i;

    for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
        UnionAppMember *member = UnionApp_GetMember(wk->param->app, i);

        if (member != NULL && i != NetSystem_GetNetId()) {
            Misc_AddMetTrainer(rivalData, getIDAsUInt(&member->info));
        }
    }
}

static void Drawing_FlushPrint(DrawingWork *wk) {
    int i;

    PrintQueue_Main(wk->nameQueue);
    PrintQueue_Main(wk->printQueue);
    for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
        PrintWindow_Flush(&wk->namePrint[i], wk->nameQueue);
    }
    PrintWindow_Flush(&wk->msgPrint, wk->printQueue);
}

// Sends the command that waits, until it goes out
static void Drawing_SendRequest(DrawingWork *wk) {
    switch (wk->sendCommand) {
    case DRAWING_NET_CMD_NEWCOMER:
        if (GFL_NetSendCommand(NetHandle_Get(GFL_NET_NETID_SERVER), DRAWING_NET_CMD_NEWCOMER, 1, &wk->sendArg)) {
            wk->sendCommand = 0;
        }
        break;
    case DRAWING_NET_CMD_PICTURE_END:
        if (GFL_NetSendCommand(NetHandle_Get(GFL_NET_NETID_SERVER), DRAWING_NET_CMD_PICTURE_END, 0, NULL)) {
            wk->sendCommand = 0;
        }
        break;
    case DRAWING_NET_CMD_SIGNAL:
        if (GFL_NetSendCommand(NetHandle_Get(GFL_NET_NETID_SERVER), DRAWING_NET_CMD_SIGNAL, sizeof(DrawingSignal),
                               &wk->signal)) {
            wk->sendCommand = 0;
        }
        break;
    case DRAWING_NET_CMD_LEAVING:
        if (GFL_NetSendCommand(NetHandle_GetOwn(), DRAWING_NET_CMD_LEAVING, 0, NULL)) {
            wk->sendCommand = 0;
        }
        break;
    }
}

void Drawing_RequestSend(DrawingWork *wk, int command, int arg) {
    if (wk->sendCommand == 0) {
        wk->sendCommand = command;
        wk->sendArg = arg;
    }
}

// The frame before the one that is being filled
static int Drawing_QueuePrev(DrawingQueue *queue) {
    int index = queue->end - 1;

    if (index < 0) {
        index = DRAWING_QUEUE_SIZE - 1;
    }
    return index;
}

// A child's: goes on to the next frame, unless this one and the one before are both empty
static void Drawing_QueueAdvanceChild(DrawingQueue *queue) {
    if (queue->fifo[queue->end][0].size != 0 || queue->fifo[Drawing_QueuePrev(queue)][0].size != 0) {
        queue->end++;
        if (queue->end >= DRAWING_QUEUE_SIZE) {
            queue->end = 0;
        }
    }
}

// The parent's: the same for the touches of every member
static void Drawing_QueueAdvanceParent(DrawingQueue *queue) {
    int i;
    int empty = 0;
    int emptyPrev = 0;

    for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
        if (queue->fifo[queue->end][i].size == 0) {
            empty++;
        }
    }
    if (empty == DRAWING_MEMBER_MAX) {
        int prev = Drawing_QueuePrev(queue);

        for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
            if (queue->fifo[prev][i].size == 0) {
                emptyPrev++;
            }
        }
    }
    if (emptyPrev != DRAWING_MEMBER_MAX) {
        queue->end++;
        if (queue->end >= DRAWING_QUEUE_SIZE) {
            queue->end = 0;
        }
    }
}

static void Drawing_ClearTouchSizes(DrawingTouchInfo *touch) {
    int i;

    for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
        touch[i].size = 0;
    }
}

// Drops the oldest frame, once it has been sent
static void Drawing_QueueNext(DrawingQueue *queue) {
    if (queue->start != queue->end) {
        Drawing_ClearTouchSizes(queue->fifo[queue->start]);
        queue->start++;
        if (queue->start >= DRAWING_QUEUE_SIZE) {
            queue->start = 0;
        }
    }
}

void Drawing_QueuePush(DrawingTouchInfo *touch, DrawingQueue *queue, int netId) {
    queue->fifo[queue->end][netId] = *touch;
}

// A child's touch of the oldest frame, or an empty one
static void Drawing_QueuePop(DrawingTouchInfo *touch, DrawingQueue *queue) {
    if (queue->start != queue->end) {
        *touch = queue->fifo[queue->start][0];
    } else {
        touch->size = 0;
    }
}

// Every member's touch of the oldest frame, or empty ones
static void Drawing_QueuePopAll(DrawingTouchInfo *touch, DrawingQueue *queue) {
    int i;

    if (queue->start != queue->end) {
        for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
            touch[i] = queue->fifo[queue->start][i];
        }
    } else {
        for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
            touch[i].size = 0;
        }
    }
}
