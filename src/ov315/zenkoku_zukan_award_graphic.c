#include "types.h"
#include "app/zenkoku_zukan_award_graphic.h"
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

// The BGs, cell actors and 3D system of the national Pokédex diploma, named after the ROM's
// "zenkoku_zukan_award_graphic.c"

typedef struct {
    u32 unk0;
} ZenkokuZukanAwardBG;

typedef struct {
    ClActUnit *unit;
} ZenkokuZukanAwardClAct;

typedef struct {
    G3DCamera *camera;
} ZenkokuZukanAwardG3dSys;

struct ZenkokuZukanAwardGraphic {
    ZenkokuZukanAwardBG bg;
    ZenkokuZukanAwardClAct clact;
    ZenkokuZukanAwardG3dSys g3d;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} ZenkokuZukanAwardBGSetup;

static void ZenkokuZukanAwardGraphic_VBlank(TCB *tcb, void *data);
static void ZenkokuZukanAwardBG_Init(ZenkokuZukanAwardBG *bg, HeapID heapId);
static void ZenkokuZukanAwardBG_Free(ZenkokuZukanAwardBG *bg);
static void ZenkokuZukanAwardBG_Update(ZenkokuZukanAwardBG *bg);
static void ZenkokuZukanAwardBG_VBlank(ZenkokuZukanAwardBG *bg);
static void ZenkokuZukanAwardClAct_Init(ZenkokuZukanAwardClAct *clact, const BGSysVRAMConfig *vramConfig,
                                        HeapID heapId);
static void ZenkokuZukanAwardClAct_Free(ZenkokuZukanAwardClAct *clact);
static void ZenkokuZukanAwardClAct_Update(ZenkokuZukanAwardClAct *clact);
static void ZenkokuZukanAwardClAct_VBlank(ZenkokuZukanAwardClAct *clact);
static ClActUnit *ZenkokuZukanAwardClAct_GetUnit(ZenkokuZukanAwardClAct *clact);
static void ZenkokuZukanAwardG3dSys_Init(ZenkokuZukanAwardG3dSys *g3d, HeapID heapId);
static void ZenkokuZukanAwardG3dSys_Free(ZenkokuZukanAwardG3dSys *g3d);

static const VecFx32 sZenkokuZukanAwardCameraUpVector = { 0, FX32_CONST(2.6), 0 };
static const VecFx32 sZenkokuZukanAwardCameraPosition = { FX32_CONST(6.7), FX32_CONST(6.7), FX32_CONST(17.3) };
static const VecFx32 sZenkokuZukanAwardCameraTarget = { 0, FX32_ONE, 0 };

static const ClActSysSetup sZenkokuZukanAwardClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };
static const BGSysLCDConfig sZenkokuZukanAwardLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0,
                                                            GX_BG0_AS_3D };

static const Light sZenkokuZukanAwardLights[] = {
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
};

static const BGSysVRAMConfig sZenkokuZukanAwardVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_D,   GX_VRAM_TEXPLTT_0_F,     GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const ZenkokuZukanAwardBGSetup sZenkokuZukanAwardBGSetups[2] = {
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
static void ZenkokuZukanAwardGraphic_G3DInit(void) {
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
    for (i = 0; i < NELEMS(sZenkokuZukanAwardLights); i++) {
        GFL_G3DSysLightSet(i, &sZenkokuZukanAwardLights[i]);
    }
    GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_AUTO, GX_BUFFERMODE_Z);
}

ZenkokuZukanAwardGraphic *ZenkokuZukanAwardGraphic_Create(u32 layout, HeapID heapId) {
    ZenkokuZukanAwardGraphic *graphic =
        GFL_HeapAllocate(heapId, sizeof(ZenkokuZukanAwardGraphic), FALSE, "zenkoku_zukan_award_graphic.c", 460);

    MI_CpuFill8(graphic, 0, sizeof(ZenkokuZukanAwardGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sZenkokuZukanAwardVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GFL_TextRndInitColorIndexLUT();
    ZenkokuZukanAwardBG_Init(&graphic->bg, heapId);
    ZenkokuZukanAwardClAct_Init(&graphic->clact, &sZenkokuZukanAwardVRAMConfig, heapId);
    ZenkokuZukanAwardG3dSys_Init(&graphic->g3d, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(ZenkokuZukanAwardGraphic_VBlank, graphic, 0);
    return graphic;
}

void ZenkokuZukanAwardGraphic_Free(ZenkokuZukanAwardGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    ZenkokuZukanAwardG3dSys_Free(&graphic->g3d);
    ZenkokuZukanAwardClAct_Free(&graphic->clact);
    ZenkokuZukanAwardBG_Free(&graphic->bg);
    GFL_TextRndResetColorIndexLUT();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(ZenkokuZukanAwardGraphic));
    GFL_HeapFree(graphic);
}

void ZenkokuZukanAwardGraphic_Update(ZenkokuZukanAwardGraphic *graphic) {
    ZenkokuZukanAwardClAct_Update(&graphic->clact);
    ZenkokuZukanAwardBG_Update(&graphic->bg);
}

ClActUnit *ZenkokuZukanAwardGraphic_GetClActUnit(ZenkokuZukanAwardGraphic *graphic) {
    return ZenkokuZukanAwardClAct_GetUnit(&graphic->clact);
}

static void ZenkokuZukanAwardGraphic_VBlank(TCB *tcb, void *data) {
    ZenkokuZukanAwardGraphic *graphic = data;

    ZenkokuZukanAwardBG_VBlank(&graphic->bg);
    ZenkokuZukanAwardClAct_VBlank(&graphic->clact);
}

static void ZenkokuZukanAwardBG_Init(ZenkokuZukanAwardBG *bg, HeapID heapId) {
    u32 i;

    MI_CpuFill8(bg, 0, sizeof(ZenkokuZukanAwardBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sZenkokuZukanAwardLCDConfig);
    for (i = 0; i < NELEMS(sZenkokuZukanAwardBGSetups); i++) {
        GFL_BGSysCreateBG(sZenkokuZukanAwardBGSetups[i].bg, &sZenkokuZukanAwardBGSetups[i].setup,
                          sZenkokuZukanAwardBGSetups[i].mode);
        GFL_BGSysClearBG(sZenkokuZukanAwardBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sZenkokuZukanAwardBGSetups[i].bg, sZenkokuZukanAwardBGSetups[i].enabled);
    }
}

static void ZenkokuZukanAwardBG_Free(ZenkokuZukanAwardBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sZenkokuZukanAwardBGSetups); i++) {
        GFL_BGSysReleaseBG(sZenkokuZukanAwardBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(ZenkokuZukanAwardBG));
}

static void ZenkokuZukanAwardBG_Update(ZenkokuZukanAwardBG *bg) {
}

static void ZenkokuZukanAwardBG_VBlank(ZenkokuZukanAwardBG *bg) {
    GFL_BGSysUpdate();
}

static void ZenkokuZukanAwardClAct_Init(ZenkokuZukanAwardClAct *clact, const BGSysVRAMConfig *vramConfig,
                                        HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(ZenkokuZukanAwardClAct));
    ClActSys_Create(&sZenkokuZukanAwardClActSetup, vramConfig, heapId);
    clact->unit = ClActUnit_Create(128, 0, heapId);
    ClActUnit_UseSystemRenderer(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void ZenkokuZukanAwardClAct_Free(ZenkokuZukanAwardClAct *clact) {
    ClActUnit_Delete(clact->unit);
    ClActSys_Delete();
    MI_CpuFill8(clact, 0, sizeof(ZenkokuZukanAwardClAct));
}

static void ZenkokuZukanAwardClAct_Update(ZenkokuZukanAwardClAct *clact) {
    ClActSys_Draw();
}

static void ZenkokuZukanAwardClAct_VBlank(ZenkokuZukanAwardClAct *clact) {
    ClActSys_VBlank();
}

static ClActUnit *ZenkokuZukanAwardClAct_GetUnit(ZenkokuZukanAwardClAct *clact) {
    return clact->unit;
}

static void ZenkokuZukanAwardG3dSys_Init(ZenkokuZukanAwardG3dSys *g3d, HeapID heapId) {
    GFL_G3DSysCreate(FALSE, 1, 0, 1, 0, heapId, ZenkokuZukanAwardGraphic_G3DInit);
    // A 40 degree field of view
    g3d->camera =
        GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)), FX_CosIdx(DEG_TO_IDX(20)),
                            FX32_CONST(4.0 / 3.0), 0, FX32_ONE, FX32_CONST(1024), 0, &sZenkokuZukanAwardCameraPosition,
                            &sZenkokuZukanAwardCameraUpVector, &sZenkokuZukanAwardCameraTarget, heapId);
}

static void ZenkokuZukanAwardG3dSys_Free(ZenkokuZukanAwardG3dSys *g3d) {
    GFL_G3DCameraFree(g3d->camera);
    GFL_G3DSysFree();
}
