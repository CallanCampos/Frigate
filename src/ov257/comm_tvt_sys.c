#include "app/comm_tvt/comm_tvt_sys.h"
#include "types.h"
#include "constants/arc.h"
#include "app/comm_tvt.h"
#include "app/comm_tvt/camera_system.h"
#include "app/comm_tvt/ctvt_call.h"
#include "app/comm_tvt/ctvt_camera.h"
#include "app/comm_tvt/ctvt_comm.h"
#include "app/comm_tvt/ctvt_draw.h"
#include "app/comm_tvt/ctvt_game.h"
#include "app/comm_tvt/ctvt_talk.h"
#include "app/comm_tvt/draw_system.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "field/field_sound.h"
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
#include "gfl/net_lower_data.h"
#include "gfl/proc.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/app_menu_common.h"
#include "system/app_taskmenu.h"
#include "system/bmp_winframe.h"
#include "system/dsi.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/time_icon.h"
#include "system/wipe.h"
#include "text/system/btl_server_flow.h"

// The Xtransceiver's system: the proc, the screens and resources the modes share, and the switch between the modes.
// The names of the members are printed over their video on the top screen, and a cursor points at the talker

#define COMM_TVT_MEMBERS 4
// The frames of the palette animation of the top screen's frame
#define COMM_TVT_PALETTE_FRAMES 10
#define COMM_TVT_OBJ_RESOURCES 11

#define ARC_COMM_TVT 170

enum {
    COMM_TVT_START_WAIT,
    COMM_TVT_START_CALL,
    COMM_TVT_START_ANSWER,
    COMM_TVT_START_EXISTING,
};

struct CommTvtWork {
    HeapID heapId;
    TCB *vblankTask;
    u32 mode;
    u32 nextMode;
    BOOL unk10;
    BOOL errorShown;
    u8 memberCount;
    u8 selfIndex;
    BOOL zoomed;
    BOOL unk20;
    BOOL unk24;
    BOOL unk28;
    BOOL unk2C;
    BOOL canExchangePhotos;
    BOOL unk34;
    u16 paletteAngle;
    u16 paletteFrame;
    u16 palettes[COMM_TVT_PALETTE_FRAMES][16];
    ArcTool *arc;
    // Palettes, then characters, then cells and animations
    u32 objResources[COMM_TVT_OBJ_RESOURCES];
    ClActUnit *clactUnit;
    ClActor *talkerCursor;
    // A bit for each member whose name is printed, and for each window that is to be copied
    u8 namesPrinted;
    u8 namesPending;
    BmpWin *nameWindows[COMM_TVT_MEMBERS];
    Font *font;
    MsgData *msgData;
    PrintQueue *printQueue;
    AppTaskMenuRes *taskMenuRes;
    TCBExManager *tcbEx;
    WaitIcon *waitIcon;
    CtvtCamera *camera;
    CtvtComm *comm;
    CtvtTalk *talk;
    CtvtDraw *draw;
    CtvtCall *call;
    CtvtGame *game;
    // The game frees the rest of the system while it runs
    BOOL inGame;
    DrawSystem *drawSystem;
    CommTvtParam *param;
};

static void CommTvt_Init(CommTvtWork *sys);
static void CommTvt_Resume(CommTvtWork *sys);
static void CommTvt_Free(CommTvtWork *sys);
static void CommTvt_Suspend(CommTvtWork *sys);
static void CommTvt_FreeSuspended(CommTvtWork *sys);
static BOOL CommTvt_Main(CommTvtWork *sys);
static void CommTvt_VBlank(TCB *tcb, void *data);
static void CommTvt_InitGraphics(CommTvtWork *sys);
static void CommTvt_FreeGraphics(CommTvtWork *sys);
static void CommTvt_CreateBG(const BGSetup *setup, u8 bg, u8 mode);
static void CommTvt_LoadResources(CommTvtWork *sys);
static void CommTvt_FreeResources(CommTvtWork *sys);
static void CommTvt_InitMessages(CommTvtWork *sys);
static void CommTvt_FreeMessages(CommTvtWork *sys);
static void CommTvt_ChangeMode(CommTvtWork *sys);
static void CommTvt_UpdateNames(CommTvtWork *sys);
static void CommTvt_PrintName(CommTvtWork *sys, const CtvtCommMemberInfo *info, BmpWin *window);
static BOOL CommTvt_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL CommTvt_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL CommTvt_ProcExit(GameProc *proc, u32 *state, void *param, void *work);

// Where the talker's cursor goes, by name window
static const u8 sTalkerCursorX[COMM_TVT_MEMBERS] = { 88, 88, 184, 184 };
static const u8 sTalkerCursorY[COMM_TVT_MEMBERS] = { 72, 200, 72, 200 };

static const BGSysLCDConfig sCommTvtLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_5, GX_BGMODE_0, GX_BG0_AS_2D };

// The video's bitmaps (BG 2 and BG 3) are made in CommTvt_InitGraphics
static const BGSetup sCommTvtBG7Setup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x7800),
    GX_BG_CHARBASE(0x08000),
    0x8000,
    GX_BG_EXTPLTT_23,
    3,
    GX_BG_AREAOVER_XLU,
    FALSE,
};
static const BGSetup sCommTvtBG6Setup = {
    0,
    0,
    0x1000,
    0,
    BGRES_256x512,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x6800),
    GX_BG_CHARBASE(0x10000),
    0x8000,
    GX_BG_EXTPLTT_23,
    2,
    GX_BG_AREAOVER_XLU,
    FALSE,
};
static const BGSetup sCommTvtBG0Setup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x7000),
    GX_BG_CHARBASE(0x00000),
    0x7000,
    GX_BG_EXTPLTT_23,
    0,
    GX_BG_AREAOVER_XLU,
    FALSE,
};
static const BGSetup sCommTvtBG4Setup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x5000),
    GX_BG_CHARBASE(0x18000),
    0x8000,
    GX_BG_EXTPLTT_23,
    0,
    GX_BG_AREAOVER_XLU,
    FALSE,
};
static const BGSetup sCommTvtBG5Setup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x6000),
    GX_BG_CHARBASE(0x00000),
    0x5000,
    GX_BG_EXTPLTT_23,
    1,
    GX_BG_AREAOVER_XLU,
    FALSE,
};
static const BGSetup sCommTvtBG1Setup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x7800),
    GX_BG_CHARBASE(0x08000),
    0x8000,
    GX_BG_EXTPLTT_23,
    3,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static const BGSysVRAMConfig sCommTvtVRAMConfig = {
    GX_VRAM_BG_256_AB, GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_16_F,  GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static inline void CommTvt_CreateNameWindows(CommTvtWork *sys) {
    u8 i;

    sys->nameWindows[0] = BmpWin_CreateDynamic(0, 2, 10, 12, 2, 10, TRUE);
    sys->nameWindows[1] = BmpWin_CreateDynamic(0, 18, 10, 12, 2, 10, TRUE);
    sys->nameWindows[2] = BmpWin_CreateDynamic(0, 2, 22, 12, 2, 10, TRUE);
    sys->nameWindows[3] = BmpWin_CreateDynamic(0, 18, 22, 12, 2, 10, TRUE);
    for (i = 0; i < COMM_TVT_MEMBERS; i++) {
        GFL_BitmapFill(BmpWin_GetBitmap(sys->nameWindows[i]), 0);
    }
    sys->namesPrinted = 0;
    sys->namesPending = 0;
}

static void CommTvt_Init(CommTvtWork *sys) {
    DrawSystemParam drawParam;

    sys->memberCount = 1;
    sys->selfIndex = 0;
    sys->zoomed = FALSE;
    CommTvt_InitGraphics(sys);
    CommTvt_LoadResources(sys);
    CommTvt_InitMessages(sys);
    sys->taskMenuRes = AppTaskMenuRes_Create(4, 7, sys->font, sys->printQueue, sys->heapId);
    sys->camera = CtvtCamera_Create(sys, sys->heapId);
    sys->comm = CtvtComm_Create(sys, sys->heapId);
    sys->talk = CtvtTalk_Create(sys, sys->heapId);
    sys->draw = CtvtDraw_Create(sys, sys->heapId);
    sys->call = CtvtCall_Create(sys, sys->heapId);
    sys->game = CtvtGame_Create(sys, sys->heapId);
    drawParam.heapId = sys->heapId;
    drawParam.unk2 = 2;
    drawParam.numCommands = 100;
    drawParam.clipLeft = 0;
    drawParam.clipRight = 256;
    drawParam.clipTop = 0;
    drawParam.clipBottom = 192;
    sys->drawSystem = DrawSystem_Create(&drawParam);
    sys->vblankTask = GFL_VBlankTCBAdd(CommTvt_VBlank, sys, 8);
    sys->mode = COMM_TVT_MODE_NONE;
    sys->paletteAngle = 0;
    sys->paletteFrame = 0;
    sys->unk10 = TRUE;
    sys->unk20 = FALSE;
    sys->unk24 = FALSE;
    sys->unk28 = FALSE;
    sys->unk2C = FALSE;
    sys->canExchangePhotos = FALSE;
    sys->unk34 = FALSE;
    sys->inGame = FALSE;
    if (canPlayerExchangePhotos() == TRUE) {
        sys->canExchangePhotos = TRUE;
    }
    func_02042ba8(FALSE, sys->heapId);

    switch (sys->param->unk4) {
    case COMM_TVT_START_WAIT:
        CtvtComm_SetNextConnectType(sys, sys->comm, CTVT_CONNECT_SCAN);
        sys->nextMode = COMM_TVT_MODE_TALK;
        break;
    case COMM_TVT_START_CALL:
        CtvtComm_SetNextConnectType(sys, sys->comm, CTVT_CONNECT_PARENT);
        sys->nextMode = COMM_TVT_MODE_CALL;
        break;
    case COMM_TVT_START_ANSWER:
        CtvtComm_SetNextConnectType(sys, sys->comm, CTVT_CONNECT_MAC);
        CtvtComm_SetParentMac(sys, sys->comm, sys->param->parentMac);
        sys->nextMode = COMM_TVT_MODE_CALL;
        FieldSnd_StopRingtone(GameData_GetFieldSoundSystem(sys->param->gameData));
        break;
    case COMM_TVT_START_EXISTING:
        CtvtComm_SetNextConnectType(sys, sys->comm, CTVT_CONNECT_EXISTING);
        sys->nextMode = COMM_TVT_MODE_CALL;
        break;
    }

    CommTvt_CreateNameWindows(sys);
    GFL_SndBGMPlay(SEQ_BGM_SILENCE_FIELD, 0xffff);
}

// Makes again what CommTvt_Suspend freed for the game
static void CommTvt_Resume(CommTvtWork *sys) {
    HeapID heapId = CommTvt_GetHeapId(sys);

    sys->zoomed = FALSE;
    CommTvt_InitGraphics(sys);
    CommTvt_LoadResources(sys);
    sys->waitIcon = NULL;
    sys->vblankTask = GFL_VBlankTCBAdd(CommTvt_VBlank, sys, 8);
    sys->paletteAngle = 0;
    sys->paletteFrame = 0;
    sys->unk10 = TRUE;
    sys->unk20 = FALSE;
    sys->unk24 = FALSE;
    sys->unk28 = FALSE;
    sys->unk2C = FALSE;
    sys->unk34 = FALSE;
    CommTvt_CreateNameWindows(sys);
    GFL_BGSysLoadNCLRDefault(23, 5, 0, 0x140, 0x20, sys->heapId);
    GFL_BGSysLoadNCLRDefault(23, 5, 4, 0x140, 0x20, sys->heapId);
    func_020232d8();
    LoadSysMsgBox(4, 0x140, 9, 0, sys->heapId);
    LoadSysMsgBox(0, 0x200, 9, 0, sys->heapId);
    sys->taskMenuRes = AppTaskMenuRes_Create(4, 7, sys->font, sys->printQueue, sys->heapId);
    CtvtCamera_Restart(sys, sys->camera);
    func_02043868(heapId, FALSE);
    CtvtComm_ResetSession(sys, sys->comm);
    func_02042ba8(FALSE, sys->heapId);
    GFL_SndBGMPlay(SEQ_BGM_SILENCE_FIELD, 0xffff);
}

static void CommTvt_Free(CommTvtWork *sys) {
    u8 i;

    for (i = 0; i < COMM_TVT_MEMBERS; i++) {
        BmpWin_Free(sys->nameWindows[i]);
    }
    GFL_TCBRemove(sys->vblankTask);
    DrawSystem_Delete(sys->drawSystem);
    CtvtCall_Delete(sys, sys->call);
    CtvtDraw_Delete(sys, sys->draw);
    CtvtTalk_Delete(sys, sys->talk);
    CtvtGame_Delete(sys, sys->game);
    func_020438dc();
    CtvtComm_Delete(sys, sys->comm);
    CtvtCamera_Delete(sys, sys->camera);
    AppTaskMenuRes_Free(sys->taskMenuRes);
    CommTvt_FreeMessages(sys);
    CommTvt_FreeResources(sys);
    CommTvt_FreeGraphics(sys);
}

// Frees the screens and resources for the game, which has its own
static void CommTvt_Suspend(CommTvtWork *sys) {
    u8 i;

    for (i = 0; i < COMM_TVT_MEMBERS; i++) {
        BmpWin_Free(sys->nameWindows[i]);
    }
    GFL_TCBRemove(sys->vblankTask);
    if (sys->waitIcon != NULL) {
        WaitIcon_Free(sys->waitIcon);
        sys->waitIcon = NULL;
    }
    AppTaskMenuRes_Free(sys->taskMenuRes);
    CommTvt_FreeResources(sys);
    CommTvt_FreeGraphics(sys);
    CtvtComm_ScanNone(sys, sys->comm);
    CtvtCamera_StopCamera(sys, sys->camera);
}

// CommTvt_Free for an exit during the game
static void CommTvt_FreeSuspended(CommTvtWork *sys) {
    DrawSystem_Delete(sys->drawSystem);
    CtvtCall_Delete(sys, sys->call);
    CtvtDraw_Delete(sys, sys->draw);
    CtvtTalk_Delete(sys, sys->talk);
    CtvtGame_Delete(sys, sys->game);
    CtvtComm_Delete(sys, sys->comm);
    CtvtCamera_Delete(sys, sys->camera);
    CommTvt_FreeMessages(sys);
}

static BOOL CommTvt_Main(CommTvtWork *sys) {
    CtvtComm_Update(sys, sys->comm);
    switch (sys->mode) {
    case COMM_TVT_MODE_NONE:
        break;
    case COMM_TVT_MODE_TALK:
        sys->nextMode = CtvtTalk_Main(sys, sys->talk);
        break;
    case COMM_TVT_MODE_CALL:
        sys->nextMode = CtvtCall_Main(sys, sys->call);
        break;
    case COMM_TVT_MODE_DRAW:
        sys->nextMode = CtvtDraw_Main(sys, sys->draw);
        break;
    case COMM_TVT_MODE_GAME:
        sys->nextMode = CtvtGame_Main(sys, sys->game);
        break;
    case COMM_TVT_MODE_EXIT:
        if (CtvtComm_IsDone(sys, sys->comm) == TRUE && CtvtCamera_IsSoundDone(sys, sys->camera) == TRUE) {
            return TRUE;
        }
        return FALSE;
    case COMM_TVT_MODE_EXIT_ERROR:
        if (CtvtComm_IsDone(sys, sys->comm) == TRUE && CtvtCamera_IsSoundDone(sys, sys->camera) == TRUE &&
            GFL_WipeIsFinished() == TRUE) {
            return TRUE;
        }
        break;
    }

    if (sys->errorShown == TRUE && GFL_WipeIsFinished() == TRUE) {
        sys->nextMode = COMM_TVT_MODE_EXIT_ERROR;
    }
    if (sys->mode != sys->nextMode) {
        CommTvt_ChangeMode(sys);
    }
    if (sys->mode == COMM_TVT_MODE_GAME || sys->inGame == TRUE) {
        if ((sys->mode == COMM_TVT_MODE_EXIT || sys->mode == COMM_TVT_MODE_EXIT_ERROR) &&
            CommTvt_IsCameraEnabled() == TRUE) {
            CameraSystem_UpdateSound(CtvtCamera_GetCameraSystem(sys, CommTvt_GetCamera(sys)));
        }
        return FALSE;
    }

    CtvtCamera_Update(sys, sys->camera);
    DrawSystem_Update(sys->drawSystem);
    func_0204b794();
    func_02021a3c(sys->printQueue);
    CommTvt_UpdateNames(sys);
    {
        u8 frame;

        sys->paletteAngle += 0x200;
        frame = FX_FX32_TO_F32((FX_SinIdx(sys->paletteAngle) + FX32_ONE) / 2 * COMM_TVT_PALETTE_FRAMES);
        if (frame >= COMM_TVT_PALETTE_FRAMES) {
            frame = COMM_TVT_PALETTE_FRAMES - 1;
        }
        if (frame != sys->paletteFrame) {
            sys->paletteFrame = frame;
            NNS_GfdRegisterNewVramTransferTask(0x1f, 0x40, sys->palettes[frame], sizeof(sys->palettes[0]));
        }
    }
    GFL_TCBExMgrUpdate(sys->tcbEx);
    GFL_BGSysMoveBG(1, BG_MOVE_SET_X, GFL_RandomLCAlt(256));
    GFL_BGSysMoveBG(1, BG_MOVE_SET_Y, GFL_RandomLCAlt(256));
    return FALSE;
}

static void CommTvt_VBlank(TCB *tcb, void *data) {
    CommTvtWork *sys = data;

    if (sys->mode != COMM_TVT_MODE_GAME) {
        CtvtCamera_Draw(sys, sys->camera);
        func_0204b7c8();
        DrawSystem_Draw(sys->drawSystem);
        CtvtTalk_Draw(sys, sys->talk);
    }
}

static void CommTvt_InitGraphics(CommTvtWork *sys) {
    ClActSysSetup clactSetup;

    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GX_SetVisiblePlane(0);
    GXS_SetVisiblePlane(0);
    Wipe_SetScreenCovered(0, 0);
    Wipe_SetScreenCovered(1, 0);
    Wipe_HideWindows(0);
    Wipe_HideWindows(1);
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
    GFL_BGSysSetVRAMBanks(&sCommTvtVRAMConfig);
    GFL_BGSysCreate(sys->heapId);
    BmpWin_InitAllocator(sys->heapId);
    GFL_BGSysSetLCDConfig(&sCommTvtLCDConfig);
    CommTvt_CreateBG(&sCommTvtBG0Setup, 0, BGMODE_TEXT);
    CommTvt_CreateBG(&sCommTvtBG1Setup, 1, BGMODE_TEXT);
    CommTvt_CreateBG(&sCommTvtBG4Setup, 4, BGMODE_TEXT);
    CommTvt_CreateBG(&sCommTvtBG5Setup, 5, BGMODE_TEXT);
    CommTvt_CreateBG(&sCommTvtBG6Setup, 6, BGMODE_TEXT);
    CommTvt_CreateBG(&sCommTvtBG7Setup, 7, BGMODE_TEXT);
    G2_SetBG2ControlDCBmp(GX_BG_SCRSIZE_DCBMP_256x256, GX_BG_AREAOVER_XLU, GX_BG_BMPSCRBASE_0x10000);
    G2_SetBG3ControlDCBmp(GX_BG_SCRSIZE_DCBMP_256x256, GX_BG_AREAOVER_XLU, GX_BG_BMPSCRBASE_0x28000);
    sys_memset(gfxGetScreenAddrBG2A(), 0, 256 * 192 * 2);
    sys_memset(gfxGetScreenAddrBG3A(), 0, 256 * 192 * 2);
    G2_SetBG2Priority(1);
    G2_SetBG3Priority(2);
    GFL_BGSysSetBGEnabled(2, TRUE);
    GFL_BGSysSetBGEnabled(3, TRUE);
    clactSetup = data_02093f08;
    ClActSys_Create(&clactSetup, &sCommTvtVRAMConfig, sys->heapId);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    sys->clactUnit = func_0204bf1c(64, 0, sys->heapId);
    func_0204c028(sys->clactUnit);
    // The video's windows: window 0 covers the screen, window 1 its right half
    GX_SetVisibleWnd(GX_WNDMASK_W0 | GX_WNDMASK_W1);
    G2_SetWnd0InsidePlane(0x1f, TRUE);
    G2_SetWnd1InsidePlane(0x1f, TRUE);
    G2_SetWndOutsidePlane(0x13, TRUE);
    G2_SetWnd0Position(0, 0, 255, 192);
    G2_SetWnd1Position(128, 0, 0, 192);
}

static void CommTvt_FreeGraphics(CommTvtWork *sys) {
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    func_0204bf98(sys->clactUnit);
    func_0204b758();
    GFL_BGSysReleaseBG(7);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(0);
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
}

static void CommTvt_CreateBG(const BGSetup *setup, u8 bg, u8 mode) {
    GFL_BGSysCreateBG(bg, setup, mode);
    GFL_BGSysSetBGEnabled(bg, TRUE);
    GFL_BGSysClearBG(bg);
    GFL_BGSysLoadScr(bg);
}

static void CommTvt_LoadResources(CommTvtWork *sys) {
    PlayerInfo *player = GetGameDataPlayerInfo(sys->param->gameData);
    ArcTool *arc;
    NNSG2dPaletteData *palette;
    ClActorSetup setup;
    void *file;

    sys->arc = GFL_ArcSysCreateFileHandle(ARC_COMM_TVT, sys->heapId);
    if (getTrainerGender(player) == GENDER_MALE) {
        GFL_G2DIOLoadArcNCLRDefault(sys->arc, 7, 4, 0, 0xc0, sys->heapId);
    } else {
        GFL_G2DIOLoadArcNCLRDefault(sys->arc, 6, 4, 0, 0xc0, sys->heapId);
    }
    GFL_BGSysLoadArcNCGRStatic(sys->arc, 12, 7, 0, 0, FALSE, sys->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(sys->arc, 14, 7, 0, 0, FALSE, sys->heapId);
    GFL_BGSysLoadScr(7);
    GFL_G2DIOLoadArcNCLRDefault(sys->arc, 0, 0, 0x20, 0x20, sys->heapId);
    GFL_BGSysLoadArcNCGRStatic(sys->arc, 8, 1, 0, 0, FALSE, sys->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(sys->arc, 13, 1, 0, 0, FALSE, sys->heapId);
    GFL_BGSysLoadScr(1);
    sys->objResources[0] = func_0204bc48(sys->arc, 3, 0, 0, sys->heapId);
    sys->objResources[4] = func_0204b81c(sys->arc, 27, FALSE, 0, sys->heapId);
    sys->objResources[8] = func_0204bde0(sys->arc, 21, 24, sys->heapId);
    sys->objResources[1] = func_0204bc48(sys->arc, 3, 1, 0, sys->heapId);
    sys->objResources[5] = func_0204b81c(sys->arc, 26, FALSE, 1, sys->heapId);
    sys->objResources[9] = func_0204bde0(sys->arc, 20, 23, sys->heapId);

    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), sys->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, func_0202d820(), 0, 0, 0x20, sys->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, func_0202d824(), 0, 0, 0, FALSE, sys->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, func_0202d820(), 4, 0xc0, 0x20, sys->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, func_0202d824(), 5, 0, 0, FALSE, sys->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, func_0202d828(), 5, 0, 0, FALSE, sys->heapId);
    GFL_BGSysSetScrPaletteNo(5, 0, 21, 32, 3, 6);
    GFL_BGSysLoadScr(5);
    sys->objResources[3] = func_0204bbb8(arc, func_0202d810(), 0, 0xe0, 0, 3, sys->heapId);
    sys->objResources[7] = func_0204b81c(arc, func_0202d814(), FALSE, 0, sys->heapId);
    sys->objResources[2] = func_0204bbb8(arc, func_0202d810(), 1, 0xe0, 0, 3, sys->heapId);
    sys->objResources[6] = func_0204b81c(arc, func_0202d814(), FALSE, 1, sys->heapId);
    sys->objResources[10] = func_0204bde0(arc, func_0202d818(2), func_0202d81c(2), sys->heapId);
    GFL_ArcToolFree(arc);

    setup.x = 0;
    setup.y = 0;
    setup.sequence = 0;
    setup.priority = 0;
    setup.bgPriority = 0;
    sys->talkerCursor =
        func_0204c040(CommTvt_GetClActUnit(sys), CommTvt_GetObjResource(sys, 4), CommTvt_GetObjResource(sys, 0),
                      CommTvt_GetObjResource(sys, 8), &setup, 0, sys->heapId);
    func_0204c124(sys->talkerCursor, FALSE);
    func_0204c520(sys->talkerCursor, TRUE);

    file = GFL_G2DIOReadNCLRArc(sys->arc, getTrainerGender(player) == GENDER_MALE ? 7 : 6, &palette, sys->heapId);
    sys_memcpy32((u8 *)palette->rawData + 0xc0, sys->palettes, sizeof(sys->palettes));
    GFL_HeapFree(file);
}

static void CommTvt_FreeResources(CommTvtWork *sys) {
    u8 i;

    func_0204c108(sys->talkerCursor);
    for (i = 0; i < 4; i++) {
        func_0204bcd0(sys->objResources[i]);
    }
    for (i = 4; i < 8; i++) {
        func_0204b98c(sys->objResources[i]);
    }
    for (i = 8; i < COMM_TVT_OBJ_RESOURCES; i++) {
        func_0204be64(sys->objResources[i]);
    }
    GFL_ArcToolFree(sys->arc);
}

static void CommTvt_InitMessages(CommTvtWork *sys) {
    sys->font = GFL_FontCreate(23, 0, 0, FALSE, sys->heapId);
    sys->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_SERVER_FLOW, sys->heapId);
    sys->printQueue = func_020219a8(0x800, sys->heapId);
    GFL_BGSysLoadNCLRDefault(23, 5, 0, 0x140, 0x20, sys->heapId);
    GFL_BGSysLoadNCLRDefault(23, 5, 4, 0x140, 0x20, sys->heapId);
    func_020232d8();
    LoadSysMsgBox(4, 0x140, 9, 0, sys->heapId);
    LoadSysMsgBox(0, 0x200, 9, 0, sys->heapId);
    sys->tcbEx = GFL_TCBExMgrCreate(sys->heapId, sys->heapId, 1, 0);
    sys->waitIcon = NULL;
}

static void CommTvt_FreeMessages(CommTvtWork *sys) {
    if (sys->waitIcon != NULL) {
        WaitIcon_Free(sys->waitIcon);
        sys->waitIcon = NULL;
    }
    GFL_TCBExMgrFreeTasks(sys->tcbEx);
    GFL_TCBExMgrFree(sys->tcbEx);
    func_02021c44(sys->printQueue);
    func_02021a18(sys->printQueue);
    GFL_MsgDataFree(sys->msgData);
    GFL_FontFree(sys->font);
}

static void CommTvt_ChangeMode(CommTvtWork *sys) {
    switch (sys->mode) {
    case COMM_TVT_MODE_NONE:
        break;
    case COMM_TVT_MODE_TALK:
        CtvtTalk_Leave(sys, sys->talk);
        break;
    case COMM_TVT_MODE_CALL:
        CtvtCall_Leave(sys, sys->call);
        break;
    case COMM_TVT_MODE_DRAW:
        CtvtDraw_Leave(sys, sys->draw);
        break;
    case COMM_TVT_MODE_GAME:
        CtvtGame_Leave(sys, sys->game);
        if (sys->inGame == TRUE && sys->nextMode == COMM_TVT_MODE_TALK) {
            CommTvt_Resume(sys);
            sys->inGame = FALSE;
        }
        break;
    }

    sys->mode = sys->nextMode;
    switch (sys->mode) {
    case COMM_TVT_MODE_NONE:
        break;
    case COMM_TVT_MODE_TALK:
        CtvtTalk_Enter(sys, sys->talk);
        break;
    case COMM_TVT_MODE_CALL:
        CtvtCall_Enter(sys, sys->call);
        break;
    case COMM_TVT_MODE_DRAW:
        CtvtDraw_Enter(sys, sys->draw);
        break;
    case COMM_TVT_MODE_GAME:
        CommTvt_Suspend(sys);
        CtvtGame_Enter(sys, sys->game);
        sys->inGame = TRUE;
        break;
    case COMM_TVT_MODE_EXIT_ERROR:
        Wipe_SetScreenCovered(0, 0);
        Wipe_SetScreenCovered(1, 0);
        // fallthrough
    case COMM_TVT_MODE_EXIT:
        CtvtComm_Disconnect(sys, sys->comm);
        CtvtCamera_StopCamera(sys, sys->camera);
        CtvtCamera_EndRecording(sys, sys->camera);
        break;
    }
}

static void CommTvt_UpdateNames(CommTvtWork *sys) {
    BOOL copied = FALSE;
    u8 i;

    if (sys->mode == COMM_TVT_MODE_DRAW || sys->mode == COMM_TVT_MODE_GAME) {
        return;
    }
    {
        int displayMode = CommTvt_GetDisplayMode(sys);

        for (i = 0; i < COMM_TVT_MEMBERS; i++) {
            if (CtvtComm_IsMemberActive(sys, sys->comm, i) == TRUE &&
                CtvtComm_IsMemberInfoReceived(sys, sys->comm, i) == TRUE &&
                CtvtCamera_IsWindowMoving(sys, sys->camera, i) == FALSE) {
                int bit = 1 << i;

                if (!(sys->namesPrinted & bit)) {
                    const CtvtCommMemberInfo *info = CtvtComm_GetMemberInfo(sys, sys->comm, i);
                    u8 window;

                    // With two members, the video is split across the top and bottom halves
                    if (displayMode == CTDM_DOUBLE) {
                        if (i == 0) {
                            window = 2;
                        } else {
                            window = 3;
                        }
                    } else {
                        window = i;
                    }
                    CommTvt_PrintName(sys, info, sys->nameWindows[window]);
                    sys->namesPrinted += (u8)bit;
                    sys->namesPending += (u8)(1 << window);
                }
            }
        }
    }
    for (i = 0; i < COMM_TVT_MEMBERS; i++) {
        int bit = 1 << i;

        if ((sys->namesPending & bit) && !func_02021c1c(sys->printQueue, BmpWin_GetBitmap(sys->nameWindows[i]))) {
            BmpWin_Transfer(sys->nameWindows[i]);
            sys->namesPending -= (u8)bit;
            copied = TRUE;
        }
    }
    if (copied == TRUE) {
        u8 talker = CtvtComm_GetTalker(sys, sys->comm);

        if (talker != 0xff) {
            func_ov257_021aad74(sys, talker);
        }
    }
}

// Prints the name centered, with a border of the shadow's color
static void CommTvt_PrintName(CommTvtWork *sys, const CtvtCommMemberInfo *info, BmpWin *window) {
    StrBuf *name = copyTrainerNameToNewStrbuf((const u16 *)info->playerInfo, sys->heapId);
    int x = (96 - (u8)GFL_FontGetBlockWidth(name, sys->font, 0)) / 2;

    func_02021c7c(sys->printQueue, BmpWin_GetBitmap(window), x + 1, 1, name, sys->font, 0x400);
    func_02021c7c(sys->printQueue, BmpWin_GetBitmap(window), x - 1, 1, name, sys->font, 0x400);
    func_02021c7c(sys->printQueue, BmpWin_GetBitmap(window), x, 2, name, sys->font, 0x400);
    func_02021c7c(sys->printQueue, BmpWin_GetBitmap(window), x, 0, name, sys->font, 0x400);
    func_02021c7c(sys->printQueue, BmpWin_GetBitmap(window), x, 1, name, sys->font, 0x3c00);
    GFL_StrBufFree(name);
}

CtvtCamera *CommTvt_GetCamera(CommTvtWork *sys) {
    return sys->camera;
}

CtvtComm *CommTvt_GetComm(CommTvtWork *sys) {
    return sys->comm;
}

CtvtTalk *CommTvt_GetTalk(CommTvtWork *sys) {
    return sys->talk;
}

CtvtMic *CommTvt_GetMic(CommTvtWork *sys) {
    return CtvtTalk_GetMic(sys, sys->talk);
}

DrawSystem *CommTvt_GetDrawSystem(CommTvtWork *sys) {
    return sys->drawSystem;
}

CtvtGame *CommTvt_GetGame(CommTvtWork *sys) {
    return sys->game;
}

CtvtCall *CommTvt_GetCall(CommTvtWork *sys) {
    return sys->call;
}

CommTvtParam *CommTvt_GetParam(CommTvtWork *sys) {
    return sys->param;
}

HeapID CommTvt_GetHeapId(CommTvtWork *sys) {
    return sys->heapId;
}

ArcTool *CommTvt_GetArc(CommTvtWork *sys) {
    return sys->arc;
}

u32 CommTvt_GetObjResource(CommTvtWork *sys, int index) {
    return sys->objResources[index];
}

ClActUnit *CommTvt_GetClActUnit(CommTvtWork *sys) {
    return sys->clactUnit;
}

BOOL func_ov257_021aaa74(CommTvtWork *sys) {
    return sys->unk10;
}

void func_ov257_021aaa78(CommTvtWork *sys, BOOL value) {
    sys->unk10 = value;
}

Font *CommTvt_GetFont(CommTvtWork *sys) {
    return sys->font;
}

MsgData *CommTvt_GetMsgData(CommTvtWork *sys) {
    return sys->msgData;
}

PrintQueue *CommTvt_GetPrintQueue(CommTvtWork *sys) {
    return sys->printQueue;
}

AppTaskMenuRes *CommTvt_GetTaskMenuRes(CommTvtWork *sys) {
    return sys->taskMenuRes;
}

u8 CommTvt_GetMode(CommTvtWork *sys) {
    return sys->mode;
}

u8 CommTvt_GetNextMode(CommTvtWork *sys) {
    return sys->nextMode;
}

void CommTvt_SetErrorShown(CommTvtWork *sys) {
    sys->errorShown = TRUE;
}

BOOL CommTvt_IsErrorShown(CommTvtWork *sys) {
    return sys->errorShown;
}

u8 CommTvt_GetMemberCount(CommTvtWork *sys) {
    return sys->memberCount;
}

void CommTvt_SetMemberCount(CommTvtWork *sys, u8 count) {
    sys->memberCount = count;
}

int CommTvt_GetDisplayMode(CommTvtWork *sys) {
    u8 count = CommTvt_GetMemberCount(sys);

    if (count == 1) {
        return CTDM_DOUBLE;
    }
    if (count == 2) {
        return CTDM_DOUBLE;
    }
    return CTDM_QUAD;
}

BOOL CommTvt_IsZoomed(CommTvtWork *sys) {
    return sys->zoomed;
}

void CommTvt_SendZoom(CommTvtWork *sys, BOOL zoomed) {
    CtvtComm_SendZoom(sys, sys->comm, zoomed);
}

void CommTvt_ToggleZoom(CommTvtWork *sys) {
    CommTvt_SendZoom(sys, sys->zoomed == FALSE ? TRUE : FALSE);
}

void CommTvt_SetZoomed(CommTvtWork *sys, BOOL zoomed) {
    sys->zoomed = zoomed;
}

u8 CommTvt_GetSelfIndex(CommTvtWork *sys) {
    return sys->selfIndex;
}

void CommTvt_SetSelfIndex(CommTvtWork *sys, u8 index) {
    sys->selfIndex = index;
}

BOOL func_ov257_021aab10(CommTvtWork *sys) {
    return sys->unk20;
}

void func_ov257_021aab14(CommTvtWork *sys, BOOL value) {
    sys->unk20 = value;
}

BOOL func_ov257_021aab18(CommTvtWork *sys) {
    return sys->unk24;
}

void func_ov257_021aab1c(CommTvtWork *sys) {
    sys->unk24 = sys->unk24 == FALSE ? TRUE : FALSE;
}

BOOL func_ov257_021aab2c(CommTvtWork *sys) {
    return sys->unk28;
}

void func_ov257_021aab30(CommTvtWork *sys, BOOL value) {
    sys->unk28 = value;
}

BOOL func_ov257_021aab34(CommTvtWork *sys) {
    return sys->unk2C;
}

void func_ov257_021aab38(CommTvtWork *sys, BOOL value) {
    sys->unk2C = value;
}

BOOL func_ov257_021aab3c(CommTvtWork *sys) {
    if (sys->param->unk4 == COMM_TVT_START_EXISTING) {
        return TRUE;
    }
    return FALSE;
}

BOOL CommTvt_CanExchangePhotos(CommTvtWork *sys) {
    return sys->canExchangePhotos;
}

void CommTvt_ClearCanExchangePhotos(CommTvtWork *sys) {
    sys->canExchangePhotos = FALSE;
}

BOOL func_ov257_021aab5c(CommTvtWork *sys) {
    return sys->unk34;
}

void func_ov257_021aab60(CommTvtWork *sys, BOOL value) {
    sys->unk34 = value;
}

BOOL CommTvt_IsCameraEnabled(void) {
    if (isRunningOnDSi() == TRUE && canPlayerExchangePhotos() == FALSE) {
        return TRUE;
    }
    return FALSE;
}

AppTaskMenu *func_ov257_021aab80(CommTvtWork *sys) {
    AppTaskMenuInit init = { 0 };
    AppTaskMenuItem items[2] = { 0 };
    AppTaskMenu *menu;

    items[0].str = GFL_MsgDataLoadStrbufNew(sys->msgData, BtlServerFlow_Text_Yes);
    items[1].str = GFL_MsgDataLoadStrbufNew(sys->msgData, BtlServerFlow_Text_No);
    items[0].color = 0x39e3;
    items[1].color = 0x39e3;
    items[0].type = 0;
    items[1].type = 0;
    init.heapId = sys->heapId;
    init.itemCount = 2;
    init.items = items;
    init.x = 24;
    init.y = 6;
    init.width = 8;
    init.height = 3;
    init.posType = APP_TASKMENU_POS_TOP_LEFT;
    menu = AppTaskMenu_Create(&init, sys->taskMenuRes);
    GFL_StrBufFree(items[0].str);
    GFL_StrBufFree(items[1].str);
    return menu;
}

AppTaskMenu *func_ov257_021aac08(CommTvtWork *sys, u8 right, u8 bottom) {
    AppTaskMenuInit init = { 0 };
    AppTaskMenuItem items[2] = { 0 };
    AppTaskMenu *menu;

    items[0].str = GFL_MsgDataLoadStrbufNew(sys->msgData, BtlServerFlow_Text_Yes);
    items[1].str = GFL_MsgDataLoadStrbufNew(sys->msgData, BtlServerFlow_Text_No);
    items[0].color = 0x39e3;
    items[1].color = 0x39e3;
    items[0].type = 0;
    items[1].type = 0;
    init.heapId = sys->heapId;
    init.itemCount = 2;
    init.items = items;
    init.posType = APP_TASKMENU_POS_TOP_LEFT;
    init.x = right;
    init.y = bottom;
    init.width = 8;
    init.height = 3;
    menu = AppTaskMenu_Create(&init, sys->taskMenuRes);
    GFL_StrBufFree(items[0].str);
    GFL_StrBufFree(items[1].str);
    return menu;
}

AppTaskMenu *func_ov257_021aac98(CommTvtWork *sys, u8 right, u8 bottom) {
    AppTaskMenuInit init = { 0 };
    AppTaskMenuItem item = { 0 };
    AppTaskMenu *menu;

    item.str = GFL_MsgDataLoadStrbufNew(sys->msgData, BtlServerFlow_Text_Cancel);
    item.color = 0x39e3;
    item.type = 0;
    init.heapId = sys->heapId;
    init.itemCount = 1;
    init.items = &item;
    init.posType = APP_TASKMENU_POS_TOP_LEFT;
    init.x = right;
    init.y = bottom;
    init.width = 8;
    init.height = 3;
    menu = AppTaskMenu_Create(&init, sys->taskMenuRes);
    GFL_StrBufFree(item.str);
    return menu;
}

void func_ov257_021aad08(CommTvtWork *sys) {
    u8 i;

    sys->namesPrinted = 0;
    sys->namesPending = 0;
    for (i = 0; i < COMM_TVT_MEMBERS; i++) {
        GFL_BitmapFill(BmpWin_GetBitmap(sys->nameWindows[i]), 0);
        if (sys->mode != COMM_TVT_MODE_DRAW) {
            BmpWin_ClearScreen(sys->nameWindows[i]);
        }
    }
}

void func_ov257_021aad48(CommTvtWork *sys) {
    u8 i;

    for (i = 0; i < COMM_TVT_MEMBERS; i++) {
        GFL_BitmapFill(BmpWin_GetBitmap(sys->nameWindows[i]), 0);
        BmpWin_FlushChar(sys->nameWindows[i]);
    }
}

// Points the cursor at the talker's name
void func_ov257_021aad74(CommTvtWork *sys, u8 talker) {
    int displayMode = CommTvt_GetDisplayMode(sys);
    u8 window = talker;

    if (sys->mode == COMM_TVT_MODE_DRAW) {
        return;
    }
    if (displayMode == CTDM_DOUBLE) {
        if (talker == 0) {
            window = 2;
        } else {
            window = 3;
        }
    }
    if (CtvtComm_IsMemberActive(sys, sys->comm, talker) == TRUE &&
        CtvtComm_IsMemberInfoReceived(sys, sys->comm, talker) == TRUE) {
        u8 cursorX[COMM_TVT_MEMBERS] = { 72, 200, 72, 200 };
        u8 cursorY[COMM_TVT_MEMBERS] = { 88, 88, 184, 184 };
        ClActorPos pos;
        StrBuf *name = copyTrainerNameToNewStrbuf(
            (const u16 *)CtvtComm_GetMemberInfo(sys, sys->comm, talker)->playerInfo, sys->heapId);
        u8 width = GFL_FontGetBlockWidth(name, sys->font, 0);

        GFL_StrBufFree(name);
        pos.x = cursorX[window] + width / 2;
        pos.y = cursorY[window];
        func_0204c140(sys->talkerCursor, &pos, 0);
        func_0204c124(sys->talkerCursor, TRUE);
        func_0204c56c(sys->talkerCursor);
    }
}

void func_ov257_021aae44(CommTvtWork *sys) {
    func_0204c124(sys->talkerCursor, FALSE);
}

void func_ov257_021aae54(CommTvtWork *sys, const CtvtCommMemberInfo *info) {
    func_0200f700(getHollow_RivalData(GameData_GetSaveControl(sys->param->gameData)),
                  getIDAsUInt((PlayerInfo *)info->playerInfo));
}

void func_ov257_021aae7c(CommTvtWork *sys, BmpWin *window) {
    if (sys->waitIcon != NULL) {
        func_ov257_021aaeb0(sys);
    }
    sys->waitIcon = WaitIcon_CreateTCBEx(sys->tcbEx, window, 15, 16, sys->heapId);
}

void func_ov257_021aaeb0(CommTvtWork *sys) {
    if (sys->waitIcon != NULL) {
        WaitIcon_Free(sys->waitIcon);
        sys->waitIcon = NULL;
    }
}

static BOOL CommTvt_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    CommTvtWork *sys;

    if (param != NULL && ((CommTvtParam *)param)->unk4 == COMM_TVT_START_EXISTING) {
        GFL_HeapCreateChild(HEAPID_USER, HEAPID_COMM_TVT, 0xd0000);
    } else {
        GFL_HeapCreateChild(HEAPID_USER, HEAPID_COMM_TVT, 0x130000);
    }
    sys = GFL_ProcInitSubsystem(proc, sizeof(CommTvtWork), HEAPID_COMM_TVT);
    sys->heapId = HEAPID_COMM_TVT;
    if (param == NULL) {
        // Started on its own, for debugging: R calls, X answers
        sys->param = GFL_HeapAllocate(HEAPID_USER, sizeof(CommTvtParam), TRUE, "comm_tvt_sys.c", 1656);
        sys->param->gameData = GameData_Create(HEAPID_USER);
        sys->param->unk4 = COMM_TVT_START_WAIT;
        if (GCTX_HIDGetHeldKeys() & PAD_BUTTON_R) {
            sys->param->unk4 = COMM_TVT_START_CALL;
        }
        if (GCTX_HIDGetHeldKeys() & PAD_BUTTON_X) {
            sys->param->unk4 = COMM_TVT_START_ANSWER;
        }
    } else {
        sys->param = param;
    }
    sys->errorShown = FALSE;
    CommTvt_Init(sys);
    return TRUE;
}

static BOOL CommTvt_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    CommTvtWork *sys = work;

    if (sys->inGame == FALSE) {
        CommTvt_Free(sys);
    } else {
        CommTvt_FreeSuspended(sys);
    }
    if (param == NULL) {
        GFL_HeapFree(sys->param);
    }
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_COMM_TVT);
    return TRUE;
}

static BOOL CommTvt_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    CommTvtWork *sys = work;

    if (GFL_NetErrCheck() && sys->errorShown == FALSE) {
        GFL_NetErrMarkShown();
        sys->errorShown = TRUE;
    }
    if (CommTvt_Main(sys) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

const GameProcFunctions COMM_TVT_PROC_FUNCTIONS = {
    CommTvt_ProcInit,
    CommTvt_ProcMain,
    CommTvt_ProcExit,
};
