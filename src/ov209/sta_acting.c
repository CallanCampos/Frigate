#include "types.h"
#include "app/musical/mus_item_draw.h"
#include "app/musical/mus_poke_draw.h"
#include "app/musical/sta_act_audience.h"
#include "app/musical/sta_act_bg.h"
#include "app/musical/sta_act_button.h"
#include "app/musical/sta_act_effect.h"
#include "app/musical/sta_act_light.h"
#include "app/musical/sta_act_obj.h"
#include "app/musical/sta_act_poke.h"
#include "app/musical/sta_act_script.h"
#include "app/musical/sta_acting.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "field/musical.h"
#include "field/musical_program.h"
#include "field/musical_stage_sys.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/blact.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wipe.h"

// Overlay 209's sta_acting.c: the musical's stage. It plays the program's scripts with the Pokémon, the background,
// the objects, the effects, the lights and the audience, follows the Pokémon in the limelight, and runs the props
// the Pokémon use, the player's from the buttons and the others' at random or over the connection

// The Pokémon in the limelight and the light the stage follows, when there is none
#define STA_ACTING_NO_POKE 4
#define STA_ACTING_NO_LIGHT 4

// The props held in each hand, whose use the other Pokémon plan
#define STA_ACTING_EQUIP_HAND_R 7
#define STA_ACTING_EQUIP_HAND_L 8
// What the communication reports when a Pokémon uses no prop
#define STA_ACTING_EQUIP_NONE 10

// MusicalPoke.unk78 of a Pokémon that uses a hand prop when it appeals, and at random, often or now and then;
// smaller values are the frame to use it on
#define STA_ACTING_USE_ON_APPEAL 0xfffd
#define STA_ACTING_USE_SOMETIMES 0xfffe
#define STA_ACTING_USE_OFTEN 0xffff

// How far the stage scrolls, and the curtain opens
#define STA_ACTING_SCROLL_MAX 256
#define STA_ACTING_CURTAIN_OPEN 0xe0

// The frames a Pokémon stays in the limelight after it uses a prop
#define STA_ACTING_LIGHT_UP_FRAMES 180

enum {
    STA_ACTING_SEQ_FADE_IN,
    STA_ACTING_SEQ_WAIT_FADE_IN,
    STA_ACTING_SEQ_FADE_OUT,
    STA_ACTING_SEQ_WAIT_FADE_OUT,
    STA_ACTING_SEQ_WAIT,
    STA_ACTING_SEQ_START,
    STA_ACTING_SEQ_PLAY,
    STA_ACTING_SEQ_SYNC,
    STA_ACTING_SEQ_END = 0xff,
};

struct StaActing {
    HeapID heapId;
    u16 waitCount;
    u16 scrollOffset;
    u16 scrollTarget;
    u16 curtainOffset;
    // The position of the player's Pokémon
    u8 playerPos;
    // Until the curtain is open, the stage doesn't follow the Pokémon
    BOOL curtainClosed;
    BOOL scrollFixed;
    BOOL scriptScroll;
    ArcTool *arc;
    u32 seq;
    TCB *vblankTcb;
    BOOL strmPlaying;
    BOOL seqPlaying;
    BOOL bgmPlaying;
    MusicalStageParam *param;
    MusPokeDrawSys *pokeDraw;
    MusItemDrawSys *itemDraw;
    StaActBg *bg;
    StaActPokeSys *pokeSys;
    StaActPoke *pokes[4];
    StaActObjSys *objSys;
    StaActObj *objs[5];
    StaActEffectSys *effectSys;
    StaActEffect *effects[8];
    StaActLightSys *lightSys;
    StaActLight *lights[4];
    StaActButton *button;
    G3DCamera *camera;
    BlActScene *blact;
    StaActScriptSys *scriptSys;
    u32 *script;
    StaActAudience *audience;
    StaActEffect *appealEffect;
    u32 unkC0[3];
    TCBExManager *tcbExMgr;
    BmpWin *msgWin;
    Font *font;
    PrintStream *printStream;
    MsgData *msgData;
    StrBuf *msgStr;
    // Whether a message waits in the print queue
    BOOL printing;
    PrintQueue *printQueue;
    // Whether the audience is to look again at what is happening
    BOOL audienceRefresh;
    u8 audienceFocusPos;
    // The Pokémon or the light the stage and the audience follow
    u8 followPos;
    u8 followLight;
    // How many Pokémon have more points than each
    u8 ranks[4];
    // The points each Pokémon won with its props during the show
    u8 bonusPoints[4];
    BOOL useRequest[4];
    u32 useEquip[4];
    // The Pokémon in the limelight after it used a prop, which one, for how long, and whether it scores then
    u8 lightUpPos;
    u8 lightUpEquip;
    u16 lightUpTimer;
    BOOL lightUpScores;
    // The frames until each of the other Pokémon uses a prop, and which
    u16 npcUseWait[4];
    u32 npcUseEquip[4];
    BOOL handPropUsed[4][2];
    // The player's prop to send over the connection
    BOOL commSendPending;
    u32 commSendEquip;
    u32 unk164;
    // The shimmer of the sub screen's palette, between its colors and those of the next palette
    u16 paletteAnimPhase;
    u16 paletteBase[16];
    s8 paletteDiffR[16];
    s8 paletteDiffG[16];
    s8 paletteDiffB[16];
    u16 paletteWork[16];
    void *sound[3];
    // The frames since the last update
    u32 updateCount;
    u32 lastVBlankCount;
};

static void StaActing_VBlank(TCB *tcb, void *work);
static void StaActing_InitGraphics(StaActing *stage);
static void StaActing_InitBg(StaActing *stage);
static void StaActing_LoadBg(StaActing *stage, u32 unused);
static void StaActing_InitBG(const BGSetup *setup, u8 bg, u8 mode);
static void StaActing_InitPoke(StaActing *stage);
static void StaActing_InitObj(StaActing *stage);
static void StaActing_InitEffect(StaActing *stage);
static void StaActing_UpdateScroll(StaActing *stage);
static void StaActing_StartScripts(StaActing *stage);
static void StaActing_UpdatePalette(StaActing *stage);
static void StaActing_InitMessage(StaActing *stage);
static void StaActing_UpdateMessage(StaActing *stage);
static void StaActing_UpdateAudience(StaActing *stage);
static void StaActing_UpdateRanks(StaActing *stage);
static void StaActing_RequestItemUse(StaActing *stage, u8 pos, u32 equip);
static BOOL StaActing_IsPokeUsingItem(StaActing *stage, u8 pos);
static void StaActing_UpdateItemUse(StaActing *stage);
static u32 StaActing_ScoreItem(StaActing *stage, u8 pos, u8 equip);
static void StaActing_UpdateNpcItemUse(StaActing *stage);
static void StaActing_InitAppealEffect(StaActing *stage);
static void StaActing_TermAppealEffect(StaActing *stage);
static void StaActing_DrawDebug(StaActing *stage);

// Read by StaActing_Main, which runs no script while it is set; nothing sets it
static BOOL sStaActingPause;

static const BGSysVRAMConfig STA_ACTING_VRAM_CONFIG = {
    GX_VRAM_BG_128_D,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_16_F,  GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_01_AB, GX_VRAM_TEXPLTT_0123_E,  GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const VecFx32 STA_ACTING_CAMERA_UP = { 0, FX32_ONE, 0 };
static const VecFx32 STA_ACTING_CAMERA_TARGET = { 0, 0, 0 };
static const VecFx32 STA_ACTING_CAMERA_POS = { 0, 0, FX32_CONST(301) };

static const GXRgb STA_ACTING_EDGE_COLORS[8] = { 0 };

static const BGSysLCDConfig STA_ACTING_LCD_CONFIG = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };

static const BGSetup STA_ACTING_BG5_SETUP = {
    0,
    0,
    0x1000,
    0,
    BGRES_512x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x7000),
    GX_BG_CHARBASE(0x08000),
    0x8000,
    GX_BG_EXTPLTT_01,
    3,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static const BGSetup STA_ACTING_BG6_SETUP = {
    0,
    0,
    0x1000,
    0,
    BGRES_512x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x6000),
    GX_BG_CHARBASE(0x10000),
    0x8000,
    GX_BG_EXTPLTT_01,
    2,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static const BGSetup STA_ACTING_BG4_SETUP = {
    0,
    0,
    0x1000,
    0,
    BGRES_512x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x5000),
    GX_BG_CHARBASE(0x18000),
    0x8000,
    GX_BG_EXTPLTT_01,
    1,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static const BGSetup STA_ACTING_BG7_SETUP = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x4800),
    GX_BG_CHARBASE(0x00000),
    0x1000,
    GX_BG_EXTPLTT_01,
    0,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static const BGSetup STA_ACTING_BG1_SETUP = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x4800),
    GX_BG_CHARBASE(0x08000),
    0x8000,
    GX_BG_EXTPLTT_01,
    0,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static const BGSetup STA_ACTING_BG2_SETUP = {
    0,
    0,
    0x2000,
    0,
    BGRES_512x512,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x5000),
    GX_BG_CHARBASE(0x18000),
    0x8000,
    GX_BG_EXTPLTT_01,
    1,
    GX_BG_AREAOVER_REPEAT,
    FALSE,
};

static const BGSetup STA_ACTING_BG3_SETUP = {
    0,
    0,
    0x1000,
    0,
    BGRES_512x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x7000),
    GX_BG_CHARBASE(0x10000),
    0x8000,
    GX_BG_EXTPLTT_23,
    2,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

StaActing *StaActing_Init(MusicalStageParam *param, HeapID heapId) {
    u8 i;
    StaActing *stage = GFL_HeapAllocate(heapId, sizeof(StaActing), TRUE, "sta_acting.c", 282);

    stage->heapId = heapId;
    stage->param = param;
    stage->waitCount = 0;
    stage->scrollOffset = 127;
    stage->scrollTarget = 128;
    stage->curtainOffset = 0;
    stage->script = NULL;
    stage->seq = STA_ACTING_SEQ_FADE_IN;
    stage->unk164 = 0;
    stage->curtainClosed = TRUE;
    stage->scriptScroll = FALSE;
    stage->scrollFixed = FALSE;
    stage->strmPlaying = FALSE;
    stage->seqPlaying = FALSE;
    stage->bgmPlaying = FALSE;
    stage->vblankTcb = GFL_VBlankTCBAdd(StaActing_VBlank, stage, 64);
    if (stage->param->comm == NULL) {
        for (i = 0; i < 4; i++) {
            if (stage->param->pokes[i]->owner == 0) {
                stage->playerPos = i;
                break;
            }
        }
        if (i == 4) {
            stage->playerPos = 1;
        }
    } else {
        stage->playerPos = func_ov211_021f0470(stage->param->comm);
    }
    for (i = 0; i < 4; i++) {
        stage->bonusPoints[i] = 0;
    }
    StaActing_UpdateRanks(stage);
    stage->audienceRefresh = TRUE;
    stage->audienceFocusPos = STA_ACTING_NO_POKE;
    stage->followPos = STA_ACTING_NO_POKE;
    stage->followLight = STA_ACTING_NO_LIGHT;
    stage->lightUpPos = STA_ACTING_NO_POKE;
    stage->lightUpTimer = 0;
    stage->commSendPending = FALSE;
    stage->paletteAnimPhase = 0;
    for (i = 0; i < 4; i++) {
        stage->useRequest[i] = FALSE;
        stage->npcUseWait[i] = 0;
        stage->handPropUsed[i][0] = FALSE;
        stage->handPropUsed[i][1] = FALSE;
    }
    stage->arc = GFL_ArcSysCreateFileHandle(ARCID_MUSICAL, stage->heapId);
    StaActing_InitGraphics(stage);
    StaActing_InitBg(stage);
    StaActing_InitMessage(stage);
    StaActing_InitPoke(stage);
    StaActing_InitObj(stage);
    StaActing_InitEffect(stage);
    stage->lightSys = StaActLight_InitSystem(stage->heapId, stage);
    stage->audience = StaActAudience_InitSystem(stage->heapId, stage, stage->param);
    stage->button = StaActButton_InitSystem(stage->heapId, stage, stage->param->pokes[stage->playerPos]);
    stage->scriptSys = StaActScript_InitSystem(stage->heapId, stage);
    StaActing_InitAppealEffect(stage);
    func_02042ba8(FALSE, stage->heapId);
    StaActing_LoadBg(stage, 0);
    stage->sound[0] = stage->param->ov210->sound[0];
    stage->sound[1] = stage->param->ov210->sound[1];
    stage->sound[2] = stage->param->ov210->sound[2];
    stage->lastVBlankCount = OS_GetVBlankCount();
    return stage;
}

void StaActing_Term(StaActing *stage) {
    if (stage->bgmPlaying == TRUE) {
        func_02005d8c();
    }
    if (stage->seqPlaying == TRUE) {
        func_02006574();
    }
    if (stage->strmPlaying == TRUE) {
        func_02006588();
    }
    GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, -16);
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, -16);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, 0, 0, 31, 31);
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    StaActing_TermAppealEffect(stage);
    StaActScript_TermSystem(stage->scriptSys);
    if (stage->msgStr != NULL) {
        GFL_StrBufFree(stage->msgStr);
    }
    func_02021c44(stage->printQueue);
    func_02021a18(stage->printQueue);
    GFL_MsgDataFree(stage->msgData);
    BmpWin_Free(stage->msgWin);
    GFL_FontFree(stage->font);
    GFL_TCBExMgrFree(stage->tcbExMgr);
    StaActButton_TermSystem(stage->button);
    StaActAudience_TermSystem(stage->audience);
    StaActBg_TermSystem(stage->bg);
    StaActLight_TermSystem(stage->lightSys);
    StaActEffect_TermSystem(stage->effectSys);
    StaActObj_TermSystem(stage->objSys);
    StaActPoke_TermSystem(stage->pokeSys);
    MusPokeDraw_TermSystem(stage->pokeDraw);
    MusItemDraw_TermSystem(stage->itemDraw);
    func_0204b758();
    func_0204e450(stage->blact);
    GFL_G3DCameraFree(stage->camera);
    GFL_G3DSysFree();
    GFL_BGSysReleaseBG(0);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(3);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(7);
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    GFL_TCBRemove(stage->vblankTcb);
    GFL_ArcToolFree(stage->arc);
    GFL_HeapFree(stage);
}

// TRUE once the stage has faded out at the end
BOOL StaActing_Main(StaActing *stage) {
    u32 vblankCount = OS_GetVBlankCount();

    stage->updateCount = vblankCount - stage->lastVBlankCount;
    stage->lastVBlankCount = vblankCount;
    switch (stage->seq) {
    case STA_ACTING_SEQ_FADE_IN:
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, stage->heapId);
        stage->seq = STA_ACTING_SEQ_WAIT_FADE_IN;
        break;
    case STA_ACTING_SEQ_WAIT_FADE_IN:
        if (GFL_WipeIsFinished() == TRUE) {
            if (stage->param->comm != NULL) {
                stage->seq = STA_ACTING_SEQ_SYNC;
                func_ov211_021ef988(stage->param->comm, 200);
            } else {
                stage->seq = STA_ACTING_SEQ_WAIT;
            }
        }
        break;
    case STA_ACTING_SEQ_FADE_OUT:
        if (GFL_SndPlayerIsActiveAny() == FALSE) {
            GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, stage->heapId);
            stage->seq = STA_ACTING_SEQ_WAIT_FADE_OUT;
        }
        break;
    case STA_ACTING_SEQ_WAIT_FADE_OUT:
        if (GFL_WipeIsFinished() == TRUE) {
            return TRUE;
        }
        break;
    case STA_ACTING_SEQ_WAIT:
        if (stage->waitCount++ >= 60) {
            stage->seq = STA_ACTING_SEQ_START;
        }
        break;
    case STA_ACTING_SEQ_START:
        if (stage->unk164 == 0) {
            stage->script = stage->param->ov210->script;
            StaActing_StartScripts(stage);
            stage->seq = STA_ACTING_SEQ_PLAY;
        }
        break;
    case STA_ACTING_SEQ_PLAY:
        if (StaActScript_GetScriptNum(stage->scriptSys) == 0) {
            stage->script = NULL;
            if (stage->unk164 == 0) {
                stage->seq = STA_ACTING_SEQ_FADE_OUT;
            } else {
                stage->seq = STA_ACTING_SEQ_END;
            }
        }
        break;
    case STA_ACTING_SEQ_SYNC:
        if (func_ov211_021ef99c(stage->param->comm, 200) == TRUE) {
            stage->seq = STA_ACTING_SEQ_WAIT;
        }
        break;
    }
    StaActing_UpdateScroll(stage);
    StaActButton_UpdateSystem(stage->button);
    StaActing_UpdateItemUse(stage);
    if (sStaActingPause == FALSE) {
        StaActScript_UpdateSystem(stage->scriptSys);
    }
    StaActBg_UpdateSystem(stage->bg);
    StaActPoke_UpdateSystem(stage->pokeSys);
    StaActObj_UpdateSystem(stage->objSys);
    StaActEffect_UpdateSystem(stage->effectSys);
    StaActing_UpdateMessage(stage);
    MusPokeDraw_UpdateSystem(stage->pokeDraw);
    StaActLight_UpdateSystem(stage->lightSys);
    StaActing_UpdateAudience(stage);
    StaActAudience_UpdateSystem(stage->audience);
    StaActing_UpdatePalette(stage);
    GFL_G3DSysReset();
    GFL_G3DSysMtxViewFlush();
    StaActLight_DrawSystem(stage->lightSys);
    StaActPoke_DrawSystem(stage->pokeSys);
    MusPokeDraw_DrawSystem(stage->pokeDraw);
    StaActPoke_UpdateSystem_Item(stage->pokeSys);
    StaActBg_DrawSystem(stage->bg);
    BlActScene_Draw(stage->blact, stage->camera, NULL);
    StaActEffect_DrawSystem(stage->effectSys);
    StaActing_DrawDebug(stage);
    GFL_G3DSysReqSwapBuffers();
    func_0204b794();
    if (stage->param->comm != NULL && stage->seq != STA_ACTING_SEQ_FADE_IN &&
        stage->seq != STA_ACTING_SEQ_WAIT_FADE_IN && stage->seq != STA_ACTING_SEQ_FADE_OUT &&
        stage->seq != STA_ACTING_SEQ_WAIT_FADE_OUT && GFL_NetErrCheck() != FALSE) {
        stage->seq = STA_ACTING_SEQ_FADE_OUT;
    }
    return FALSE;
}

static void StaActing_VBlank(TCB *tcb, void *work) {
    func_0204b7c8();
}

static void StaActing_InitGraphics(StaActing *stage) {
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GX_SetVisiblePlane(0);
    GXS_SetVisiblePlane(0);
    Wipe_SetScreenCovered(0, WIPE_COLOR_BLACK);
    Wipe_SetScreenCovered(1, WIPE_COLOR_BLACK);
    Wipe_HideWindows(0);
    Wipe_HideWindows(1);
    GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
    GFL_BGSysSetVRAMBanks(&STA_ACTING_VRAM_CONFIG);
    GFL_BGSysCreate(stage->heapId);
    BmpWin_InitAllocator(stage->heapId);
    GFL_BGSysSetLCDConfig(&STA_ACTING_LCD_CONFIG);
    StaActing_InitBG(&STA_ACTING_BG1_SETUP, 1, 0);
    StaActing_InitBG(&STA_ACTING_BG2_SETUP, 2, 0);
    StaActing_InitBG(&STA_ACTING_BG3_SETUP, 3, 0);
    StaActing_InitBG(&STA_ACTING_BG5_SETUP, 5, 0);
    StaActing_InitBG(&STA_ACTING_BG6_SETUP, 6, 0);
    StaActing_InitBG(&STA_ACTING_BG4_SETUP, 4, 0);
    StaActing_InitBG(&STA_ACTING_BG7_SETUP, 7, 0);
    GFL_BGSysSetBGEnabled(0, TRUE);
    GFL_G3DSysCreate(FALSE, 2, FALSE, 4, 0, stage->heapId, NULL);
    GFL_BGSysSet3DBGPriority(3);
    GFL_G3DSysSetSwapBufferParams(0, 1);
    stage->camera =
        GFL_G3DCameraCreate(G3DCAM_PROJECTION_ORTHO, FX32_CONST(12), 0, 0, FX32_CONST(16), FX32_ONE, FX32_CONST(1000),
                            0, &STA_ACTING_CAMERA_POS, &STA_ACTING_CAMERA_UP, &STA_ACTING_CAMERA_TARGET, stage->heapId);
    GFL_G3DCameraFlush(stage->camera);
    gfxSetEdgeColorTable(STA_ACTING_EDGE_COLORS);
    G3X_EdgeMarking(FALSE);
    G3X_AntiAlias(TRUE);
    G3X_AlphaBlend(TRUE);
    GFL_G3DSysSetSwapBufferParams(0, 0);
    {
        BlActSceneSetup setup = { 128, 128, { FX32_ONE, FX32_ONE, FX32_ONE }, 0, 0, 0, 0, 63, 0 };
        VecFx32 scale = { FX32_CONST(4), FX32_CONST(4), FX32_ONE };

        stage->blact = BlActScene_Create(&setup, stage->heapId);
        BlActScene_SetScale(stage->blact, &scale);
    }
    ClActSys_Create(&data_02093f08, &STA_ACTING_VRAM_CONFIG, stage->heapId);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG3, GX_BLEND_PLANEMASK_BG0, 0, 10);
    GX_SetVisibleWnd(GX_WNDMASK_OW);
    G2_SetWndOBJInsidePlane(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ, TRUE);
    G2_SetWndOutsidePlane(GX_PLANEMASK_ALL, TRUE);
}

static void StaActing_InitBg(StaActing *stage) {
    u16 palette[16];
    u8 i;

    GFL_G2DIOLoadArcNCLR(stage->arc, 2, 0, 0x20, 0x20, 0x20, stage->heapId);
    GFL_BGSysLoadArcNCGRStatic(stage->arc, 8, 3, 0, 0, FALSE, stage->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(stage->arc, 14, 3, 0, 0, FALSE, stage->heapId);
    GFL_G2DIOLoadArcNCLR(stage->arc, 3, 0, 0, 0, 0x20, stage->heapId);
    GFL_BGSysLoadArcNCGRStatic(stage->arc, 9, 2, 0, 0, FALSE, stage->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(stage->arc, 15, 2, 0, 0, FALSE, stage->heapId);
    GFL_G2DIOLoadArcNCLRDefault(stage->arc, 5, 4, 0, 0, stage->heapId);
    GFL_BGSysLoadArcNCGRStatic(stage->arc, 13, 5, 0, 0, FALSE, stage->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(stage->arc, 17, 5, 0, 0, FALSE, stage->heapId);
    GFL_BGSysLoadArcNCGRStatic(stage->arc, 12, 6, 0, 0, FALSE, stage->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(stage->arc, 16, 6, 0, 0, FALSE, stage->heapId);
    GFL_BGSysLoadArcNCGRStatic(stage->arc, 11, 4, 0, 0, FALSE, stage->heapId);
    // The sub screen's first palette shimmers toward its second
    MI_CpuCopy16((void *)HW_DB_BG_PLTT, stage->paletteBase, 0x20);
    MI_CpuCopy16((void *)(HW_DB_BG_PLTT + 0x20), palette, 0x20);
    for (i = 0; i < 16; i++) {
        u16 base = stage->paletteBase[i];
        u16 color = palette[i];

        stage->paletteDiffR[i] =
            (u8)((color & GX_RGB_R_MASK) >> GX_RGB_R_SHIFT) - (u8)((base & GX_RGB_R_MASK) >> GX_RGB_R_SHIFT);
        stage->paletteDiffG[i] =
            (u8)((color & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT) - (u8)((base & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT);
        stage->paletteDiffB[i] =
            (u8)((color & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT) - (u8)((base & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT);
    }
    GFL_BGSysLoadScr(3);
    GFL_BGSysLoadScr(5);
    stage->bg = StaActBg_InitSystem(stage->heapId, stage);
}

static void StaActing_LoadBg(StaActing *stage, u32 unused) {
    StaActBg_LoadBg(stage->bg, func_ov012_02152614(stage->param->program));
}

static void StaActing_InitBG(const BGSetup *setup, u8 bg, u8 mode) {
    GFL_BGSysCreateBG(bg, setup, mode);
    GFL_BGSysSetBGEnabled(bg, TRUE);
    GFL_BGSysClearBG(bg);
    GFL_BGSysLoadScr(bg);
}

static void StaActing_InitPoke(StaActing *stage) {
    VecFx32 pos = { FX32_CONST(256), FX32_CONST(160), FX32_CONST(170) };
    u8 i;

    stage->pokeDraw = MusPokeDraw_InitSystem(stage->heapId);
    MusPokeDraw_SetTexBase(stage->pokeDraw, 0x20000);
    stage->itemDraw = MusItemDraw_InitSystem(stage->blact, 36, stage->heapId);
    stage->pokeSys = StaActPoke_InitSystem(stage->heapId, stage, stage->pokeDraw, stage->itemDraw, stage->blact);
    for (i = 0; i < 4; i++) {
        stage->pokes[i] = NULL;
    }
    for (i = 0; i < 4; i++) {
        stage->pokes[i] = StaActPoke_CreatePoke(stage->pokeSys, stage->param->pokes[i]);
        StaActPoke_SetPosition(stage->pokeSys, stage->pokes[i], &pos);
        pos.z -= FX32_CONST(30);
    }
}

static void StaActing_InitObj(StaActing *stage) {
    VecFx32 unused = { 0, 0, 0 };
    int i;

    for (i = 0; i < 5; i++) {
        stage->objs[i] = NULL;
    }
    stage->objSys = StaActObj_InitSystem(stage->heapId, stage->blact);
}

static void StaActing_InitEffect(StaActing *stage) {
    int i;

    stage->effectSys = StaActEffect_InitSystem(stage->heapId);
    for (i = 0; i < 8; i++) {
        stage->effects[i] = NULL;
    }
}

// The stage follows the Pokémon in the limelight, or the player's, or the light the audience looks at, unless a
// script scrolls it; left and right scroll it by hand
static void StaActing_UpdateScroll(StaActing *stage) {
    s16 target;
    VecFx32 pos;

    if (GCTX_HIDGetHeldKeys() & PAD_KEY_RIGHT) {
        if (stage->scrollTarget + 2 < STA_ACTING_SCROLL_MAX) {
            stage->scrollTarget += 2;
        } else {
            stage->scrollTarget = STA_ACTING_SCROLL_MAX;
        }
    }
    if (GCTX_HIDGetHeldKeys() & PAD_KEY_LEFT) {
        if (stage->scrollTarget - 2 > 0) {
            stage->scrollTarget -= 2;
        } else {
            stage->scrollTarget = 0;
        }
    }
    if (!(GCTX_HIDGetHeldKeys() & (PAD_KEY_RIGHT | PAD_KEY_LEFT)) && stage->scriptScroll == FALSE &&
        stage->scrollFixed == FALSE && stage->curtainClosed == FALSE) {
        if (stage->followLight == STA_ACTING_NO_LIGHT) {
            u8 followPos = stage->followPos;

            if (followPos == STA_ACTING_NO_POKE) {
                followPos = stage->playerPos;
            }
            if (stage->pokes[followPos] != NULL) {
                StaActPoke_GetPosition(stage->pokeSys, stage->pokes[followPos], &pos);
                target = FX_FX32_TO_F32(pos.x) - 128.0f;
            }
        } else {
            StaActLight_GetPosition(stage->lightSys, stage->lights[stage->followLight], &pos);
            target = FX_FX32_TO_F32(pos.x) - 128.0f;
        }
        if (target > STA_ACTING_SCROLL_MAX) {
            target = STA_ACTING_SCROLL_MAX;
        } else if (target < 0) {
            target = 0;
        }
        if (target != stage->scrollOffset) {
            stage->scrollTarget = target;
        }
    }
    if (stage->scrollTarget != stage->scrollOffset) {
        VecFx32 camPos = { 0, 0, FX32_CONST(301) };
        VecFx32 camTarget = { 0, 0, 0 };

        if (stage->scrollOffset > stage->scrollTarget) {
            if (stage->scrollOffset > 4 && stage->scrollOffset - 4 > stage->scrollTarget) {
                stage->scrollOffset -= 4;
            } else {
                stage->scrollOffset = stage->scrollTarget;
            }
        } else if (stage->scrollOffset < stage->scrollTarget) {
            if (stage->scrollOffset + 4 < stage->scrollTarget) {
                stage->scrollOffset += 4;
            } else {
                stage->scrollOffset = stage->scrollTarget;
            }
        }
        camPos.x = FX_F32_TO_FX32(stage->scrollOffset / 16.0f);
        camTarget.x = FX_F32_TO_FX32(stage->scrollOffset / 16.0f);
        GFL_G3DCameraSetLookatPos(stage->camera, &camPos);
        GFL_G3DCameraSetLookatTarget(stage->camera, &camTarget);
        GFL_G3DCameraFlush(stage->camera);
        GFL_BGSysMoveBG(2, BG_MOVE_SET_X, stage->scrollOffset);
        StaActBg_SetScrollOffset(stage->bg, stage->scrollOffset);
        StaActPoke_SetScrollOffset(stage->pokeSys, stage->scrollOffset);
        StaActObj_SetScrollOffset(stage->objSys, stage->scrollOffset);
        StaActAudience_SetScrollOffset(stage->audience, stage->scrollOffset);
    }
    if (stage->curtainOffset == STA_ACTING_CURTAIN_OPEN) {
        stage->curtainClosed = FALSE;
    }
    GFL_BGSysMoveBG(2, BG_MOVE_SET_Y, stage->curtainOffset);
}

// Read only by StaActing_GetUnused. Defined after StaActing_UpdateScroll, with STA_ACTING_VRAM_CONFIG first among the
// tables: one of the orders that lay out the file's data as the original does
const u32 STA_ACTING_UNUSED = 0x01600002;

// Runs the program's first script, synced with the scripts of the four positions that differ from it
static void StaActing_StartScripts(StaActing *stage) {
    u8 i;
    u32 *script = stage->script;
    u8 indexes[4] = { 2, 3, 4, 5 };

    StaActScript_CreateScript(stage->scriptSys, (u8 *)script + script[0], TRUE);
    for (i = 0; i < 4; i++) {
        if (script[0] != script[indexes[i]]) {
            StaActScript_CreateScript(stage->scriptSys, (u8 *)stage->script + script[indexes[i]], TRUE);
        }
    }
}

void StaActing_StartAppealScript(StaActing *stage, u8 file, u8 pos) {
    u32 *script = GFL_ArcToolReadHeapNewLZ(stage->arc, file + 55, FALSE, stage->heapId);

    if (stage->param->pokes[pos]->points >= 70) {
        StaActScript_CreateScriptFile(stage->scriptSys, script, 0, 1 << pos);
    } else if (stage->param->pokes[pos]->points >= 40) {
        StaActScript_CreateScriptFile(stage->scriptSys, script, 1, 1 << pos);
    } else {
        StaActScript_CreateScriptFile(stage->scriptSys, script, 2, 1 << pos);
    }
}

void StaActing_StartPokeScript(StaActing *stage, u8 file, u8 pokeMask) {
    u32 *script = GFL_ArcToolReadHeapNewLZ(stage->arc, file + 75, FALSE, stage->heapId);

    StaActScript_CreateScriptFile(stage->scriptSys, script, 0, pokeMask);
}

static void StaActing_UpdatePalette(StaActing *stage) {
    u8 i;
    fx32 ratio;

    if (stage->paletteAnimPhase + 0x160 >= 0x10000) {
        stage->paletteAnimPhase = stage->paletteAnimPhase + 0x160 - 0x10000;
    } else {
        stage->paletteAnimPhase += 0x160;
    }
    ratio = (FX_SinIdx(stage->paletteAnimPhase) + FX32_ONE) / 2;
    for (i = 0; i < 16; i++) {
        stage->paletteWork[i] = GX_RGB((u8)((stage->paletteBase[i] & GX_RGB_R_MASK) >> GX_RGB_R_SHIFT) +
                                           (s8)((stage->paletteDiffR[i] * ratio) >> FX32_SHIFT),
                                       (u8)((stage->paletteBase[i] & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT) +
                                           (s8)((stage->paletteDiffG[i] * ratio) >> FX32_SHIFT),
                                       (u8)((stage->paletteBase[i] & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT) +
                                           (s8)((stage->paletteDiffB[i] * ratio) >> FX32_SHIFT));
    }
    NNS_GfdRegisterNewVramTransferTask(31, 0, stage->paletteWork, 0x20);
}

static void StaActing_InitMessage(StaActing *stage) {
    stage->msgWin = BmpWin_CreateDynamic(1, 4, 20, 24, 4, 15, 1);
    BmpWin_FlushChar(stage->msgWin);
    BmpWin_FlushMap(stage->msgWin);
    GFL_BGSysLoadScr(1);
    stage->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, stage->heapId);
    stage->msgData = GFL_MsgDataCreateFromHandle(stage->param->ov210->msgArc, stage->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 15 * 0x20, 0x20, stage->heapId);
    stage->tcbExMgr = GFL_TCBExMgrCreate(stage->heapId, stage->heapId, 3, 0x100);
    stage->printStream = NULL;
    stage->msgStr = NULL;
    stage->printing = FALSE;
    stage->printQueue = func_02021998(stage->heapId);
}

static void StaActing_UpdateMessage(StaActing *stage) {
    GFL_TCBExMgrUpdate(stage->tcbExMgr);
    if (stage->printStream != NULL && func_020223b4(stage->printStream) == 2) {
        func_020223cc(stage->printStream);
        stage->printStream = NULL;
    }
    if (stage->printing == TRUE) {
        func_02021a3c(stage->printQueue);
        if (func_02021c0c(stage->printQueue) == TRUE) {
            BmpWin_FlushChar(stage->msgWin);
            stage->printing = FALSE;
        }
    }
}

void StaActing_PrintMessage(StaActing *stage, u16 msgId, u32 wait) {
    if (stage->printStream != NULL) {
        func_020223cc(stage->printStream);
        stage->printStream = NULL;
    }
    if (stage->msgStr != NULL) {
        GFL_StrBufFree(stage->msgStr);
        stage->msgStr = NULL;
    }
    GFL_BitmapFill(BmpWin_GetBitmap(stage->msgWin), 0);
    stage->msgStr = GFL_MsgDataLoadStrbufNew(stage->msgData, msgId);
    if (wait != 0xff) {
        stage->printStream =
            func_02022268(stage->msgWin, 0, 0, stage->msgStr, stage->font, wait, stage->tcbExMgr, 2, stage->heapId, 0);
    } else {
        func_02021c54(stage->printQueue, BmpWin_GetBitmap(stage->msgWin), 0, 0, stage->msgStr, stage->font);
        stage->printing = TRUE;
        GFL_StrBufFree(stage->msgStr);
        stage->msgStr = NULL;
    }
}

void StaActing_ClearMessage(StaActing *stage) {
    if (stage->printStream != NULL) {
        func_020223cc(stage->printStream);
        stage->printStream = NULL;
    }
    if (stage->msgStr != NULL) {
        GFL_StrBufFree(stage->msgStr);
        stage->msgStr = NULL;
    }
    GFL_BitmapFill(BmpWin_GetBitmap(stage->msgWin), 0);
    BmpWin_FlushChar(stage->msgWin);
}

// The audience cheers the Pokémon in the limelight or in focus, or looks at the only light, or cheers the leaders
static void StaActing_UpdateAudience(StaActing *stage) {
    if (stage->audienceRefresh == TRUE) {
        u8 lightCount = 0;
        u8 light;
        u8 i;

        for (i = 0; i < 4; i++) {
            StaActAudience_SetCheerPoke(stage->audience, i, FALSE);
        }
        StaActAudience_SetLookLight(stage->audience, 0xff);
        stage->followPos = STA_ACTING_NO_POKE;
        stage->followLight = STA_ACTING_NO_LIGHT;
        for (i = 0; i < 4; i++) {
            if (stage->lights[i] != NULL) {
                light = i;
                lightCount++;
            }
        }
        if (stage->lightUpPos < STA_ACTING_NO_POKE) {
            stage->followPos = stage->lightUpPos;
            StaActAudience_SetCheerPoke(stage->audience, stage->lightUpPos, TRUE);
        } else if (stage->audienceFocusPos < STA_ACTING_NO_POKE) {
            stage->followPos = stage->audienceFocusPos;
            StaActAudience_SetCheerPoke(stage->audience, stage->audienceFocusPos, TRUE);
        } else if (lightCount == 1) {
            stage->followLight = light;
            StaActAudience_SetLookLight(stage->audience, light);
        } else {
            for (i = 0; i < 4; i++) {
                if (stage->ranks[i] == 0) {
                    StaActAudience_SetCheerPoke(stage->audience, i, TRUE);
                }
            }
        }
        stage->audienceRefresh = FALSE;
    }
}

void StaActing_SetAudienceFocus(StaActing *stage, u8 pos, BOOL focus) {
    if (focus == TRUE) {
        stage->audienceFocusPos = pos;
    } else {
        stage->audienceFocusPos = STA_ACTING_NO_POKE;
    }
    stage->audienceRefresh = TRUE;
}

void StaActing_RefreshAudience(StaActing *stage) {
    stage->audienceRefresh = TRUE;
}

static void StaActing_UpdateRanks(StaActing *stage) {
    u8 i;
    u8 j;

    for (i = 0; i < 4; i++) {
        stage->ranks[i] = 0;
    }
    for (i = 0; i < 4; i++) {
        u16 points = stage->param->pokes[i]->points + stage->bonusPoints[i];

        for (j = 0; j < 4; j++) {
            if (i != j && (u16)(stage->param->pokes[j]->points + stage->bonusPoints[j]) > points) {
                stage->ranks[i]++;
            }
        }
    }
}

void StaActing_PlayStrm(StaActing *stage, u16 seq) {
    stage->strmPlaying = TRUE;
    func_020064b8(stage->sound[0], stage->sound[1], seq);
}

void StaActing_PlaySeq(StaActing *stage, u16 seq) {
    stage->seqPlaying = TRUE;
    func_02006564(seq);
}

void StaActing_StopSeq(StaActing *stage) {
    stage->seqPlaying = FALSE;
    func_02006574();
}

void StaActing_PlayWave(StaActing *stage, u16 waveArc, u16 a2, u16 a3) {
    func_02006528(waveArc, a2, stage->sound[2], a3);
}

void StaActing_PlayBgm(StaActing *stage, u32 bgm) {
    stage->bgmPlaying = TRUE;
    GFL_SndBGMPlay(bgm, 0xffff);
}

void StaActing_StopBgm(StaActing *stage) {
    stage->bgmPlaying = FALSE;
    func_02005d8c();
}

void StaActing_PlayApplause(StaActing *stage) {
    u8 i;
    u8 minPoints = 0xff;

    for (i = 0; i < 4; i++) {
        u16 points = stage->param->pokes[i]->points + stage->bonusPoints[i];

        if (minPoints > points) {
            minPoints = points;
        }
    }
    if (minPoints >= 70) {
        GFL_SndSEPlay(SEQ_SE_MSCL_09);
    } else if (minPoints >= 30) {
        GFL_SndSEPlay(SEQ_SE_MSCL_10);
    } else if (minPoints >= 1) {
        GFL_SndSEPlay(SEQ_SE_MSCL_11);
    }
}

void StaActing_UseItem(StaActing *stage, u32 equipPos) {
    if (stage->param->comm == NULL) {
        StaActing_RequestItemUse(stage, stage->playerPos, equipPos);
    } else {
        stage->commSendPending = TRUE;
        stage->commSendEquip = equipPos;
    }
}

static void StaActing_RequestItemUse(StaActing *stage, u8 pos, u32 equip) {
    stage->useRequest[pos] = TRUE;
    stage->useEquip[pos] = equip;
}

BOOL StaActing_IsUsingItem(StaActing *stage) {
    return StaActing_IsPokeUsingItem(stage, stage->playerPos);
}

static BOOL StaActing_IsPokeUsingItem(StaActing *stage, u8 pos) {
    if (StaActPoke_IsItemEffect(stage->pokeSys, stage->pokes[pos]) == TRUE) {
        return TRUE;
    }
    return stage->useRequest[pos];
}

// Starts the props the Pokémon asked to use, puts one of them in the limelight, and when its time ends scores it
static void StaActing_UpdateItemUse(StaActing *stage) {
    u8 userCount = 0;
    u8 users[4];
    u8 i;

    StaActing_UpdateNpcItemUse(stage);
    if (stage->param->comm != NULL && stage->commSendPending == TRUE &&
        func_ov211_021f0460(stage->param->comm, stage->commSendEquip) == TRUE) {
        stage->commSendPending = FALSE;
    }
    if (stage->param->comm == NULL) {
        for (i = 0; i < 4; i++) {
            if (stage->useRequest[i] == TRUE) {
                StaActPoke_StartItemEffect(stage->pokeSys, stage->pokes[i], stage->useEquip[i]);
                users[userCount] = i;
                userCount++;
                stage->useRequest[i] = FALSE;
                if (stage->useEquip[i] == STA_ACTING_EQUIP_HAND_R) {
                    stage->handPropUsed[i][0] = TRUE;
                } else if (stage->useEquip[i] == STA_ACTING_EQUIP_HAND_L) {
                    stage->handPropUsed[i][1] = TRUE;
                }
                StaActing_SetNpcItemTiming(stage, 2, i);
            }
        }
        if (userCount != 0) {
            if (stage->lightUpPos == STA_ACTING_NO_POKE) {
                stage->lightUpScores = TRUE;
            } else {
                stage->lightUpScores = FALSE;
            }
            stage->lightUpTimer = STA_ACTING_LIGHT_UP_FRAMES;
            stage->lightUpPos = users[GFL_RandomLCAlt(userCount)];
            stage->lightUpEquip = stage->useEquip[stage->lightUpPos];
            stage->audienceRefresh = TRUE;
        }
    } else {
        u8 lightUpPos = func_ov211_021f0598(stage->param->comm);

        if (lightUpPos < STA_ACTING_NO_POKE) {
            if (stage->lightUpPos == STA_ACTING_NO_POKE) {
                stage->lightUpScores = TRUE;
            } else {
                stage->lightUpScores = FALSE;
            }
            stage->lightUpTimer = STA_ACTING_LIGHT_UP_FRAMES;
            stage->lightUpPos = lightUpPos;
            stage->lightUpEquip = func_ov211_021f053c(stage->param->comm, lightUpPos);
            stage->audienceRefresh = TRUE;
            func_ov211_021f05b4(stage->param->comm);
        }
        for (i = 0; i < 4; i++) {
            u8 equip = func_ov211_021f053c(stage->param->comm, i);

            if (equip != STA_ACTING_EQUIP_NONE) {
                StaActPoke_StartItemEffect(stage->pokeSys, stage->pokes[i], equip);
                func_ov211_021f056c(stage->param->comm, i);
                if (equip == STA_ACTING_EQUIP_HAND_R) {
                    stage->handPropUsed[i][0] = TRUE;
                } else if (equip == STA_ACTING_EQUIP_HAND_L) {
                    stage->handPropUsed[i][1] = TRUE;
                }
                StaActing_SetNpcItemTiming(stage, 2, i);
            }
        }
    }
    if (stage->lightUpTimer != 0) {
        if (stage->lightUpTimer > stage->updateCount) {
            stage->lightUpTimer -= (u16)stage->updateCount;
        } else {
            stage->lightUpTimer = 0;
            if (stage->lightUpScores == TRUE) {
                if (stage->param->comm == NULL) {
                    u32 se = StaActing_ScoreItem(stage, stage->lightUpPos, stage->lightUpEquip);

                    stage->param->pokes[stage->lightUpPos]->unk54[stage->lightUpEquip] = TRUE;
                    GFL_SndSEPlay(se);
                } else if (func_02042bc4() == TRUE) {
                    if (StaActing_ScoreItem(stage, stage->lightUpPos, stage->lightUpEquip) == SEQ_SE_MSCL_11) {
                        func_ov211_021f05c0(stage->param->comm, stage->lightUpPos, stage->lightUpEquip, 1);
                    } else {
                        func_ov211_021f05c0(stage->param->comm, stage->lightUpPos, stage->lightUpEquip, 2);
                    }
                }
            }
            stage->lightUpPos = STA_ACTING_NO_POKE;
            stage->audienceRefresh = TRUE;
        }
    }
}

// Adds the points of a prop to the Pokémon's, and returns the sound of the audience's reaction
static u32 StaActing_ScoreItem(StaActing *stage, u8 pos, u8 equip) {
    u8 points =
        func_ov012_02152644(stage->param->program, func_ov210_021ef164(StaActing_GetItemData(stage),
                                                                       stage->param->pokes[pos]->equips[equip].itemId));

    stage->bonusPoints[pos] += points;
    StaActing_UpdateRanks(stage);
    if (points >= 7) {
        return SEQ_SE_MSCL_10;
    }
    return SEQ_SE_MSCL_11;
}

static void StaActing_UpdateNpcItemUse(StaActing *stage) {
    u8 i;

    if (stage->param->comm == NULL || func_02042bc4() == TRUE) {
        for (i = 0; i < 4; i++) {
            if (stage->npcUseWait[i] != 0) {
                stage->npcUseWait[i]--;
                if (stage->npcUseWait[i] == 0) {
                    if (stage->param->comm == NULL) {
                        StaActing_RequestItemUse(stage, i, stage->npcUseEquip[i]);
                    } else {
                        func_ov211_021f0510(stage->param->comm, i, stage->npcUseEquip[i]);
                    }
                }
            }
        }
    }
}

static inline u8 StaActing_RandomHand(void) {
    if ((u8)GFL_RandomMTRange(2) == 0) {
        return STA_ACTING_EQUIP_HAND_R;
    }
    return STA_ACTING_EQUIP_HAND_L;
}

void StaActing_SetNpcItemTiming(StaActing *stage, u32 timing, u8 pos) {
    u8 i;

    if (stage->param->comm == NULL || func_02042bc4() == TRUE) {
        for (i = 0; i < 4; i++) {
            BOOL hand[2] = { FALSE, FALSE };
            MusicalPoke *poke = stage->param->pokes[i];

            if (poke->owner == 2 && StaActing_IsPokeUsingItem(stage, i) != TRUE && stage->npcUseWait[i] == 0) {
                if (poke->equips[STA_ACTING_EQUIP_HAND_R].itemId != 0xff && stage->handPropUsed[i][0] == FALSE) {
                    hand[0] = TRUE;
                }
                if (poke->equips[STA_ACTING_EQUIP_HAND_L].itemId != 0xff && stage->handPropUsed[i][1] == FALSE) {
                    hand[1] = TRUE;
                }
                if (hand[0] != FALSE || hand[1] != FALSE) {
                    u8 equip;

                    if (hand[0] == TRUE && hand[1] == FALSE) {
                        equip = STA_ACTING_EQUIP_HAND_R;
                    } else if (hand[1] == TRUE && hand[0] == FALSE) {
                        equip = STA_ACTING_EQUIP_HAND_L;
                    } else {
                        equip = StaActing_RandomHand();
                    }
                    switch (timing) {
                    case 1:
                        if (poke->unk78 == STA_ACTING_USE_ON_APPEAL && i == pos) {
                            stage->npcUseWait[i] = 30 + GFL_RandomMTRange(30);
                            stage->npcUseEquip[i] = equip;
                        }
                        break;
                    case 2:
                        if (poke->unk78 == STA_ACTING_USE_OFTEN && GFL_RandomMTRange(100) < 50) {
                            stage->npcUseWait[i] = 60 + GFL_RandomMTRange(60);
                            stage->npcUseEquip[i] = equip;
                        }
                        if (poke->unk78 == STA_ACTING_USE_SOMETIMES && GFL_RandomMTRange(100) < 25) {
                            stage->npcUseWait[i] = 60 + GFL_RandomMTRange(60);
                            stage->npcUseEquip[i] = equip;
                        }
                        break;
                    case 0:
                        if (poke->unk78 != 0 && poke->unk78 < STA_ACTING_USE_ON_APPEAL) {
                            s8 jitter = GFL_RandomMTRange(121) - 60;
                            int wait = poke->unk78 + jitter;

                            if (wait < 0) {
                                stage->npcUseWait[i] = 0;
                            } else {
                                stage->npcUseWait[i] = wait;
                            }
                            stage->npcUseEquip[i] = equip;
                        }
                        break;
                    }
                }
            }
        }
    }
}

static void StaActing_InitAppealEffect(StaActing *stage) {
    stage->appealEffect = StaActEffect_AddEffect(stage->effectSys, 54);
}

static void StaActing_TermAppealEffect(StaActing *stage) {
    StaActEffect_DelEffect(stage->effectSys, stage->appealEffect);
}

void StaActing_StartAppealEffect(StaActing *stage, u8 pos) {
    VecFx32 pokePos;
    u8 i;

    StaActPoke_GetPosition(stage->pokeSys, stage->pokes[pos], &pokePos);
    pokePos.x = pokePos.x / 16;
    pokePos.y = (FX32_CONST(192) - (pokePos.y - FX32_CONST(32))) / 16;
    pokePos.z = FX32_CONST(180);
    for (i = 0; i < 3; i++) {
        StaActEffect_CreateEmitter(stage->appealEffect, i, &pokePos);
    }
    GFL_SndSEPlay(SEQ_SE_MSCL_20);
}

void StaActing_EnableButtons(StaActing *stage) {
    StaActButton_SetShowFlg(stage->button, TRUE);
}

void StaActing_DisableButtons(StaActing *stage) {
    StaActButton_SetShowFlg(stage->button, FALSE);
}

StaActPokeSys *StaActing_GetPokeSys(StaActing *stage) {
    return stage->pokeSys;
}

StaActPoke *StaActing_GetPoke(StaActing *stage, u8 pos) {
    return stage->pokes[pos];
}

StaActObjSys *StaActing_GetObjSys(StaActing *stage) {
    return stage->objSys;
}

StaActObj *StaActing_GetObj(StaActing *stage, u8 index) {
    return stage->objs[index];
}

void StaActing_SetObj(StaActing *stage, StaActObj *obj, u8 index) {
    stage->objs[index] = obj;
}

StaActEffectSys *StaActing_GetEffectSys(StaActing *stage) {
    return stage->effectSys;
}

StaActEffect *StaActing_GetEffect(StaActing *stage, u8 index) {
    return stage->effects[index];
}

void StaActing_SetEffect(StaActing *stage, StaActEffect *effect, u8 index) {
    stage->effects[index] = effect;
}

StaActLightSys *StaActing_GetLightSys(StaActing *stage) {
    return stage->lightSys;
}

StaActLight *StaActing_GetLight(StaActing *stage, u8 index) {
    return stage->lights[index];
}

void StaActing_SetLight(StaActing *stage, StaActLight *light, u8 index) {
    stage->lights[index] = light;
}

StaActAudience *StaActing_GetAudience(StaActing *stage) {
    return stage->audience;
}

u16 StaActing_GetCurtainOffset(StaActing *stage) {
    return stage->curtainOffset;
}

void StaActing_SetCurtainOffset(StaActing *stage, u16 offset) {
    stage->curtainOffset = offset;
}

u16 StaActing_GetScrollOffset(StaActing *stage) {
    return stage->scrollOffset;
}

void StaActing_SetScrollTarget(StaActing *stage, u16 target) {
    stage->scrollTarget = target;
}

void *StaActing_GetItemData(StaActing *stage) {
    return MusItemDraw_GetItemData(stage->itemDraw);
}

u8 StaActing_GetLightUpPoke(StaActing *stage) {
    return stage->lightUpPos;
}

u8 StaActing_GetPoints(StaActing *stage, u8 pos) {
    return stage->param->pokes[pos]->points;
}

u8 StaActing_GetBonusPoints(StaActing *stage, u8 pos) {
    return stage->bonusPoints[pos];
}

void StaActing_SetScrollFixed(StaActing *stage, BOOL fixed) {
    stage->scrollFixed = fixed;
}

void StaActing_SetScriptScroll(StaActing *stage, BOOL scroll) {
    stage->scriptScroll = scroll;
}

u32 StaActing_GetUpdateCount(StaActing *stage) {
    return stage->updateCount;
}

static void StaActing_DrawDebug(StaActing *stage) {
}
