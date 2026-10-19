#include "types.h"
#include "demo/egg_demo.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "system/gf_font.h"

// The egg demo's BGs, cell actors and 3D system

#define BG_COUNT 6

typedef struct {
    u32 unk0;
} EggDemoBG;

typedef struct {
    ClActUnit *unit;
} EggDemoClAct;

typedef struct {
    G3DCamera *camera;
} EggDemoG3dSys;

struct EggDemoGraphic {
    EggDemoBG bg;
    EggDemoClAct clact;
    EggDemoG3dSys g3d;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} EggDemoBGSetup;

static void EggDemoGraphic_VBlank(TCB *tcb, void *data);
static void EggDemoBG_Init(EggDemoBG *bg, HeapID heapId);
static void EggDemoBG_Free(EggDemoBG *bg);
static void EggDemoBG_Update(EggDemoBG *bg);
static void EggDemoBG_VBlank(EggDemoBG *bg);
static void EggDemoClAct_Init(EggDemoClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void EggDemoClAct_Free(EggDemoClAct *clact);
static void EggDemoClAct_Update(EggDemoClAct *clact);
static void EggDemoClAct_VBlank(EggDemoClAct *clact);
static ClActUnit *EggDemoClAct_GetUnit(EggDemoClAct *clact);
static void EggDemoG3dSys_Init(EggDemoG3dSys *g3d, HeapID heapId);
static void EggDemoG3dSys_Free(EggDemoG3dSys *g3d);
static void EggDemoG3dSys_Begin(EggDemoG3dSys *g3d);
static void EggDemoG3dSys_End(EggDemoG3dSys *g3d);

static const BGSysVRAMConfig sVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_64_E,  GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_01_BD, GX_VRAM_TEXPLTT_01_FG,   GX_OBJVRAMMODE_CHAR_1D_64K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const VecFx32 sCameraPosition = { 0, FX32_CONST(10), FX32_CONST(100) };
static const VecFx32 sCameraTarget = { 0, 0, 0 };
static const VecFx32 sCameraUpVector = { 0, FX32_ONE, 0 };

// clang-format off
static const EggDemoBGSetup sBGSetups[BG_COUNT] = {
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x08000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 2,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 3,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x14000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 5,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x08000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 6,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
};

// clang-format on

static const Light sLights[] = {
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
};

static const BGSysLCDConfig sLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };
static const ClActSysSetup sClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

// The 3D system calls this once it is set up
static void EggDemoGraphic_G3DInit(void) {
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
    for (i = 0; i < NELEMS(sLights); i++) {
        GFL_G3DSysLightSet(i, &sLights[i]);
    }
    GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_AUTO, GX_BUFFERMODE_Z);
}

EggDemoGraphic *EggDemoGraphic_Create(u32 layout, HeapID heapId) {
    EggDemoGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(EggDemoGraphic), FALSE, "egg_demo_graphic.c", 466);

    MI_CpuFill8(graphic, 0, sizeof(EggDemoGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GFL_TextRndInitColorIndexLUT();
    EggDemoBG_Init(&graphic->bg, heapId);
    EggDemoClAct_Init(&graphic->clact, &sVRAMConfig, heapId);
    EggDemoG3dSys_Init(&graphic->g3d, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(EggDemoGraphic_VBlank, graphic, 0);
    return graphic;
}

void EggDemoGraphic_Free(EggDemoGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    EggDemoG3dSys_Free(&graphic->g3d);
    EggDemoClAct_Free(&graphic->clact);
    EggDemoBG_Free(&graphic->bg);
    GFL_TextRndResetColorIndexLUT();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(EggDemoGraphic));
    GFL_HeapFree(graphic);
}

void EggDemoGraphic_Update(EggDemoGraphic *graphic) {
    EggDemoClAct_Update(&graphic->clact);
    EggDemoBG_Update(&graphic->bg);
}

void EggDemoGraphic_Begin3D(EggDemoGraphic *graphic) {
    EggDemoG3dSys_Begin(&graphic->g3d);
}

void EggDemoGraphic_End3D(EggDemoGraphic *graphic) {
    EggDemoG3dSys_End(&graphic->g3d);
}

ClActUnit *EggDemoGraphic_GetClActUnit(EggDemoGraphic *graphic) {
    return EggDemoClAct_GetUnit(&graphic->clact);
}

static void EggDemoGraphic_VBlank(TCB *tcb, void *data) {
    EggDemoGraphic *graphic = data;

    EggDemoBG_VBlank(&graphic->bg);
    EggDemoClAct_VBlank(&graphic->clact);
}

static void EggDemoBG_Init(EggDemoBG *bg, HeapID heapId) {
    u32 i;

    MI_CpuFill8(bg, 0, sizeof(EggDemoBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sLCDConfig);
    for (i = 0; i < NELEMS(sBGSetups); i++) {
        GFL_BGSysCreateBG(sBGSetups[i].bg, &sBGSetups[i].setup, sBGSetups[i].mode);
        GFL_BGSysClearBG(sBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sBGSetups[i].bg, sBGSetups[i].enabled);
    }
}

static void EggDemoBG_Free(EggDemoBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sBGSetups); i++) {
        GFL_BGSysReleaseBG(sBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(EggDemoBG));
}

static void EggDemoBG_Update(EggDemoBG *bg) {
}

static void EggDemoBG_VBlank(EggDemoBG *bg) {
    GFL_BGSysUpdate();
}

static void EggDemoClAct_Init(EggDemoClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(EggDemoClAct));
    ClActSys_Create(&sClActSetup, vramConfig, heapId);
    clact->unit = ClActUnit_Create(128, 0, heapId);
    ClActUnit_UseSystemRenderer(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void EggDemoClAct_Free(EggDemoClAct *clact) {
    ClActUnit_Delete(clact->unit);
    ClActSys_Delete();
    MI_CpuFill8(clact, 0, sizeof(EggDemoClAct));
}

static void EggDemoClAct_Update(EggDemoClAct *clact) {
    ClActSys_Draw();
}

static void EggDemoClAct_VBlank(EggDemoClAct *clact) {
    ClActSys_VBlank();
}

static ClActUnit *EggDemoClAct_GetUnit(EggDemoClAct *clact) {
    return clact->unit;
}

static void EggDemoG3dSys_Init(EggDemoG3dSys *g3d, HeapID heapId) {
    GFL_G3DSysCreate(FALSE, 2, 0, 1, 0, heapId, EggDemoGraphic_G3DInit);
    // A 40 degree field of view
    g3d->camera = GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)),
                                      FX_CosIdx(DEG_TO_IDX(20)), FX32_CONST(4.0 / 3.0), 0, FX32_ONE, FX32_CONST(1024),
                                      0, &sCameraPosition, &sCameraUpVector, &sCameraTarget, heapId);
}

static void EggDemoG3dSys_Free(EggDemoG3dSys *g3d) {
    GFL_G3DCameraFree(g3d->camera);
    GFL_G3DSysFree();
}

static void EggDemoG3dSys_Begin(EggDemoG3dSys *g3d) {
    GFL_G3DSysReset();
    GFL_G3DCameraFlush(g3d->camera);
    GFL_G3DSysMtxViewFlush();
}

static void EggDemoG3dSys_End(EggDemoG3dSys *g3d) {
    GFL_G3DSysReqSwapBuffers();
}
