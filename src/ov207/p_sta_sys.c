#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/net.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "pml/poke_party.h"
#include "p_status_local.h"
#include "system/app_menu_common.h"
#include "system/game_data.h"
#include "system/wipe.h"

// The summary screen's core: setting up and tearing down the screens, the main loop, the buttons of the bottom
// screen, changing the page or the Pokémon behind a mosaic, and printing for the pages. The file's name is a guess

// The shortcut of the info page; the skill and ribbon pages' follow it
#define SHORTCUT_PSTATUS_INFO 9

// The BG palettes of each screen, in the version's colors
#ifdef BLACK2
#define NCLR_MAIN_BG 1
#define NCLR_SUB_BG 3
#else
#define NCLR_MAIN_BG 0
#define NCLR_SUB_BG 2
#endif

static void PStatus_VBlank(TCB *tcb, void *data);
static void PStatus_InitGraphics(PStatusWork *wk);
static void PStatus_ExitGraphics(PStatusWork *wk);
static void PStatus_CreateBG(const BGSetup *setup, u8 bg, u8 mode);
static void PStatus_LoadResources(PStatusWork *wk);
static void PStatus_FreeResources(PStatusWork *wk);
static void PStatus_CreateActors(PStatusWork *wk);
static void PStatus_FreeActors(PStatusWork *wk);
static void PStatus_InitText(PStatusWork *wk);
static void PStatus_ExitText(PStatusWork *wk);
static void PStatus_CheckTouch(PStatusWork *wk);
static void PStatus_HandleInput(PStatusWork *wk);
static BOOL PStatus_HandleKeys(PStatusWork *wk);
static void PStatus_HandleTouch(PStatusWork *wk);
static void PStatus_AnimatePalettes(PStatusWork *wk);
static BOOL PStatus_ChangePokemon(PStatusWork *wk, u8 dir);
static BOOL PStatus_FindPokemon(PStatusWork *wk, u8 dir, u16 *index);
static void PStatus_LoadPokemon(PStatusWork *wk);
static void PStatus_StartRedraw(PStatusWork *wk);
static void PStatus_UpdateRedraw(PStatusWork *wk);
static BOOL PStatus_IsFromFieldMenu(PStatusWork *wk);
static BOOL PStatus_HasRibbon(PStatusWork *wk, BoxPkm *pkm);

static const BGSysVRAMConfig sPStatusVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,        GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_D,   GX_VRAM_TEXPLTT_0_F,     GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const BGSysLCDConfig sPStatusLCDConfig = { 1, 0, 0, 1 };

static const BGSetup sPStatusBG1Setup = { 0, 0, 0x800, 0, 1, GX_BG_COLORMODE_16, 0xc, 2, 0x8000, 0, 1, 0, 0 };
static const BGSetup sPStatusBG2Setup = { 0, 0, 0x800, 0, 1, GX_BG_COLORMODE_16, 0xd, 4, 0x8000, 0, 2, 1, 0 };
static const BGSetup sPStatusBG3Setup = { 0, 0, 0x800, 0, 1, GX_BG_COLORMODE_16, 0xe, 4, 0, 1, 3, 0, 0 };
static const BGSetup sPStatusBG4Setup = { 0, 0, 0x800, 0, 1, GX_BG_COLORMODE_16, 0xc, 0, 0x6000, 1, 0, 0, 0 };
static const BGSetup sPStatusBG5Setup = { 0, 0, 0x800, 0, 1, GX_BG_COLORMODE_16, 0xd, 4, 0x8000, 1, 1, 0, 0 };
static const BGSetup sPStatusBG6Setup = { 0, 0, 0x800, 0, 1, GX_BG_COLORMODE_16, 0xe, 2, 0, 1, 2, 0, 0 };
static const BGSetup sPStatusBG7Setup = { 0, 0, 0x800, 0, 1, GX_BG_COLORMODE_16, 0xf, 2, 0, 1, 3, 0, 0 };

static const VecFx32 sPStatusCameraPos = { FX32_CONST(-41), 0, FX32_CONST(101) };
static const VecFx32 sPStatusCameraTarget = { 0, 0, -FX32_ONE };
static const VecFx32 sPStatusCameraUp = { 0, FX32_ONE, 0 };

static const GXRgb sPStatusEdgeColors[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

BOOL PStatus_Init(PStatusWork *wk) {
    u8 i;

    wk->partyIndex = wk->param->partyIndex;
    wk->shownPartyIndex = 0xff;
    wk->bgScrollFrames = 0;
    wk->lastVBlankCount = OS_GetVBlankCount();
    wk->shownPage = PSTA_PAGE_NONE;
    wk->isInputEnabled = TRUE;
    wk->upPressed = FALSE;
    wk->downPressed = FALSE;
    wk->exitResult = 0;
    wk->isRedrawing = TRUE;
    wk->mosaicState = PSTA_MOSAIC_NONE;
    wk->mosaicLevel = 0;
    wk->seq = PSTA_SEQ_FADE_IN;
    wk->pressedButton = NULL;
    wk->playCry = FALSE;
    wk->canForgetHm = FALSE;
    wk->isTouch = func_0203d554();
    if (wk->param->mode == PSTATUS_MODE_FORGET_HM) {
        wk->param->mode = PSTATUS_MODE_FORGET_MOVE;
        wk->canForgetHm = TRUE;
    }
    if (wk->param->dataType == PSTATUS_DATA_BOX) {
        wk->boxPartyPkm = boxPkmRegenToPartyPkm(PStatus_GetBoxPkm(wk), wk->heapId);
    } else {
        wk->boxPartyPkm = NULL;
    }
    PStatus_InitGraphics(wk);
    wk->sub = PStaSub_Create(wk);
    wk->info = PStaInfo_Create(wk);
    wk->skill = PStaSkill_Create(wk);
    wk->ribbon = PStaRibbon_Create(wk);
    PStatus_LoadResources(wk);
    PStatus_InitText(wk);
    PStatus_CreateActors(wk);
    wk->vblankTcb = GFL_VBlankTCBAdd(PStatus_VBlank, wk, 8);
    wk->plttAnimPhase = 0;
    sys_memcpy16((void *)(HW_OBJ_PLTT + 0x6c), wk->cursorPltt, 12);
    sys_memcpy16((void *)(HW_OBJ_PLTT + 0x160), wk->tabPlttSrc, sizeof(wk->tabPlttSrc));
    sys_memcpy16((void *)(HW_OBJ_PLTT + 0x1a0), wk->buttonPltt, sizeof(wk->buttonPltt));
    sys_memcpy16((void *)(HW_OBJ_PLTT + 0x1a0), wk->buttonPlttSrc, sizeof(wk->buttonPlttSrc));
    PStatus_LoadPokemon(wk);
    if (wk->param->mode == PSTATUS_MODE_FORGET_MOVE) {
        wk->page = PSTATUS_PAGE_FORGET;
        PStaSub_Load(wk, wk->sub);
        PStaSkill_LoadForget(wk, wk->skill);
        PStaRibbon_LoadPokemon(wk, wk->ribbon);
    } else {
        wk->page = wk->param->page;
        if (wk->page < PSTATUS_PAGE_FORGET) {
            for (i = 0; i < 3; i++) {
                wk->shortcutRegistered[i] =
                    GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_PSTATUS_INFO + i);
            }
        }
        PStaSub_Load(wk, wk->sub);
        PStaRibbon_LoadPokemon(wk, wk->ribbon);
        switch (wk->page) {
        case PSTATUS_PAGE_INFO:
            PStaInfo_Load(wk, wk->info);
            break;
        case PSTATUS_PAGE_SKILL:
            PStaSkill_Load(wk, wk->skill);
            break;
        case PSTATUS_PAGE_RIBBON:
            PStaRibbon_Load(wk, wk->ribbon);
            break;
        default:
            wk->page = PSTATUS_PAGE_INFO;
            PStaInfo_Load(wk, wk->info);
            break;
        }
    }
    func_02042ba8(TRUE, wk->heapId);
    return TRUE;
}

BOOL PStatus_Exit(PStatusWork *wk) {
    u8 i;

    GFL_TCBRemove(wk->vblankTcb);
    wk->param->partyIndex = wk->partyIndex;
    PStatus_FreeActors(wk);
    PStatus_ExitText(wk);
    PStatus_FreeResources(wk);
    PStaRibbon_UnloadPokemon(wk, wk->ribbon);
    PStaRibbon_Free(wk, wk->ribbon);
    PStaSkill_Free(wk, wk->skill);
    PStaInfo_Free(wk, wk->info);
    PStaSub_Free(wk, wk->sub);
    PStatus_ExitGraphics(wk);
    if (wk->page < PSTATUS_PAGE_FORGET) {
        for (i = 0; i < 3; i++) {
            GameData_SetKeyItemRegistration(wk->param->gameData, SHORTCUT_PSTATUS_INFO + i, wk->shortcutRegistered[i]);
        }
    }
    if (wk->canForgetHm == TRUE) {
        wk->param->mode = PSTATUS_MODE_FORGET_HM;
        wk->canForgetHm = FALSE;
    }
    func_0203d564(wk->isTouch);
    if (wk->boxPartyPkm != NULL) {
        GFL_HeapFree(wk->boxPartyPkm);
    }
    return TRUE;
}

int PStatus_Main(PStatusWork *wk) {
    u32 vblankCount;

    wk->prevTouchX = wk->touchX;
    wk->prevTouchY = wk->touchY;
    func_0203da84(&wk->touchX, &wk->touchY);
    switch (wk->seq) {
    case PSTA_SEQ_FADE_IN:
        if (wk->isRedrawing == TRUE) {
            PStatus_UpdateRedraw(wk);
        } else {
            GFL_WipeSet(0, 1, 1, 0, 6, 1, wk->heapId);
            wk->seq = PSTA_SEQ_WAIT_FADE_IN;
        }
        break;
    case PSTA_SEQ_WAIT_FADE_IN:
        if (GFL_WipeIsFinished() == TRUE) {
            wk->seq = PSTA_SEQ_MAIN;
        }
        break;
    case PSTA_SEQ_EXIT:
        if (wk->pressedButton == NULL || func_0204c560(wk->pressedButton) == FALSE || wk->param->forceExit == TRUE) {
            GFL_WipeSet(0, 0, 0, 0, 6, 1, wk->heapId);
            wk->seq = PSTA_SEQ_WAIT_FADE_OUT;
        }
        break;
    case PSTA_SEQ_WAIT_FADE_OUT:
        if (GFL_WipeIsFinished() == TRUE) {
            return wk->exitResult;
        }
        break;
    case PSTA_SEQ_MAIN:
        PStatus_CheckTouch(wk);
        if (wk->isRedrawing == TRUE) {
            PStatus_UpdateRedraw(wk);
            break;
        }
        PStaSub_Main(wk, wk->sub);
        if (wk->param->forceExit == TRUE) {
            wk->exitResult = PSTATUS_RESULT_BACK;
            wk->seq = PSTA_SEQ_EXIT;
        }
        PStatus_HandleInput(wk);
        if (wk->isRedrawing == FALSE) {
            switch (wk->shownPage) {
            case PSTATUS_PAGE_INFO:
                PStaInfo_Main(wk, wk->info);
                break;
            case PSTATUS_PAGE_SKILL:
            case PSTATUS_PAGE_FORGET:
                PStaSkill_Main(wk, wk->skill);
                break;
            case PSTATUS_PAGE_RIBBON:
                PStaRibbon_Main(wk, wk->ribbon);
                break;
            }
        }
        break;
    }
    func_02021a3c(wk->printQueue);
    func_0204b794();
    MCSSSys_Update(wk->mcssSys);
    GFL_G3DSysReset();
    GFL_G3DSysMtxViewFlush();
    MCSSSys_Draw(wk->mcssSys);
    GFL_G3DSysReqSwapBuffers();
    vblankCount = OS_GetVBlankCount();
    wk->bgScrollFrames += (u8)(vblankCount - wk->lastVBlankCount);
    wk->lastVBlankCount = vblankCount;
    while (wk->bgScrollFrames >= 4) {
        GFL_BGSysMoveBGReq(3, BG_MOVE_RIGHT, 1);
        GFL_BGSysMoveBGReq(3, BG_MOVE_DOWN, 1);
        GFL_BGSysMoveBGReq(7, BG_MOVE_RIGHT, 1);
        GFL_BGSysMoveBGReq(7, BG_MOVE_DOWN, 1);
        wk->bgScrollFrames -= 4;
    }
    if (wk->upPressed == TRUE && func_0204c560(wk->buttons[PSTA_BUTTON_UP]) == FALSE) {
        if (PStatus_FindPokemon(wk, PSTA_DIR_UP, NULL) == TRUE) {
            func_0204c488(wk->buttons[PSTA_BUTTON_UP], 3);
        } else {
            func_0204c488(wk->buttons[PSTA_BUTTON_UP], 17);
        }
        wk->upPressed = FALSE;
    }
    if (wk->downPressed == TRUE && func_0204c560(wk->buttons[PSTA_BUTTON_DOWN]) == FALSE) {
        if (PStatus_FindPokemon(wk, PSTA_DIR_DOWN, NULL) == TRUE) {
            func_0204c488(wk->buttons[PSTA_BUTTON_DOWN], 2);
        } else {
            func_0204c488(wk->buttons[PSTA_BUTTON_DOWN], 16);
        }
        wk->downPressed = FALSE;
    }
    PStatus_AnimatePalettes(wk);
    return 0;
}

static void PStatus_VBlank(TCB *tcb, void *data) {
    func_0204b7c8();
}

static void PStatus_InitGraphics(PStatusWork *wk) {
    ClActSysSetup clactSetup;

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
    GX_SetDispSelect(GX_DISP_SELECT_SUB_MAIN);
    GFL_BGSysSetVRAMBanks(&sPStatusVRAMConfig);
    GFL_BGSysCreate(wk->heapId);
    BmpWin_InitAllocator(wk->heapId);
    GFL_BGSysSetLCDConfig(&sPStatusLCDConfig);
    PStatus_CreateBG(&sPStatusBG1Setup, 1, 0);
    PStatus_CreateBG(&sPStatusBG2Setup, 2, 0);
    PStatus_CreateBG(&sPStatusBG3Setup, 3, 0);
    GFL_BGSysSetBGEnabled(0, TRUE);
    PStatus_CreateBG(&sPStatusBG4Setup, 4, 0);
    PStatus_CreateBG(&sPStatusBG5Setup, 5, 0);
    PStatus_CreateBG(&sPStatusBG6Setup, 6, 0);
    PStatus_CreateBG(&sPStatusBG7Setup, 7, 0);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_PLANEMASK_BG2,
                        GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ, 13, 16);
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_PLANEMASK_BG2, GX_PLANEMASK_BG3, 13, 16);
    clactSetup = data_02093f08;
    clactSetup.charCount = 110;
    ClActSys_Create(&clactSetup, &sPStatusVRAMConfig, wk->heapId);
    wk->actorUnit = func_0204bf1c(96, 0, wk->heapId);
    func_0204c028(wk->actorUnit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    GFL_G3DSysCreate(FALSE, 2, FALSE, 1, 0, wk->heapId, NULL);
    wk->camera = GFL_G3DCameraCreate(G3DCAM_PROJECTION_ORTHO, FX32_CONST(192), 0, 0, FX32_CONST(256), FX32_CONST(128),
                                     FX32_CONST(512), 0, &sPStatusCameraPos, &sPStatusCameraUp, &sPStatusCameraTarget,
                                     wk->heapId);
    GFL_G3DCameraFlush(wk->camera);
    gfxSetEdgeColorTable(sPStatusEdgeColors);
    G3X_EdgeMarking(FALSE);
    GFL_G3DSysSetSwapBufferParams(0, 0);
    GFL_BGSysSet3DBGPriority(0);
    wk->mcssSys = MCSSSys_Create(2, wk->heapId);
    func_0201aefc(wk->mcssSys, 0);
    func_0201af9c(wk->mcssSys, 0x10000);
}

static void PStatus_ExitGraphics(PStatusWork *wk) {
    GfdClearVramTransferQueue();
    MCSSSys_Free(wk->mcssSys);
    GFL_G3DCameraFree(wk->camera);
    GFL_G3DSysFree();
    func_0204bf98(wk->actorUnit);
    func_0204b758();
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(7);
    GFL_BGSysReleaseBG(3);
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(0);
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
}

static void PStatus_CreateBG(const BGSetup *setup, u8 bg, u8 mode) {
    GFL_BGSysCreateBG(bg, setup, mode);
    GFL_BGSysSetBGEnabled(bg, TRUE);
    GFL_BGSysClearBG(bg);
    GFL_BGSysLoadScr(bg);
}

static void PStatus_LoadResources(PStatusWork *wk) {
    u8 i;
    ArcTool *uiArc;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_P_STATUS, wk->heapId);

    GFL_G2DIOLoadArcNCLRDefault(arc, NCLR_MAIN_BG, 0, 0, 0, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 10, 2, 0, 0, FALSE, wk->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 72, 3, 0, 0, FALSE, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, NCLR_SUB_BG, 4, 0, 0, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 12, 6, 0, 0, FALSE, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 11, 5, 0, 0, FALSE, wk->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 73, 7, 0, 0, FALSE, wk->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 67, 5, 0, 0, FALSE, wk->heapId);
    wk->clResources[PSTA_RES_PLTT(0)] = func_0204bbb8(arc, 5, 0, 0, 0, 4, wk->heapId);
    wk->clResources[PSTA_RES_PLTT(6)] = func_0204bbb8(arc, 7, 0, 0x160, 0, 2, wk->heapId);
    wk->clResources[PSTA_RES_PLTT(7)] = func_0204bbb8(arc, 8, 1, 0, 0, 5, wk->heapId);
    wk->clResources[PSTA_RES_PLTT(8)] = func_0204bbb8(arc, 6, 0, 0x1a0, 0, 1, wk->heapId);
    wk->clResources[PSTA_RES_PLTT(11)] = func_0204bbb8(arc, 4, 0, 0x1c0, 0, 1, wk->heapId);
    wk->clResources[PSTA_RES_CHAR(0)] = func_0204b81c(arc, 13, FALSE, 0, wk->heapId);
    wk->clResources[PSTA_RES_CHAR(8)] = func_0204b81c(arc, 17, FALSE, 0, wk->heapId);
    wk->clResources[PSTA_RES_CHAR(9)] = func_0204b81c(arc, 16, FALSE, 0, wk->heapId);
    wk->clResources[PSTA_RES_CHAR(12)] = func_0204b81c(arc, 15, FALSE, 0, wk->heapId);
    wk->clResources[PSTA_RES_CELL(0)] = func_0204bde0(arc, 80, 131, wk->heapId);
    wk->clResources[PSTA_RES_CELL(7)] = func_0204bde0(arc, 83, 134, wk->heapId);
    wk->clResources[PSTA_RES_CELL(8)] = func_0204bde0(arc, 82, 133, wk->heapId);
    wk->clResources[PSTA_RES_CELL(11)] = func_0204bde0(arc, 84, 135, wk->heapId);
    wk->clResources[PSTA_RES_CELL(12)] = func_0204bde0(arc, 81, 132, wk->heapId);

    uiArc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(uiArc, func_0202d820(), 0, 0x120, 0x20, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(uiArc, func_0202d824(), 1, 0, 0, FALSE, wk->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(uiArc, func_0202d828(), 1, 0, 0, FALSE, wk->heapId);
    GFL_BGSysSetScrPaletteNo(1, 0, 21, 32, 3, 9);
    GFL_G2DIOLoadArcNCLRDefault(uiArc, 0, 4, 0x1a0, 0x20, wk->heapId);
    wk->clResources[PSTA_RES_PLTT(5)] = func_0204bbb8(uiArc, 1, 1, 0x100, 0, 1, wk->heapId);
    wk->clResources[PSTA_RES_CHAR(7)] = func_0204b81c(uiArc, 2, FALSE, 1, wk->heapId);
    wk->clResources[PSTA_RES_CELL(6)] = func_0204bde0(uiArc, 5, 8, wk->heapId);
    wk->clResources[PSTA_RES_PLTT(3)] = func_0204bbb8(uiArc, func_0202d7e4(), 0, 0x100, 0, 3, wk->heapId);
    wk->clResources[PSTA_RES_PLTT(4)] = func_0204bbb8(uiArc, func_0202d7e4(), 1, 0xa0, 0, 3, wk->heapId);
    wk->clResources[PSTA_RES_CELL(4)] = func_0204bde0(uiArc, func_0202d7f8(2), func_0202d7fc(2), wk->heapId);
    wk->clResources[PSTA_RES_CELL(5)] = func_0204bde0(uiArc, func_0202d7f8(0), func_0202d7fc(0), wk->heapId);
    for (i = 0; i < PSTA_TYPE_COUNT; i++) {
        wk->typeIconChars[i] = func_0204b81c(uiArc, func_0202d7f4(i), FALSE, 0, wk->heapId);
    }
    wk->clResources[PSTA_RES_CHAR(4)] = func_0204b81c(uiArc, 56, FALSE, 1, wk->heapId);
    wk->clResources[PSTA_RES_CHAR(5)] = func_0204b81c(uiArc, 57, FALSE, 1, wk->heapId);
    wk->clResources[PSTA_RES_CHAR(6)] = func_0204b81c(uiArc, 58, FALSE, 1, wk->heapId);
    wk->clResources[PSTA_RES_CHAR(1)] = func_0204b81c(uiArc, func_0202d814(), FALSE, 0, wk->heapId);
    wk->clResources[PSTA_RES_CELL(1)] = func_0204bde0(uiArc, func_0202d818(2), func_0202d81c(2), wk->heapId);
    wk->clResources[PSTA_RES_PLTT(9)] = func_0204bbb8(uiArc, func_0202d964(), 0, 0xa0, 0, 1, wk->heapId);
    wk->clResources[PSTA_RES_CHAR(10)] = func_0204b81c(uiArc, func_0202d968(2), FALSE, 0, wk->heapId);
    wk->clResources[PSTA_RES_CELL(9)] = func_0204bde0(uiArc, func_0202d96c(2), func_0202d970(2), wk->heapId);
    wk->clResources[PSTA_RES_PLTT(2)] = func_0204bbb8(uiArc, func_0202d944(), 0, 0xc0, 0, 1, wk->heapId);
    wk->clResources[PSTA_RES_CHAR(3)] = func_0204b81c(uiArc, func_0202d948(2), FALSE, 0, wk->heapId);
    wk->clResources[PSTA_RES_CELL(3)] = func_0204bde0(uiArc, func_0202d94c(2), func_0202d950(2), wk->heapId);
    wk->clResources[PSTA_RES_PLTT(10)] = func_0204bbb8(uiArc, func_0202d8b0(), 0, 0x80, 0, 1, wk->heapId);
    wk->clResources[PSTA_RES_CHAR(11)] = func_0204b81c(uiArc, func_0202d8b4(), FALSE, 0, wk->heapId);
    wk->clResources[PSTA_RES_CELL(10)] = func_0204bde0(uiArc, func_0202d8b8(2), func_0202d8bc(2), wk->heapId);
    wk->clResources[PSTA_RES_PLTT(1)] = func_0204bbb8(uiArc, func_0202d91c(1), 0, 0xe0, 0, 1, wk->heapId);
    wk->clResources[PSTA_RES_CHAR(2)] = func_0204b81c(uiArc, func_0202d928(1), FALSE, 0, wk->heapId);
    wk->clResources[PSTA_RES_CELL(2)] = func_0204bde0(uiArc, func_0202d934(1, 2), func_0202d93c(1, 2), wk->heapId);
    GFL_ArcToolFree(uiArc);

    PStaSub_LoadResources(wk, wk->sub, arc);
    PStaInfo_LoadResources(wk, wk->info, arc);
    PStaSkill_LoadResources(wk, wk->skill, arc);
    PStaRibbon_LoadResources(wk, wk->ribbon, arc);
    GFL_ArcToolFree(arc);
}

static void PStatus_FreeResources(PStatusWork *wk) {
    u8 i;

    PStaRibbon_FreeResources(wk, wk->ribbon);
    PStaSkill_FreeResources(wk, wk->skill);
    PStaInfo_FreeResources(wk, wk->info);
    PStaSub_FreeResources(wk, wk->sub);
    for (i = PSTA_RES_PLTT(0); i <= PSTA_RES_PLTT(11); i++) {
        func_0204bcd0(wk->clResources[i]);
    }
    for (i = PSTA_RES_CHAR(0); i <= PSTA_RES_CHAR(12); i++) {
        func_0204b98c(wk->clResources[i]);
    }
    for (i = PSTA_RES_CELL(0); i <= PSTA_RES_CELL(12); i++) {
        func_0204be64(wk->clResources[i]);
    }
    for (i = 0; i < PSTA_TYPE_COUNT; i++) {
        func_0204b98c(wk->typeIconChars[i]);
    }
}

static void PStatus_CreateActors(PStatusWork *wk) {
    u8 sequences[PSTA_BUTTON_COUNT] = { 0, 1, 2, 6, 3, 2, 0, 1 };
    u8 xs[PSTA_BUTTON_COUNT] = { 0, 40, 80, 124, 144, 168, 200, 232 };
    ClActorSetup setup;
    u8 i;
    u32 chars;
    u32 cellAnims;

    setup.priority = 10;
    setup.bgPriority = 0;
    for (i = 0; i < PSTA_BUTTON_COUNT; i++) {
        if (i <= PSTA_BUTTON_RIBBON) {
            chars = wk->clResources[PSTA_RES_CHAR(0)];
            cellAnims = wk->clResources[PSTA_RES_CELL(0)];
        } else {
            chars = wk->clResources[PSTA_RES_CHAR(1)];
            cellAnims = wk->clResources[PSTA_RES_CELL(1)];
        }
        setup.x = xs[i];
        setup.y = i == PSTA_BUTTON_SHORTCUT ? 172 : 168;
        setup.sequence = sequences[i];
        wk->buttons[i] =
            func_0204c040(wk->actorUnit, chars, wk->clResources[PSTA_RES_PLTT(0)], cellAnims, &setup, 0, wk->heapId);
        func_0204c520(wk->buttons[i], TRUE);
        func_0204c124(wk->buttons[i], TRUE);
    }
    setup.x = 0;
    setup.y = 0;
    setup.sequence = 0;
    wk->typeIcons[0] = func_0204c040(wk->actorUnit, wk->typeIconChars[0], wk->clResources[PSTA_RES_PLTT(3)],
                                     wk->clResources[PSTA_RES_CELL(4)], &setup, 0, wk->heapId);
    wk->typeIcons[1] = func_0204c040(wk->actorUnit, wk->typeIconChars[0], wk->clResources[PSTA_RES_PLTT(3)],
                                     wk->clResources[PSTA_RES_CELL(4)], &setup, 0, wk->heapId);
    for (i = 0; i < 2; i++) {
        func_0204c124(wk->typeIcons[i], FALSE);
    }
    if (wk->param->mode == PSTATUS_MODE_FORGET_MOVE) {
        for (i = 0; i < PSTA_BUTTON_COUNT; i++) {
            if (i != PSTA_BUTTON_BACK) {
                func_0204c124(wk->buttons[i], FALSE);
            }
        }
    }
    if (PStatus_IsFromFieldMenu(wk) == FALSE) {
        func_0204c124(wk->buttons[PSTA_BUTTON_CLOSE], FALSE);
        func_0204c124(wk->buttons[PSTA_BUTTON_SHORTCUT], FALSE);
    }
    PStaSub_CreateActors(wk, wk->sub);
    PStaRibbon_CreateActors(wk, wk->ribbon);
    PStaSkill_CreateActors(wk, wk->skill);
}

static void PStatus_FreeActors(PStatusWork *wk) {
    u8 i;

    PStaRibbon_FreeActors(wk, wk->ribbon);
    PStaSkill_FreeActors(wk, wk->skill);
    PStaSub_FreeActors(wk, wk->sub);
    for (i = 0; i < PSTA_BUTTON_COUNT; i++) {
        func_0204c108(wk->buttons[i]);
    }
    for (i = 0; i < 2; i++) {
        func_0204c108(wk->typeIcons[i]);
    }
}

static void PStatus_InitText(PStatusWork *wk) {
    GXRgb *pltt;

    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0179, wk->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 0x1c0, 0x20, wk->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 0x1c0, 0x20, wk->heapId);
    pltt = (GXRgb *)(HW_DB_BG_PLTT + 0x1c0);
    pltt[9] = GX_RGB(25, 18, 19);
    pltt[10] = GX_RGB(18, 18, 26);
    wk->printQueue = func_020219a8(0x1000, wk->heapId);
}

static void PStatus_ExitText(PStatusWork *wk) {
    func_02021c44(wk->printQueue);
    func_02021a18(wk->printQueue);
    GFL_MsgDataFree(wk->msgData);
    GFL_FontFree(wk->font);
}

static void PStatus_CheckTouch(PStatusWork *wk) {
    TouchRect rects[PSTA_BUTTON_COUNT + 1] = {
        { 168, 192, 0, 40 },    { 168, 192, 40, 80 },   { 168, 192, 80, 120 },
        { 168, 192, 120, 144 }, { 168, 192, 144, 168 }, { 168, 192, 168, 192 },
        { 168, 192, 200, 224 }, { 168, 192, 232, 0 }, // The right edge is 256, which wraps to 0   { TOUCH_RECT_END, 0,
                                                      // 0, 0 },
    };

    wk->touchHit = func_0203da0c(rects);
}

static void PStatus_HandleInput(PStatusWork *wk) {
    if (wk->isInputEnabled == TRUE && func_02021c0c(wk->printQueue) == TRUE && PStatus_HandleKeys(wk) == FALSE) {
        PStatus_HandleTouch(wk);
    }
}

static BOOL PStatus_HandleKeys(PStatusWork *wk) {
    int lastPage;

    if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
        if (PStatus_ChangePokemon(wk, PSTA_DIR_DOWN) == TRUE) {
            PStatus_StartRedraw(wk);
            func_0204c488(wk->buttons[PSTA_BUTTON_UP], 3);
            func_0204c488(wk->buttons[PSTA_BUTTON_DOWN], 10);
            wk->upPressed = FALSE;
            wk->downPressed = TRUE;
            GFL_SndSEPlay(SEQ_SE_SYS_37);
            wk->isTouch = FALSE;
            return TRUE;
        }
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
        if (PStatus_ChangePokemon(wk, PSTA_DIR_UP) == TRUE) {
            PStatus_StartRedraw(wk);
            func_0204c488(wk->buttons[PSTA_BUTTON_UP], 11);
            func_0204c488(wk->buttons[PSTA_BUTTON_DOWN], 2);
            wk->upPressed = TRUE;
            wk->downPressed = FALSE;
            GFL_SndSEPlay(SEQ_SE_SYS_37);
            wk->isTouch = FALSE;
            return TRUE;
        }
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_RIGHT) {
        lastPage = PSTATUS_PAGE_RIBBON;
        if (wk->hasRibbon == FALSE) {
            lastPage = PSTATUS_PAGE_SKILL;
        }
        if (wk->page < lastPage && wk->isEgg == FALSE) {
            wk->page++;
            PStatus_StartRedraw(wk);
            GFL_SndSEPlay(SEQ_SE_SYS_38);
            wk->isTouch = FALSE;
            return TRUE;
        }
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_LEFT) {
        if (wk->page > PSTATUS_PAGE_INFO && wk->isEgg == FALSE) {
            wk->page--;
            PStatus_StartRedraw(wk);
            GFL_SndSEPlay(SEQ_SE_SYS_38);
            wk->isTouch = FALSE;
            return TRUE;
        }
    } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        wk->exitResult = PSTATUS_RESULT_BACK;
        wk->param->result = PSTATUS_RESULT_BACK;
        wk->seq = PSTA_SEQ_EXIT;
        func_0204c488(wk->buttons[PSTA_BUTTON_BACK], 9);
        wk->pressedButton = wk->buttons[PSTA_BUTTON_BACK];
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        wk->isTouch = FALSE;
        return TRUE;
    } else if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) && PStatus_IsFromFieldMenu(wk) == TRUE) {
        wk->exitResult = PSTATUS_RESULT_CLOSE;
        wk->param->result = PSTATUS_RESULT_CLOSE;
        wk->seq = PSTA_SEQ_EXIT;
        func_0204c488(wk->buttons[PSTA_BUTTON_CLOSE], 8);
        wk->pressedButton = wk->buttons[PSTA_BUTTON_CLOSE];
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        wk->isTouch = FALSE;
        return TRUE;
    } else if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_Y) && PStatus_IsFromFieldMenu(wk) == TRUE) {
        if (wk->page < PSTATUS_PAGE_FORGET) {
            if (wk->shortcutRegistered[wk->page] == TRUE) {
                wk->shortcutRegistered[wk->page] = FALSE;
                func_0204c488(wk->buttons[PSTA_BUTTON_SHORTCUT], 6);
            } else {
                wk->shortcutRegistered[wk->page] = TRUE;
                func_0204c488(wk->buttons[PSTA_BUTTON_SHORTCUT], 7);
            }
            GFL_SndSEPlay(SEQ_SE_SYS_07);
            wk->isTouch = FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

static void PStatus_HandleTouch(PStatusWork *wk) {
    switch (wk->touchHit) {
    case PSTA_BUTTON_INFO:
        if (wk->page != PSTATUS_PAGE_INFO) {
            wk->page = PSTATUS_PAGE_INFO;
            PStatus_StartRedraw(wk);
            GFL_SndSEPlay(SEQ_SE_SYS_38);
            wk->isTouch = TRUE;
        }
        break;
    case PSTA_BUTTON_SKILL:
        if (wk->page != PSTATUS_PAGE_SKILL && wk->isEgg == FALSE) {
            wk->page = PSTATUS_PAGE_SKILL;
            PStatus_StartRedraw(wk);
            GFL_SndSEPlay(SEQ_SE_SYS_38);
            wk->isTouch = TRUE;
        }
        break;
    case PSTA_BUTTON_RIBBON:
        if (wk->page != PSTATUS_PAGE_RIBBON && wk->isEgg == FALSE && wk->hasRibbon == TRUE) {
            wk->page = PSTATUS_PAGE_RIBBON;
            PStatus_StartRedraw(wk);
            GFL_SndSEPlay(SEQ_SE_SYS_38);
            wk->isTouch = TRUE;
        }
        break;
    case PSTA_BUTTON_SHORTCUT:
        if (wk->page < PSTATUS_PAGE_FORGET && PStatus_IsFromFieldMenu(wk) == TRUE) {
            if (wk->shortcutRegistered[wk->page] == TRUE) {
                wk->shortcutRegistered[wk->page] = FALSE;
                func_0204c488(wk->buttons[PSTA_BUTTON_SHORTCUT], 6);
            } else {
                wk->shortcutRegistered[wk->page] = TRUE;
                func_0204c488(wk->buttons[PSTA_BUTTON_SHORTCUT], 7);
            }
            GFL_SndSEPlay(SEQ_SE_SYS_07);
            wk->isTouch = TRUE;
        }
        break;
    case PSTA_BUTTON_UP:
        if (PStatus_ChangePokemon(wk, PSTA_DIR_UP) == TRUE) {
            PStatus_StartRedraw(wk);
            func_0204c488(wk->buttons[PSTA_BUTTON_UP], 11);
            func_0204c488(wk->buttons[PSTA_BUTTON_DOWN], 2);
            wk->upPressed = TRUE;
            wk->downPressed = FALSE;
            GFL_SndSEPlay(SEQ_SE_SYS_37);
            wk->isTouch = TRUE;
        }
        break;
    case PSTA_BUTTON_DOWN:
        if (PStatus_ChangePokemon(wk, PSTA_DIR_DOWN) == TRUE) {
            PStatus_StartRedraw(wk);
            func_0204c488(wk->buttons[PSTA_BUTTON_UP], 3);
            func_0204c488(wk->buttons[PSTA_BUTTON_DOWN], 10);
            wk->upPressed = FALSE;
            wk->downPressed = TRUE;
            GFL_SndSEPlay(SEQ_SE_SYS_37);
            wk->isTouch = TRUE;
        }
        break;
    case PSTA_BUTTON_CLOSE:
        if (PStatus_IsFromFieldMenu(wk) == TRUE) {
            wk->exitResult = PSTATUS_RESULT_CLOSE;
            wk->param->result = PSTATUS_RESULT_CLOSE;
            wk->seq = PSTA_SEQ_EXIT;
            func_0204c488(wk->buttons[PSTA_BUTTON_CLOSE], 8);
            wk->pressedButton = wk->buttons[PSTA_BUTTON_CLOSE];
            GFL_SndSEPlay(SEQ_SE_CLOSE1);
            wk->isTouch = TRUE;
        }
        break;
    case PSTA_BUTTON_BACK:
        wk->exitResult = PSTATUS_RESULT_BACK;
        wk->param->result = PSTATUS_RESULT_BACK;
        wk->seq = PSTA_SEQ_EXIT;
        func_0204c488(wk->buttons[PSTA_BUTTON_BACK], 9);
        wk->pressedButton = wk->buttons[PSTA_BUTTON_BACK];
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        wk->isTouch = TRUE;
        break;
    }
}

static void PStatus_AnimatePalettes(PStatusWork *wk) {
    f32 ratio;
    u8 i;
    s8 r;
    s8 g;
    s8 b;
    s8 endR;
    s8 endG;
    s8 endB;

    if (wk->plttAnimPhase + 0x400 >= 0x10000) {
        wk->plttAnimPhase = wk->plttAnimPhase + 0x400 - 0x10000;
    } else {
        wk->plttAnimPhase += 0x400;
    }
    {
        u16 startColors[4] = { GX_RGB(20, 20, 22), GX_RGB(16, 16, 18), GX_RGB(12, 12, 14), GX_RGB(13, 13, 14) };
        u16 endColors[4] = { GX_RGB(31, 31, 31), GX_RGB(31, 31, 31), GX_RGB(18, 18, 23), GX_RGB(17, 17, 20) };

        ratio = (f32)((FX_SinIdx(wk->plttAnimPhase) + FX32_ONE) / 2) / 4096.0f;

        for (i = 0; i < 4; i++) {
            r = startColors[i] & GX_RGB_R_MASK;
            g = (startColors[i] & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
            b = (startColors[i] & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
            r = r + ratio * ((s8)(endColors[i] & GX_RGB_R_MASK) - r);
            g = g + ratio * ((s8)((endColors[i] & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT) - g);
            b = b + ratio * ((s8)((endColors[i] & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT) - b);
            if (r < 0) {
                r = 0;
            }
            if (g < 0) {
                g = 0;
            }
            if (b < 0) {
                b = 0;
            }
            if (r > 31) {
                r = 31;
            }
            if (g > 31) {
                g = 31;
            }
            if (b > 31) {
                b = 31;
            }
            wk->cursorPltt[4 + i] = GX_RGB(r, g, b);
        }
        NNS_GfdRegisterNewVramTransferTask(14, 0x6c, &wk->cursorPltt[4], 8);
    }
    for (i = 0; i < 16; i++) {
        r = wk->tabPlttSrc[0][i] & GX_RGB_R_MASK;
        g = (wk->tabPlttSrc[0][i] & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
        b = (wk->tabPlttSrc[0][i] & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
        r = r + ratio * ((s8)(wk->tabPlttSrc[1][i] & GX_RGB_R_MASK) - r);
        g = g + ratio * ((s8)((wk->tabPlttSrc[1][i] & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT) - g);
        b = b + ratio * ((s8)((wk->tabPlttSrc[1][i] & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT) - b);
        if (r < 0) {
            r = 0;
        }
        if (g < 0) {
            g = 0;
        }
        if (b < 0) {
            b = 0;
        }
        if (r > 31) {
            r = 31;
        }
        if (g > 31) {
            g = 31;
        }
        if (b > 31) {
            b = 31;
        }
        wk->tabPltt[i] = GX_RGB(r, g, b);
    }
    NNS_GfdRegisterNewVramTransferTask(14, 0x180, wk->tabPltt, sizeof(wk->tabPltt));

    for (i = 12; i <= 14; i++) {
        r = wk->buttonPlttSrc[i] & GX_RGB_R_MASK;
        g = (wk->buttonPlttSrc[i] & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
        b = (wk->buttonPlttSrc[i] & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
        endR = r + 10;
        endG = g + 10;
        endB = b + 10;
        if (endR > 31) {
            endR = 31;
        }
        if (endG > 31) {
            endG = 31;
        }
        if (endB > 31) {
            endB = 31;
        }
        r = r + ratio * (endR - r);
        g = g + ratio * (endG - g);
        b = b + ratio * (endB - b);
        if (r < 0) {
            r = 0;
        }
        if (g < 0) {
            g = 0;
        }
        if (b < 0) {
            b = 0;
        }
        if (r > 31) {
            r = 31;
        }
        if (g > 31) {
            g = 31;
        }
        if (b > 31) {
            b = 31;
        }
        wk->buttonPltt[i] = GX_RGB(r, g, b);
    }
    NNS_GfdRegisterNewVramTransferTask(14, 0x1a0, wk->buttonPltt, sizeof(wk->buttonPltt));
}

void PStatus_EnableInput(PStatusWork *wk, BOOL enable) {
    wk->isInputEnabled = enable;
    if (enable == FALSE) {
        func_0204c124(wk->buttons[PSTA_BUTTON_INFO], FALSE);
        func_0204c124(wk->buttons[PSTA_BUTTON_SKILL], FALSE);
        func_0204c124(wk->buttons[PSTA_BUTTON_RIBBON], FALSE);
        func_0204c124(wk->buttons[PSTA_BUTTON_SHORTCUT], FALSE);
        func_0204c124(wk->buttons[PSTA_BUTTON_UP], FALSE);
        func_0204c124(wk->buttons[PSTA_BUTTON_DOWN], FALSE);
        func_0204c124(wk->buttons[PSTA_BUTTON_CLOSE], FALSE);
    } else {
        func_0204c124(wk->buttons[PSTA_BUTTON_INFO], TRUE);
        func_0204c124(wk->buttons[PSTA_BUTTON_SKILL], TRUE);
        if (wk->hasRibbon == TRUE) {
            func_0204c124(wk->buttons[PSTA_BUTTON_RIBBON], TRUE);
        }
        func_0204c124(wk->buttons[PSTA_BUTTON_UP], TRUE);
        func_0204c124(wk->buttons[PSTA_BUTTON_DOWN], TRUE);
        if (PStatus_IsFromFieldMenu(wk) == TRUE) {
            func_0204c124(wk->buttons[PSTA_BUTTON_SHORTCUT], TRUE);
            func_0204c124(wk->buttons[PSTA_BUTTON_CLOSE], TRUE);
        }
    }
}

static BOOL PStatus_ChangePokemon(PStatusWork *wk, u8 dir) {
    u16 index;
    BOOL found = PStatus_FindPokemon(wk, dir, &index);

    if (found == TRUE) {
        wk->partyIndex = index;
        if (wk->param->dataType == PSTATUS_DATA_BOX) {
            if (wk->boxPartyPkm != NULL) {
                PokeParty_ClearPkm(wk->boxPartyPkm);
                GFL_HeapFree(wk->boxPartyPkm);
            }
            wk->boxPartyPkm = boxPkmRegenToPartyPkm(PStatus_GetBoxPkm(wk), wk->heapId);
        }
        PStatus_LoadPokemon(wk);
    }
    return found;
}

static BOOL PStatus_FindPokemon(PStatusWork *wk, u8 dir, u16 *index) {
    BOOL done = FALSE;
    BOOL found = FALSE;
    u16 start = wk->partyIndex;
    BoxPkm *pkm;

    while (done == FALSE) {
        if (dir == PSTA_DIR_DOWN && wk->partyIndex >= wk->param->partyCount - 1) {
            done = TRUE;
        } else if (dir == PSTA_DIR_UP && wk->partyIndex == 0) {
            done = TRUE;
        } else {
            if (dir == PSTA_DIR_DOWN) {
                wk->partyIndex++;
            } else {
                wk->partyIndex--;
            }
            pkm = PStatus_GetBoxPkm(wk);
            if (PML_PkmGetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
                if (PML_PkmGetParam(pkm, PKM_PARAM_IS_EGG, NULL) != TRUE || wk->page == PSTATUS_PAGE_INFO) {
                    if (wk->page != PSTATUS_PAGE_RIBBON || PStatus_HasRibbon(wk, pkm) != FALSE) {
                        done = TRUE;
                        found = TRUE;
                    }
                }
            }
        }
    }
    if (index != NULL) {
        *index = wk->partyIndex;
    }
    wk->partyIndex = start;
    return found;
}

static void PStatus_LoadPokemon(PStatusWork *wk) {
    PartyPkm *pkm = PStatus_GetPartyPkm(wk);
    BoxPkm *boxPkm = PStatus_GetBoxPkm(wk);

    PStatus_SetDecrypted(wk, TRUE);
    wk->isEgg = PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL);
    wk->happiness = PokeParty_GetParam(pkm, PKM_PARAM_HAPPINESS, NULL);
    wk->hasRibbon = PStatus_HasRibbon(wk, boxPkm);
    PStatus_SetDecrypted(wk, FALSE);
    wk->playCry = TRUE;
}

static void PStatus_StartRedraw(PStatusWork *wk) {
    PStatus_SetDecrypted(wk, TRUE);
    if (wk->shownPartyIndex != wk->partyIndex) {
        if (wk->shownPartyIndex != 0xff) {
            PStaSub_Unload(wk, wk->sub);
            PStaRibbon_UnloadPokemon(wk, wk->ribbon);
        }
        PStaSub_Load(wk, wk->sub);
        PStaRibbon_LoadPokemon(wk, wk->ribbon);
    }
    switch (wk->shownPage) {
    case PSTATUS_PAGE_INFO:
        PStaInfo_Unload(wk, wk->info);
        break;
    case PSTATUS_PAGE_SKILL:
        PStaSkill_Unload(wk, wk->skill);
        break;
    case PSTATUS_PAGE_RIBBON:
        PStaRibbon_Unload(wk, wk->ribbon);
        break;
    case PSTATUS_PAGE_FORGET:
        PStaSkill_UnloadForget(wk, wk->skill);
        break;
    }
    switch (wk->page) {
    case PSTATUS_PAGE_INFO:
        PStaInfo_Load(wk, wk->info);
        break;
    case PSTATUS_PAGE_SKILL:
        PStaSkill_Load(wk, wk->skill);
        break;
    case PSTATUS_PAGE_RIBBON:
        PStaRibbon_Load(wk, wk->ribbon);
        break;
    case PSTATUS_PAGE_FORGET:
        PStaSkill_LoadForget(wk, wk->skill);
        break;
    }
    if (wk->shownPage != wk->page) {
        wk->mosaicState = PSTA_MOSAIC_IN;
        wk->mosaicLevel = 0;
        G2_BG2Mosaic(TRUE);
        G2S_BG1Mosaic(TRUE);
        G2S_BG2Mosaic(TRUE);
    }
    PStatus_SetDecrypted(wk, FALSE);
    wk->isRedrawing = TRUE;
}

static void PStatus_UpdateRedraw(PStatusWork *wk) {
    PartyPkm *pkm;
    u32 species;
    u32 form;
    PokeVoiceChatterInfo chatter;

    if (wk->mosaicState == PSTA_MOSAIC_IN) {
        wk->mosaicLevel++;
        G2_SetBGMosaicSize(wk->mosaicLevel, wk->mosaicLevel * 2);
        G2S_SetBGMosaicSize(wk->mosaicLevel, wk->mosaicLevel * 2);
        if (wk->mosaicLevel >= 3) {
            wk->mosaicState = PSTA_MOSAIC_REDRAW;
        }
    }
    if (wk->mosaicState == PSTA_MOSAIC_OUT) {
        wk->mosaicLevel--;
        G2_SetBGMosaicSize(wk->mosaicLevel, wk->mosaicLevel * 2);
        G2S_SetBGMosaicSize(wk->mosaicLevel, wk->mosaicLevel * 2);
        if (wk->mosaicLevel == 0) {
            wk->isRedrawing = FALSE;
            wk->mosaicState = PSTA_MOSAIC_NONE;
            return;
        }
    }
    if (func_02021c0c(wk->printQueue) != TRUE ||
        (wk->mosaicState != PSTA_MOSAIC_REDRAW && wk->mosaicState != PSTA_MOSAIC_NONE)) {
        return;
    }
    if (wk->shownPartyIndex != wk->partyIndex) {
        if (wk->shownPartyIndex != 0xff) {
            PStaSub_Clear(wk, wk->sub);
        }
        PStaSub_Draw(wk, wk->sub);
        wk->shownPartyIndex = wk->partyIndex;
    }
    switch (wk->shownPage) {
    case PSTATUS_PAGE_INFO:
        PStaInfo_Clear(wk, wk->info);
        break;
    case PSTATUS_PAGE_SKILL:
        PStaSkill_Clear(wk, wk->skill);
        break;
    case PSTATUS_PAGE_RIBBON:
        PStaRibbon_Clear(wk, wk->ribbon);
        break;
    case PSTATUS_PAGE_FORGET:
        PStaSkill_ClearForget(wk, wk->skill);
        break;
    }
    switch (wk->page) {
    case PSTATUS_PAGE_INFO:
        PStaInfo_Draw(wk, wk->info);
        break;
    case PSTATUS_PAGE_SKILL:
        PStaSkill_Draw(wk, wk->skill);
        break;
    case PSTATUS_PAGE_RIBBON:
        PStaRibbon_Draw(wk, wk->ribbon);
        break;
    case PSTATUS_PAGE_FORGET:
        PStaSkill_DrawForget(wk, wk->skill);
        break;
    }
    if (wk->isEgg == FALSE && wk->param->mode != PSTATUS_MODE_FORGET_MOVE) {
        func_0204c124(wk->buttons[PSTA_BUTTON_SKILL], TRUE);
        if (wk->hasRibbon == FALSE) {
            func_0204c124(wk->buttons[PSTA_BUTTON_RIBBON], FALSE);
        } else {
            func_0204c124(wk->buttons[PSTA_BUTTON_RIBBON], TRUE);
        }
    } else {
        func_0204c124(wk->buttons[PSTA_BUTTON_SKILL], FALSE);
        func_0204c124(wk->buttons[PSTA_BUTTON_RIBBON], FALSE);
    }
    if (wk->upPressed == FALSE) {
        if (PStatus_FindPokemon(wk, PSTA_DIR_UP, NULL) == TRUE) {
            func_0204c488(wk->buttons[PSTA_BUTTON_UP], 3);
        } else {
            func_0204c488(wk->buttons[PSTA_BUTTON_UP], 17);
        }
    }
    if (wk->downPressed == FALSE) {
        if (PStatus_FindPokemon(wk, PSTA_DIR_DOWN, NULL) == TRUE) {
            func_0204c488(wk->buttons[PSTA_BUTTON_DOWN], 2);
        } else {
            func_0204c488(wk->buttons[PSTA_BUTTON_DOWN], 16);
        }
    }
    if (wk->page < PSTATUS_PAGE_FORGET && PStatus_IsFromFieldMenu(wk) == TRUE) {
        if (wk->shortcutRegistered[wk->page] == TRUE) {
            func_0204c488(wk->buttons[PSTA_BUTTON_SHORTCUT], 7);
        } else {
            func_0204c488(wk->buttons[PSTA_BUTTON_SHORTCUT], 6);
        }
    }
    if (wk->playCry == TRUE) {
        wk->playCry = FALSE;
        PokeVoice_Release(0);
        if (wk->isEgg == FALSE) {
            pkm = PStatus_GetPartyPkm(wk);
            species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
            form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
            PokeVoice_CreateChatterInfo(&chatter);
            PokeVoice_Play(species, form, 64, 0, 0, 0, 0, &chatter);
        }
    }
    if (wk->mosaicState == PSTA_MOSAIC_REDRAW) {
        wk->mosaicState = PSTA_MOSAIC_OUT;
        wk->shownPage = wk->page;
    } else {
        wk->isRedrawing = FALSE;
        wk->shownPage = wk->page;
    }
}

BoxPkm *PStatus_GetBoxPkm(PStatusWork *wk) {
    switch (wk->param->dataType) {
    case PSTATUS_DATA_PARTY_PKM:
        return func_0201d620(wk->param->party);
    case PSTATUS_DATA_PARTY:
        return func_0201d620(PokeParty_GetPkm(wk->param->party, wk->partyIndex));
    case PSTATUS_DATA_BOX:
        return (BoxPkm *)((u8 *)wk->param->party + PML_GetPkmRawSize() * wk->partyIndex);
    }
    return NULL;
}

PartyPkm *PStatus_GetPartyPkm(PStatusWork *wk) {
    switch (wk->param->dataType) {
    case PSTATUS_DATA_PARTY_PKM:
        return wk->param->party;
    case PSTATUS_DATA_PARTY:
        return PokeParty_GetPkm(wk->param->party, wk->partyIndex);
    case PSTATUS_DATA_BOX:
        return wk->boxPartyPkm;
    }
    return NULL;
}

void PStatus_SetDecrypted(PStatusWork *wk, BOOL decrypt) {
    PartyPkm *pkm;
    BoxPkm *boxPkm;

    switch (wk->param->dataType) {
    case PSTATUS_DATA_PARTY_PKM:
        pkm = wk->param->party;
        if (decrypt == TRUE) {
            PokeParty_DecryptPkm(pkm);
        } else {
            PokeParty_EncryptPkm(pkm, TRUE);
        }
        break;
    case PSTATUS_DATA_PARTY:
        pkm = PokeParty_GetPkm(wk->param->party, wk->partyIndex);
        if (decrypt == TRUE) {
            PokeParty_DecryptPkm(pkm);
        } else {
            PokeParty_EncryptPkm(pkm, TRUE);
        }
        break;
    case PSTATUS_DATA_BOX:
        boxPkm = PStatus_GetBoxPkm(wk);
        if (decrypt == TRUE) {
            PokeParty_DecryptPkm(wk->boxPartyPkm);
            PML_PkmDecrypt(boxPkm);
        } else {
            PokeParty_EncryptPkm(wk->boxPartyPkm, TRUE);
            PML_PkmReEncrypt(boxPkm, TRUE);
        }
        break;
    }
}

void PStatus_Print(PStatusWork *wk, GFLBitmap *bitmap, u32 msgId, u16 x, u16 y, u16 color) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);

    func_02021c7c(wk->printQueue, bitmap, x, y, str, wk->font, color);
    GFL_StrBufFree(str);
}

void PStatus_PrintToWindow(PStatusWork *wk, BmpWin *window, u32 msgId, u16 x, u16 y, u16 color) {
    PStatus_Print(wk, BmpWin_GetBitmap(window), msgId, x, y, color);
}

void PStatus_PrintCentered(PStatusWork *wk, GFLBitmap *bitmap, u32 msgId, u16 x, u16 y, u16 color) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);
    u32 width = GFL_FontGetBlockWidth(str, wk->font, 0);

    func_02021c7c(wk->printQueue, bitmap, x - width / 2, y, str, wk->font, color);
    GFL_StrBufFree(str);
}

void PStatus_PrintFormatted(PStatusWork *wk, GFLBitmap *bitmap, WordSet *wordSet, u32 msgId, u16 x, u16 y, u16 color) {
    StrBuf *str = GFL_StrBufCreate(16, wk->heapId);
    StrBuf *format = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);

    GFL_WordSetFormatStrbuf(wordSet, str, format);
    func_02021c7c(wk->printQueue, bitmap, x, y, str, wk->font, color);
    GFL_StrBufFree(format);
    GFL_StrBufFree(str);
}

void PStatus_PrintFormattedToWindow(PStatusWork *wk, BmpWin *window, WordSet *wordSet, u32 msgId, u16 x, u16 y,
                                    u16 color) {
    PStatus_PrintFormatted(wk, BmpWin_GetBitmap(window), wordSet, msgId, x, y, color);
}

void PStatus_PrintFormattedRight(PStatusWork *wk, GFLBitmap *bitmap, WordSet *wordSet, u32 msgId, u16 x, u16 y,
                                 u16 color) {
    StrBuf *str = GFL_StrBufCreate(16, wk->heapId);
    StrBuf *format = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);
    u32 width;

    GFL_WordSetFormatStrbuf(wordSet, str, format);
    width = GFL_FontGetBlockWidth(str, wk->font, 0);
    func_02021c7c(wk->printQueue, bitmap, x - width, y, str, wk->font, color);
    GFL_StrBufFree(format);
    GFL_StrBufFree(str);
}

void PStatus_PrintFormattedRightToWindow(PStatusWork *wk, BmpWin *window, WordSet *wordSet, u32 msgId, u16 x, u16 y,
                                         u16 color) {
    PStatus_PrintFormattedRight(wk, BmpWin_GetBitmap(window), wordSet, msgId, x, y, color);
}

static BOOL PStatus_IsFromFieldMenu(PStatusWork *wk) {
    if (wk->param->fromFieldMenu == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL PStatus_HasRibbon(PStatusWork *wk, BoxPkm *pkm) {
    u8 i;

    for (i = 0; i < RIBBON_COUNT; i++) {
        if (PML_PkmGetParam(pkm, Ribbon_GetData(i, RIBBON_DATA_PARAM), NULL) == TRUE) {
            return TRUE;
        }
    }
    return FALSE;
}
