#include "types.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/species.h"
#include "constants/text_banks.h"
#include "demo/hall_of_fame_demo.h"
#include "demo/hall_of_fame_demo_main.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/calctool.h"
#include "gfl/double3Ddisp.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/particle.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "pml/poke_party.h"
#include "system/gf_font.h"
#include "system/mcss.h"
#include "system/wordset.h"

// The Hall of Fame entry demo's set-up: the display, the texts, the sound, the team, the 3D of both scenes, the
// particles of the first and the Pokémon sprite of the second (overlay 265). The file's name is a guess, after the
// Hall of Fame PC's hall_of_fame_pc_main.c

static void HallOfFameDemoMain_VBlank1st(TCB *tcb, void *data);
static void HallOfFameDemoMain_VBlank2nd(TCB *tcb, void *data);
static void HallOfFameDemoMain_Setup3D(void);
static void HallOfFameDemoMain_SetParticleCamera(HallOfFameDemoWork *wk, ParticleSystem *ptc, BOOL sub);
static void HallOfFameDemoMain_McssCallBack(u32 param, fx32 frame);

// The first scene's banks, with 3D on both screens, and the second's
static const BGSysVRAMConfig sHallOfFameDemoVramConfig1st = {
    GX_VRAM_BG_32_FG,
    GX_VRAM_BGEXTPLTT_NONE,
    GX_VRAM_NONE,
    GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B,
    GX_VRAM_OBJEXTPLTT_NONE,
    GX_VRAM_SUB_OBJ_NONE,
    GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_A,
    GX_VRAM_E,
    GX_OBJVRAMMODE_CHAR_1D_128K,
    GX_OBJVRAMMODE_CHAR_1D_128K,
};

static const BGSysVRAMConfig sHallOfFameDemoVramConfig2nd = {
    GX_VRAM_BG_128_D,
    GX_VRAM_BGEXTPLTT_23_G,
    GX_VRAM_SUB_BG_128_C,
    GX_VRAM_SUB_BGEXTPLTT_0123_H,
    GX_VRAM_OBJ_128_B,
    GX_VRAM_OBJEXTPLTT_NONE,
    GX_VRAM_SUB_OBJ_16_I,
    GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_A,
    GX_VRAM_E,
    GX_OBJVRAMMODE_CHAR_1D_128K,
    GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const u32 sHallOfFameDemoPreloadSE[] = { SEQ_SE_ROTATION_B };

static const Light sHallOfFameDemoLights[] = {
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
};

// The camera of the second scene's Pokémon
static const VecFx32 sHallOfFameDemoCameraUp = { 0, FX32_ONE, 0 };
static const VecFx32 sHallOfFameDemoCameraPos = { 0, FX32_CONST(30), FX32_CONST(3000) };
static const VecFx32 sHallOfFameDemoCameraTarget = { 0, FX32_CONST(30), 0 };

// The particle file of each type
static const u32 sHallOfFameDemoTypeParticles[] = {
    45, 38, 40, 46, 43, 48, 34, 41, 49, 39, 50, 42, 37, 47, 44, 36, 35,
};

void HallOfFameDemoMain_InitVBlank1st(HallOfFameDemoWork *wk) {
    wk->vblankTask = GFL_VBlankTCBAdd(HallOfFameDemoMain_VBlank1st, wk, 0);
}

void HallOfFameDemoMain_InitVBlank2nd(HallOfFameDemoWork *wk) {
    wk->vblankTask = GFL_VBlankTCBAdd(HallOfFameDemoMain_VBlank2nd, wk, 0);
}

void HallOfFameDemoMain_ExitVBlank(HallOfFameDemoWork *wk) {
    GFL_TCBRemove(wk->vblankTask);
}

static void HallOfFameDemoMain_VBlank1st(TCB *tcb, void *data) {
    ClActSys_VBlank();
    GFL_G3DDual3DExecVBlank();
    OS_SetIrqCheckFlag(OS_IE_V_BLANK);
}

static void HallOfFameDemoMain_VBlank2nd(TCB *tcb, void *data) {
    GFL_BGSysUpdate();
    ClActSys_VBlank();
    OS_SetIrqCheckFlag(OS_IE_V_BLANK);
}

void HallOfFameDemoMain_InitVram(u32 scene) {
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    if (scene == 0) {
        GFL_BGSysSetVRAMBanks(&sHallOfFameDemoVramConfig1st);
    } else {
        GFL_BGSysSetVRAMBanks(&sHallOfFameDemoVramConfig2nd);
    }
}

const BGSysVRAMConfig *HallOfFameDemoMain_GetVramConfig(u32 scene) {
    if (scene == 0) {
        return &sHallOfFameDemoVramConfig1st;
    }
    return &sHallOfFameDemoVramConfig2nd;
}

void HallOfFameDemoMain_InitBg(void) {
    GFL_BGSysCreate(HEAPID_HALL_OF_FAME_DEMO);
}

void HallOfFameDemoMain_ExitBg(void) {
    GFL_BGSysFree();
}

void HallOfFameDemoMain_InitBgMode(void) {
    BGSysLCDConfig config = { GX_DISPMODE_GRAPHICS, GX_BGMODE_3, GX_BGMODE_3, GX_BG0_AS_3D };

    GFL_BGSysSetLCDConfig(&config);
    GFL_BGSysResetStdPalette(0, GX_RGB(0, 0, 0));
    GFL_BGSysResetStdPalette(BGSYS_BG_SUB, GX_RGB(0, 0, 0));
}

// The second scene's backgrounds: one 1024x1024 affine picture across both screens
void HallOfFameDemoMain_InitBg2nd(void) {
    BGSetup setup = {
        0,
        0,
        0x2000,
        0,
        BGRES_512x512,
        GX_BG_COLORMODE_256,
        GX_BG_SCRBASE(0x2000),
        GX_BG_CHARBASE(0x8000),
        0x10000,
        GX_BG_EXTPLTT_23,
        1,
        GX_BG_AREAOVER_XLU,
        FALSE,
    };
    MtxFx22 mtx;

    GFL_BGSysCreateBG(3, &setup, BGMODE_EXTENDED);
    GFL_BGSysCreateBG(BGSYS_BG_SUB + 3, &setup, BGMODE_EXTENDED);
    MAT2_SetScaleRot(&mtx, 0, FX32_ONE, FX32_ONE, MAT2_ROT_IDX);
    G2_SetBG3Affine(&mtx, 0, 0, 0, 0);
    G2S_SetBG3Affine(&mtx, 0, 0, 0, 0);
    GFL_BGSysMoveBGReq(3, BG_MOVE_SET_X, 160);
    GFL_BGSysMoveBGReq(3, BG_MOVE_SET_Y, 112);
    GFL_BGSysMoveBGReq(BGSYS_BG_SUB + 3, BG_MOVE_SET_X, 160);
    GFL_BGSysMoveBGReq(BGSYS_BG_SUB + 3, BG_MOVE_SET_Y, -152);
    GFL_BGSysAdjustBGOriginReq(3, BG_CENTER_SET_X, 160);
    GFL_BGSysAdjustBGOriginReq(3, BG_CENTER_SET_Y, 160);
    GFL_BGSysAdjustBGOriginReq(BGSYS_BG_SUB + 3, BG_CENTER_SET_X, 160);
    GFL_BGSysAdjustBGOriginReq(BGSYS_BG_SUB + 3, BG_CENTER_SET_Y, 160);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG3, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG3, TRUE);
    GFL_BGSysResetStdPalette(0, GX_RGB(0, 0, 0));
    GFL_BGSysResetStdPalette(BGSYS_BG_SUB, GX_RGB(0, 0, 0));
}

void HallOfFameDemoMain_ExitBg2nd(void) {
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG3, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG3, FALSE);
    GFL_BGSysReleaseBG(BGSYS_BG_SUB + 3);
    GFL_BGSysReleaseBG(3);
}

void HallOfFameDemoMain_LoadBg(void) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_HALL_OF_FAME_DEMO, HEAPID_HALL_OF_FAME_DEMO);

    GFL_G2DIOLoadArcNCLRDefault(arc, NARC_HALL_OF_FAME_DEMO_BG_NCLR, PALTYPE_MAIN_BG_EX, 0x6000, 0x20,
                                HEAPID_HALL_OF_FAME_DEMO);
    GFL_G2DIOLoadArcNCLRDefault(arc, NARC_HALL_OF_FAME_DEMO_BG_NCLR, PALTYPE_SUB_BG_EX, 0x6000, 0x20,
                                HEAPID_HALL_OF_FAME_DEMO);
    GFL_BGSysLoadArcNCGRStatic(arc, NARC_HALL_OF_FAME_DEMO_BG_NCGR, 3, 0, 0, FALSE, HEAPID_HALL_OF_FAME_DEMO);
    GFL_BGSysLoadArcNCGRStatic(arc, NARC_HALL_OF_FAME_DEMO_BG_NCGR, BGSYS_BG_SUB + 3, 0, 0, FALSE,
                               HEAPID_HALL_OF_FAME_DEMO);
    loadBGScrToVramByFileNoReserveNegAlign(arc, NARC_HALL_OF_FAME_DEMO_BG_NSCR, 3, 0, 0, FALSE,
                                           HEAPID_HALL_OF_FAME_DEMO);
    loadBGScrToVramByFileNoReserveNegAlign(arc, NARC_HALL_OF_FAME_DEMO_BG_NSCR, BGSYS_BG_SUB + 3, 0, 0, FALSE,
                                           HEAPID_HALL_OF_FAME_DEMO);
    GFL_ArcToolFree(arc);
}

// Blends the 3D of the first scene over the actors and the backdrop
void HallOfFameDemoMain_SetBlendAlpha(void) {
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG0,
                       GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_BD,
                       6, 10);
    G2x_SetBlendAlpha_(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG0,
                       GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_BD,
                       6, 10);
}

void HallOfFameDemoMain_InitMsg(HallOfFameDemoWork *wk) {
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_2, HEAPID_HALL_OF_FAME_DEMO);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_HALL_OF_FAME_DEMO);
    wk->fontSmall = GFL_FontCreate(ARCID_FONT, 3, 0, FALSE, HEAPID_HALL_OF_FAME_DEMO);
    wk->wordSet = GFL_WordSetSystemCreateDefault(HEAPID_HALL_OF_FAME_DEMO);
    wk->strbuf = GFL_StrBufCreate(0x400, HEAPID_HALL_OF_FAME_DEMO);
}

void HallOfFameDemoMain_ExitMsg(HallOfFameDemoWork *wk) {
    GFL_StrBufFree(wk->strbuf);
    GFL_WordSetSystemFree(wk->wordSet);
    GFL_FontFree(wk->fontSmall);
    GFL_FontFree(wk->font);
    GFL_MsgDataFree(wk->msgData);
}

void HallOfFameDemoMain_InitSound(HallOfFameDemoWork *wk) {
    wk->seHandle = GFL_SndBGMStack_Preload(sHallOfFameDemoPreloadSE, NELEMS(sHallOfFameDemoPreloadSE));
    GFL_SndBGMPlay(SEQ_BGM_E_DENDOUIRI, 0xffff);
}

void HallOfFameDemoMain_ExitSound(HallOfFameDemoWork *wk) {
    GFL_SndBGMStack_Unload(wk->seHandle);
}

// Lists the team's Pokémon that are not Eggs
void HallOfFameDemoMain_GetPokeMax(HallOfFameDemoWork *wk) {
    u32 i;

    wk->monCount = 0;
    for (i = 0; i < PokeParty_GetPkmCount(wk->param->party); i++) {
        PartyPkm *pkm = PokeParty_GetPkm(wk->param->party, i);
        BOOL wasEncrypted = PokeParty_DecryptPkm(pkm);

        if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == FALSE) {
            wk->mons[wk->monCount] = pkm;
            wk->monCount++;
        }
        PokeParty_EncryptPkm(pkm, wasEncrypted);
    }
}

void HallOfFameDemoMain_GetPokeData(HallOfFameDemoWork *wk) {
    PartyPkm *pkm = wk->mons[wk->monIndex];
    BOOL wasEncrypted = PokeParty_DecryptPkm(pkm);

    wk->species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    wk->type = PokeParty_GetParam(pkm, PKM_PARAM_TYPE1, NULL);
    wk->form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
    PokeParty_EncryptPkm(pkm, wasEncrypted);
}

void HallOfFameDemoMain_LoadPokeVoice(HallOfFameDemoWork *wk) {
    if (wk->species == SPECIES_CHATOT) {
        PokeVoiceChatterInfo chatter;

        PokeVoice_CreateChatterInfo(&chatter);
        wk->voice = PokeVoice_Load(wk->species, wk->form, 64, FALSE, 0, 0, FALSE, &chatter);
    } else {
        wk->voice = PokeVoice_Load(wk->species, wk->form, 64, FALSE, 0, 0, FALSE, NULL);
    }
}

// The 3D system calls this once it is set up
static void HallOfFameDemoMain_Setup3D(void) {
    u32 i;

    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, TRUE);
    G2_SetBG0Priority(0);
    G3X_SetShading(GX_SHADING_HIGHLIGHT);
    G3X_AntiAlias(FALSE);
    G3X_AlphaTest(FALSE, 0);
    G3X_AlphaBlend(TRUE);
    G3X_EdgeMarking(FALSE);
    G3X_SetFog(FALSE, GX_FOGBLEND_COLOR_ALPHA, GX_FOGSLOPE_0x8000, 0);
    G3X_SetClearColor(GX_RGB(0, 0, 0), 0, 0x7fff, 63, FALSE);
    G3_ViewPort(0, 0, 255, 191);
    for (i = 0; i < NELEMS(sHallOfFameDemoLights); i++) {
        GFL_G3DSysLightSet(i, &sHallOfFameDemoLights[i]);
    }
    GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_MANUAL, GX_BUFFERMODE_Z);
}

void HallOfFameDemoMain_Init3D(HallOfFameDemoWork *wk) {
    GFL_G3DSysCreate(FALSE, 1, FALSE, 1, 0x1000, HEAPID_HALL_OF_FAME_DEMO, HallOfFameDemoMain_Setup3D);
    wk->g3dMgr = GFL_G3DMgrCreate(32, 32, HEAPID_HALL_OF_FAME_DEMO);
}

void HallOfFameDemoMain_Exit3D(HallOfFameDemoWork *wk) {
    GFL_G3DMgrFree(wk->g3dMgr);
    GFL_G3DSysFree();
}

// Draws the second scene's Pokémon
void HallOfFameDemoMain_Main3D(HallOfFameDemoWork *wk) {
    GFL_G3DSysReset();
    GFL_G3DSysMtxViewFlush();
    MCSSSys_Update(wk->mcssSys);
    MCSSSys_Draw(wk->mcssSys);
    GFL_G3DSysReqSwapBuffers();
}

// Draws the first scene's particles for the screen whose turn it is
void HallOfFameDemoMain_MainDouble3D(HallOfFameDemoWork *wk) {
    GFL_G3DSysReset();
    GFL_G3DSysMtxViewFlush();
    if (GFL_G3DDual3DGetRenderDisp() == TRUE) {
        HallOfFameDemoMain_SetParticleCamera(wk, wk->ptc[HALL_OF_FAME_DEMO_PTC_TYPE], TRUE);
        HallOfFameDemoMain_SetParticleCamera(wk, wk->ptc[HALL_OF_FAME_DEMO_PTC_NAME], TRUE);
    } else {
        HallOfFameDemoMain_SetParticleCamera(wk, wk->ptc[HALL_OF_FAME_DEMO_PTC_TYPE], FALSE);
        HallOfFameDemoMain_SetParticleCamera(wk, wk->ptc[HALL_OF_FAME_DEMO_PTC_NAME], FALSE);
    }
    Particle_DrawAndUpdateAll();
    GFL_G3DSysReqSwapBuffers();
    GFL_G3DDual3DRaiseNeedRenderFlag();
}

void HallOfFameDemoMain_InitDouble3D(void) {
    GFL_G3DDual3DInit(HEAPID_HALL_OF_FAME_DEMO);
}

void HallOfFameDemoMain_ExitDouble3D(void) {
    GFL_G3DDual3DFree();
}

void HallOfFameDemoMain_InitParticle(void) {
    Particle_Init(HEAPID_HALL_OF_FAME_DEMO);
}

void HallOfFameDemoMain_ExitParticle(void) {
    Particle_Free();
}

void HallOfFameDemoMain_CreateTypeParticle(HallOfFameDemoWork *wk) {
    wk->ptc[HALL_OF_FAME_DEMO_PTC_TYPE] = Particle_CreateSystem(
        wk->ptcWork[HALL_OF_FAME_DEMO_PTC_TYPE], HALL_OF_FAME_DEMO_PTC_WORK_SIZE, TRUE, HEAPID_HALL_OF_FAME_DEMO);
    Particle_SetResource(wk->ptc[HALL_OF_FAME_DEMO_PTC_TYPE],
                         Particle_ReadResource(ARCID_HALL_OF_FAME_DEMO, sHallOfFameDemoTypeParticles[wk->type],
                                               HEAPID_HALL_OF_FAME_DEMO),
                         TRUE, NULL);
}

void HallOfFameDemoMain_DeleteTypeParticle(HallOfFameDemoWork *wk) {
    Particle_DeleteSystem(wk->ptc[HALL_OF_FAME_DEMO_PTC_TYPE]);
    wk->ptc[HALL_OF_FAME_DEMO_PTC_TYPE] = NULL;
}

void HallOfFameDemoMain_SetTypeParticle(HallOfFameDemoWork *wk, u32 id) {
    VecFx32 pos = { 0, 0, 0x19a };

    Particle_CreateEmitter(wk->ptc[HALL_OF_FAME_DEMO_PTC_TYPE], id, &pos);
}

void HallOfFameDemoMain_CreateNameParticle(HallOfFameDemoWork *wk) {
    wk->ptc[HALL_OF_FAME_DEMO_PTC_NAME] = Particle_CreateSystem(
        wk->ptcWork[HALL_OF_FAME_DEMO_PTC_NAME], HALL_OF_FAME_DEMO_PTC_WORK_SIZE, TRUE, HEAPID_HALL_OF_FAME_DEMO);
    Particle_SetResource(
        wk->ptc[HALL_OF_FAME_DEMO_PTC_NAME],
        Particle_ReadResource(ARCID_HALL_OF_FAME_DEMO, NARC_HALL_OF_FAME_DEMO_NAME_SPA, HEAPID_HALL_OF_FAME_DEMO), TRUE,
        NULL);
}

void HallOfFameDemoMain_DeleteNameParticle(HallOfFameDemoWork *wk) {
    Particle_DeleteSystem(wk->ptc[HALL_OF_FAME_DEMO_PTC_NAME]);
    wk->ptc[HALL_OF_FAME_DEMO_PTC_NAME] = NULL;
}

void HallOfFameDemoMain_SetNameParticle(HallOfFameDemoWork *wk, u32 id) {
    VecFx32 pos = { 0, 0, 0 };

    Particle_CreateEmitter(wk->ptc[HALL_OF_FAME_DEMO_PTC_NAME], id, &pos);
}

// Gives a particle system the half of the view that a screen shows: the particles are laid out over both screens
static void HallOfFameDemoMain_SetParticleCamera(HallOfFameDemoWork *wk, ParticleSystem *ptc, BOOL sub) {
    G3DCameraProjection projection;

    if (ptc == NULL) {
        return;
    }
    if (sub == TRUE) {
        projection.param1 = FX32_CONST(4);
        projection.param2 = -FX32_CONST(4);
    } else {
        projection.param1 = FX32_CONST(14);
        projection.param2 = FX32_CONST(6);
    }
    projection.param3 = -0x5548;
    projection.param4 = 0x5548;
    projection.type = G3DCAM_PROJECTION_ORTHO;
    projection.far = FX32_CONST(1024);
    projection.near = 0;
    projection.ndcRangeOverride = 0;
    Particle_FreeCamera(ptc);
    Particle_SetCamera(ptc, &projection, 0x2000, NULL, NULL, NULL, HEAPID_HALL_OF_FAME_DEMO);
}

void HallOfFameDemoMain_InitMcss(HallOfFameDemoWork *wk) {
    wk->mcssSys = MCSSSys_Create(1, HEAPID_HALL_OF_FAME_DEMO);
    MCSSSys_SetTexBase(wk->mcssSys, 0);
    MCSSSys_EnableOrtho(wk->mcssSys);
    wk->camera = GFL_G3DCameraCreate(G3DCAM_PROJECTION_ORTHO, FX32_CONST(96), -FX32_CONST(96), -FX32_CONST(128),
                                     FX32_CONST(128), FX32_ONE, FX32_CONST(1024), 0, &sHallOfFameDemoCameraPos,
                                     &sHallOfFameDemoCameraTarget, &sHallOfFameDemoCameraUp, HEAPID_HALL_OF_FAME_DEMO);
    GFL_G3DCameraFlush(wk->camera);
}

void HallOfFameDemoMain_ExitMcss(HallOfFameDemoWork *wk) {
    HallOfFameDemoMain_DelMcss(wk);
    GFL_G3DCameraFree(wk->camera);
    MCSSSys_Free(wk->mcssSys);
}

// Adds the sprite of the Pokémon being shown to the right of the screen, still
void HallOfFameDemoMain_AddMcss(HallOfFameDemoWork *wk) {
    MCSSLoadInfo info;
    VecFx32 scale = { FX32_CONST(16), FX32_CONST(16), FX32_ONE };

    SetupPokemonLoaderByPartyPkm(wk->mons[wk->monIndex], &info, 0);
    wk->mcss = MCSSSys_Add(wk->mcssSys, FX32_CONST(176), -FX32_CONST(48), 0, &info);
    MCSS_SetScale(wk->mcss, &scale);
    MCSS_PauseAnimation(wk->mcss);
}

void HallOfFameDemoMain_DelMcss(HallOfFameDemoWork *wk) {
    if (wk->mcss != NULL) {
        MCSSSys_Remove(wk->mcssSys, wk->mcss);
        wk->mcss = NULL;
    }
}

void HallOfFameDemoMain_MoveMcss(HallOfFameDemoWork *wk, s32 move) {
    VecFx32 pos;

    MCSS_GetPosition(wk->mcss, &pos);
    pos.x += FX32_CONST(move);
    MCSS_SetPosition(wk->mcss, &pos);
}

static void HallOfFameDemoMain_McssCallBack(u32 param, fx32 frame) {
    HallOfFameDemoWork *wk = (HallOfFameDemoWork *)param;

    MCSS_PauseAnimation(wk->mcss);
    wk->mcssAnmEnd = TRUE;
}

// Has the Pokémon's animation stop at its end, which sets mcssAnmEnd
void HallOfFameDemoMain_SetMcssCallBack(HallOfFameDemoWork *wk) {
    wk->mcssAnmEnd = FALSE;
    MCSS_SetAnimationEndCallback(wk->mcss, (u32)wk, HallOfFameDemoMain_McssCallBack, 0);
}
