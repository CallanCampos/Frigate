#include "types.h"
#include "app/regional_pokedex_award_graphic.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "system/gf_font.h"

// The BGs, cell actors and 3D system of the regional Pokédex diploma, named after the ROM's
// "chihou_zukan_award_graphic.c"

typedef struct {
    u32 unk0;
} RegionalPokedexAwardBG;

typedef struct {
    ClActUnit *unit;
} RegionalPokedexAwardClAct;

typedef struct {
    G3DCamera *camera;
} RegionalPokedexAwardG3dSys;

struct RegionalPokedexAwardGraphic {
    RegionalPokedexAwardBG bg;
    RegionalPokedexAwardClAct clact;
    RegionalPokedexAwardG3dSys g3d;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} RegionalPokedexAwardBGSetup;

static void RegionalPokedexAwardGraphic_VBlank(TCB *tcb, void *data);
static void RegionalPokedexAwardBG_Init(RegionalPokedexAwardBG *bg, HeapID heapId);
static void RegionalPokedexAwardBG_Free(RegionalPokedexAwardBG *bg);
static void RegionalPokedexAwardBG_Update(RegionalPokedexAwardBG *bg);
static void RegionalPokedexAwardBG_VBlank(RegionalPokedexAwardBG *bg);
static void RegionalPokedexAwardClAct_Init(RegionalPokedexAwardClAct *clact, const BGSysVRAMConfig *vramConfig,
                                           HeapID heapId);
static void RegionalPokedexAwardClAct_Free(RegionalPokedexAwardClAct *clact);
static void RegionalPokedexAwardClAct_Update(RegionalPokedexAwardClAct *clact);
static void RegionalPokedexAwardClAct_VBlank(RegionalPokedexAwardClAct *clact);
static ClActUnit *RegionalPokedexAwardClAct_GetUnit(RegionalPokedexAwardClAct *clact);
static void RegionalPokedexAwardG3dSys_Init(RegionalPokedexAwardG3dSys *g3d, HeapID heapId);
static void RegionalPokedexAwardG3dSys_Free(RegionalPokedexAwardG3dSys *g3d);

static const VecFx32 sRegionalPokedexAwardCameraUpVector = { 0, FX32_CONST(2.6), 0 };
static const VecFx32 sRegionalPokedexAwardCameraPosition = { FX32_CONST(6.7), FX32_CONST(6.7), FX32_CONST(17.3) };
static const VecFx32 sRegionalPokedexAwardCameraTarget = { 0, FX32_ONE, 0 };

static const ClActSysSetup sRegionalPokedexAwardClActSetup = {
    0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16
};
static const BGSysLCDConfig sRegionalPokedexAwardLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0,
                                                               GX_BG0_AS_3D };

static const Light sRegionalPokedexAwardLights[] = {
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
};

static const BGSysVRAMConfig sRegionalPokedexAwardVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_D,   GX_VRAM_TEXPLTT_0_F,     GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const RegionalPokedexAwardBGSetup sRegionalPokedexAwardBGSetups[2] = {
    { 1,
      { 0, 0, 0x1000, 0, BGRES_256x512, GX_BG_COLORMODE_256, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x4000), 0x10000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 2,
      { 0, 0, 0x1000, 0, BGRES_256x512, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x14000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

// The 3D system calls this once it is set up
static void RegionalPokedexAwardGraphic_G3DInit(void) {
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
    for (i = 0; i < NELEMS(sRegionalPokedexAwardLights); i++) {
        GFL_G3DSysLightSet(i, &sRegionalPokedexAwardLights[i]);
    }
    GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_AUTO, GX_BUFFERMODE_Z);
}

RegionalPokedexAwardGraphic *RegionalPokedexAwardGraphic_Create(u32 layout, HeapID heapId) {
    RegionalPokedexAwardGraphic *graphic =
        GFL_HeapAllocate(heapId, sizeof(RegionalPokedexAwardGraphic), FALSE, "chihou_zukan_award_graphic.c", 460);

    MI_CpuFill8(graphic, 0, sizeof(RegionalPokedexAwardGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sRegionalPokedexAwardVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GFL_TextRndInitColorIndexLUT();
    RegionalPokedexAwardBG_Init(&graphic->bg, heapId);
    RegionalPokedexAwardClAct_Init(&graphic->clact, &sRegionalPokedexAwardVRAMConfig, heapId);
    RegionalPokedexAwardG3dSys_Init(&graphic->g3d, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(RegionalPokedexAwardGraphic_VBlank, graphic, 0);
    return graphic;
}

void RegionalPokedexAwardGraphic_Free(RegionalPokedexAwardGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    RegionalPokedexAwardG3dSys_Free(&graphic->g3d);
    RegionalPokedexAwardClAct_Free(&graphic->clact);
    RegionalPokedexAwardBG_Free(&graphic->bg);
    GFL_TextRndResetColorIndexLUT();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(RegionalPokedexAwardGraphic));
    GFL_HeapFree(graphic);
}

void RegionalPokedexAwardGraphic_Update(RegionalPokedexAwardGraphic *graphic) {
    RegionalPokedexAwardClAct_Update(&graphic->clact);
    RegionalPokedexAwardBG_Update(&graphic->bg);
}

ClActUnit *RegionalPokedexAwardGraphic_GetClActUnit(RegionalPokedexAwardGraphic *graphic) {
    return RegionalPokedexAwardClAct_GetUnit(&graphic->clact);
}

static void RegionalPokedexAwardGraphic_VBlank(TCB *tcb, void *data) {
    RegionalPokedexAwardGraphic *graphic = data;

    RegionalPokedexAwardBG_VBlank(&graphic->bg);
    RegionalPokedexAwardClAct_VBlank(&graphic->clact);
}

static void RegionalPokedexAwardBG_Init(RegionalPokedexAwardBG *bg, HeapID heapId) {
    u32 i;

    MI_CpuFill8(bg, 0, sizeof(RegionalPokedexAwardBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sRegionalPokedexAwardLCDConfig);
    for (i = 0; i < NELEMS(sRegionalPokedexAwardBGSetups); i++) {
        GFL_BGSysCreateBG(sRegionalPokedexAwardBGSetups[i].bg, &sRegionalPokedexAwardBGSetups[i].setup,
                          sRegionalPokedexAwardBGSetups[i].mode);
        GFL_BGSysClearBG(sRegionalPokedexAwardBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sRegionalPokedexAwardBGSetups[i].bg, sRegionalPokedexAwardBGSetups[i].enabled);
    }
}

static void RegionalPokedexAwardBG_Free(RegionalPokedexAwardBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sRegionalPokedexAwardBGSetups); i++) {
        GFL_BGSysReleaseBG(sRegionalPokedexAwardBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(RegionalPokedexAwardBG));
}

static void RegionalPokedexAwardBG_Update(RegionalPokedexAwardBG *bg) {
}

static void RegionalPokedexAwardBG_VBlank(RegionalPokedexAwardBG *bg) {
    GFL_BGSysUpdate();
}

static void RegionalPokedexAwardClAct_Init(RegionalPokedexAwardClAct *clact, const BGSysVRAMConfig *vramConfig,
                                           HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(RegionalPokedexAwardClAct));
    ClActSys_Create(&sRegionalPokedexAwardClActSetup, vramConfig, heapId);
    clact->unit = ClActUnit_Create(128, 0, heapId);
    ClActUnit_UseSystemRenderer(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void RegionalPokedexAwardClAct_Free(RegionalPokedexAwardClAct *clact) {
    ClActUnit_Delete(clact->unit);
    ClActSys_Delete();
    MI_CpuFill8(clact, 0, sizeof(RegionalPokedexAwardClAct));
}

static void RegionalPokedexAwardClAct_Update(RegionalPokedexAwardClAct *clact) {
    ClActSys_Draw();
}

static void RegionalPokedexAwardClAct_VBlank(RegionalPokedexAwardClAct *clact) {
    ClActSys_VBlank();
}

static ClActUnit *RegionalPokedexAwardClAct_GetUnit(RegionalPokedexAwardClAct *clact) {
    return clact->unit;
}

static void RegionalPokedexAwardG3dSys_Init(RegionalPokedexAwardG3dSys *g3d, HeapID heapId) {
    GFL_G3DSysCreate(FALSE, 1, 0, 1, 0, heapId, RegionalPokedexAwardGraphic_G3DInit);
    // A 40 degree field of view
    g3d->camera = GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)),
                                      FX_CosIdx(DEG_TO_IDX(20)), FX32_CONST(4.0 / 3.0), 0, FX32_ONE, FX32_CONST(1024),
                                      0, &sRegionalPokedexAwardCameraPosition, &sRegionalPokedexAwardCameraUpVector,
                                      &sRegionalPokedexAwardCameraTarget, heapId);
}

static void RegionalPokedexAwardG3dSys_Free(RegionalPokedexAwardG3dSys *g3d) {
    GFL_G3DCameraFree(g3d->camera);
    GFL_G3DSysFree();
}
