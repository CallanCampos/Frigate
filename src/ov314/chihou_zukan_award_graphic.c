#include "types.h"
#include "app/chihou_zukan_award_graphic.h"
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
} ChihouZukanAwardBG;

typedef struct {
    ClActUnit *unit;
} ChihouZukanAwardClAct;

typedef struct {
    G3DCamera *camera;
} ChihouZukanAwardG3dSys;

struct ChihouZukanAwardGraphic {
    ChihouZukanAwardBG bg;
    ChihouZukanAwardClAct clact;
    ChihouZukanAwardG3dSys g3d;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} ChihouZukanAwardBGSetup;

static void ChihouZukanAwardGraphic_VBlank(TCB *tcb, void *data);
static void ChihouZukanAwardBG_Init(ChihouZukanAwardBG *bg, HeapID heapId);
static void ChihouZukanAwardBG_Free(ChihouZukanAwardBG *bg);
static void ChihouZukanAwardBG_Update(ChihouZukanAwardBG *bg);
static void ChihouZukanAwardBG_VBlank(ChihouZukanAwardBG *bg);
static void ChihouZukanAwardClAct_Init(ChihouZukanAwardClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void ChihouZukanAwardClAct_Free(ChihouZukanAwardClAct *clact);
static void ChihouZukanAwardClAct_Update(ChihouZukanAwardClAct *clact);
static void ChihouZukanAwardClAct_VBlank(ChihouZukanAwardClAct *clact);
static ClActUnit *ChihouZukanAwardClAct_GetUnit(ChihouZukanAwardClAct *clact);
static void ChihouZukanAwardG3dSys_Init(ChihouZukanAwardG3dSys *g3d, HeapID heapId);
static void ChihouZukanAwardG3dSys_Free(ChihouZukanAwardG3dSys *g3d);

static const VecFx32 sChihouZukanAwardCameraUpVector = { 0, FX32_CONST(2.6), 0 };
static const VecFx32 sChihouZukanAwardCameraPosition = { FX32_CONST(6.7), FX32_CONST(6.7), FX32_CONST(17.3) };
static const VecFx32 sChihouZukanAwardCameraTarget = { 0, FX32_ONE, 0 };

static const ClActSysSetup sChihouZukanAwardClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };
static const BGSysLCDConfig sChihouZukanAwardLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0,
                                                           GX_BG0_AS_3D };

static const Light sChihouZukanAwardLights[] = {
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
};

static const BGSysVRAMConfig sChihouZukanAwardVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_D,   GX_VRAM_TEXPLTT_0_F,     GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const ChihouZukanAwardBGSetup sChihouZukanAwardBGSetups[2] = {
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
static void ChihouZukanAwardGraphic_G3DInit(void) {
    u32 i;

    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, TRUE);
    G2_SetBG0Priority(0);
    G3X_SetShading(GX_SHADING_HIGHLIGHT);
    G3X_AntiAlias(FALSE);
    G3X_AlphaTest(FALSE, 0);
    G3X_AlphaBlend(TRUE);
    G3X_EdgeMarking(FALSE);
    G3X_SetFog(FALSE, 0, 0, 0);
    G3X_SetClearColor(GX_RGB(0, 0, 0), 0, 0x7fff, 63, FALSE);
    G3_ViewPort(0, 0, 255, 191);
    for (i = 0; i < NELEMS(sChihouZukanAwardLights); i++) {
        GFL_G3DSysLightSet(i, &sChihouZukanAwardLights[i]);
    }
    GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_AUTO, GX_BUFFERMODE_Z);
}

ChihouZukanAwardGraphic *ChihouZukanAwardGraphic_Create(u32 layout, HeapID heapId) {
    ChihouZukanAwardGraphic *graphic =
        GFL_HeapAllocate(heapId, sizeof(ChihouZukanAwardGraphic), FALSE, "chihou_zukan_award_graphic.c", 460);

    MI_CpuFill8(graphic, 0, sizeof(ChihouZukanAwardGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sChihouZukanAwardVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GFL_TextRndInitColorIndexLUT();
    ChihouZukanAwardBG_Init(&graphic->bg, heapId);
    ChihouZukanAwardClAct_Init(&graphic->clact, &sChihouZukanAwardVRAMConfig, heapId);
    ChihouZukanAwardG3dSys_Init(&graphic->g3d, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(ChihouZukanAwardGraphic_VBlank, graphic, 0);
    return graphic;
}

void ChihouZukanAwardGraphic_Free(ChihouZukanAwardGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    ChihouZukanAwardG3dSys_Free(&graphic->g3d);
    ChihouZukanAwardClAct_Free(&graphic->clact);
    ChihouZukanAwardBG_Free(&graphic->bg);
    GFL_TextRndResetColorIndexLUT();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(ChihouZukanAwardGraphic));
    GFL_HeapFree(graphic);
}

void ChihouZukanAwardGraphic_Update(ChihouZukanAwardGraphic *graphic) {
    ChihouZukanAwardClAct_Update(&graphic->clact);
    ChihouZukanAwardBG_Update(&graphic->bg);
}

ClActUnit *ChihouZukanAwardGraphic_GetClActUnit(ChihouZukanAwardGraphic *graphic) {
    return ChihouZukanAwardClAct_GetUnit(&graphic->clact);
}

static void ChihouZukanAwardGraphic_VBlank(TCB *tcb, void *data) {
    ChihouZukanAwardGraphic *graphic = data;

    ChihouZukanAwardBG_VBlank(&graphic->bg);
    ChihouZukanAwardClAct_VBlank(&graphic->clact);
}

static void ChihouZukanAwardBG_Init(ChihouZukanAwardBG *bg, HeapID heapId) {
    u32 i;

    MI_CpuFill8(bg, 0, sizeof(ChihouZukanAwardBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sChihouZukanAwardLCDConfig);
    for (i = 0; i < NELEMS(sChihouZukanAwardBGSetups); i++) {
        GFL_BGSysCreateBG(sChihouZukanAwardBGSetups[i].bg, &sChihouZukanAwardBGSetups[i].setup,
                          sChihouZukanAwardBGSetups[i].mode);
        GFL_BGSysClearBG(sChihouZukanAwardBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sChihouZukanAwardBGSetups[i].bg, sChihouZukanAwardBGSetups[i].enabled);
    }
}

static void ChihouZukanAwardBG_Free(ChihouZukanAwardBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sChihouZukanAwardBGSetups); i++) {
        GFL_BGSysReleaseBG(sChihouZukanAwardBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(ChihouZukanAwardBG));
}

static void ChihouZukanAwardBG_Update(ChihouZukanAwardBG *bg) {
}

static void ChihouZukanAwardBG_VBlank(ChihouZukanAwardBG *bg) {
    GFL_BGSysUpdate();
}

static void ChihouZukanAwardClAct_Init(ChihouZukanAwardClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(ChihouZukanAwardClAct));
    ClActSys_Create(&sChihouZukanAwardClActSetup, vramConfig, heapId);
    clact->unit = ClActUnit_Create(128, 0, heapId);
    ClActUnit_UseSystemRenderer(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void ChihouZukanAwardClAct_Free(ChihouZukanAwardClAct *clact) {
    ClActUnit_Delete(clact->unit);
    ClActSys_Delete();
    MI_CpuFill8(clact, 0, sizeof(ChihouZukanAwardClAct));
}

static void ChihouZukanAwardClAct_Update(ChihouZukanAwardClAct *clact) {
    ClActSys_Draw();
}

static void ChihouZukanAwardClAct_VBlank(ChihouZukanAwardClAct *clact) {
    ClActSys_VBlank();
}

static ClActUnit *ChihouZukanAwardClAct_GetUnit(ChihouZukanAwardClAct *clact) {
    return clact->unit;
}

static void ChihouZukanAwardG3dSys_Init(ChihouZukanAwardG3dSys *g3d, HeapID heapId) {
    GFL_G3DSysCreate(FALSE, 1, 0, 1, 0, heapId, ChihouZukanAwardGraphic_G3DInit);
    // A 40 degree field of view
    g3d->camera =
        GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)), FX_CosIdx(DEG_TO_IDX(20)),
                            FX32_CONST(4.0 / 3.0), 0, FX32_ONE, FX32_CONST(1024), 0, &sChihouZukanAwardCameraPosition,
                            &sChihouZukanAwardCameraUpVector, &sChihouZukanAwardCameraTarget, heapId);
}

static void ChihouZukanAwardG3dSys_Free(ChihouZukanAwardG3dSys *g3d) {
    GFL_G3DCameraFree(g3d->camera);
    GFL_G3DSysFree();
}
