// The ROM names this file shinka_demo_graphic.c.
#include "types.h"
#include "demo/evolution_demo.h"
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

// The evolution demo's BGs, cell actors and 3D system. The BGs of the sub screen are only created on request

#define BG_COUNT 6
// The last BG of the main screen and the first of the sub screen
#define BG_MAIN_LAST 3
#define BG_SUB_FIRST 4

typedef struct {
    u32 unk0;
} EvolutionDemoBG;

typedef struct {
    ClActUnit *unit;
} EvolutionDemoClAct;

typedef struct {
    G3DCamera *camera;
} EvolutionDemoG3dSys;

struct EvolutionDemoGraphic {
    EvolutionDemoBG bg;
    EvolutionDemoClAct clact;
    EvolutionDemoG3dSys g3d;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} EvolutionDemoBGSetup;

static void EvolutionDemoGraphic_VBlank(TCB *tcb, void *data);
static void EvolutionDemoBG_Init(EvolutionDemoBG *bg, HeapID heapId);
static void EvolutionDemoBG_Free(EvolutionDemoBG *bg);
static void EvolutionDemoBG_Update(EvolutionDemoBG *bg);
static void EvolutionDemoBG_VBlank(EvolutionDemoBG *bg);
static void EvolutionDemoClAct_Init(EvolutionDemoClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void EvolutionDemoClAct_Free(EvolutionDemoClAct *clact);
static void EvolutionDemoClAct_Update(EvolutionDemoClAct *clact);
static void EvolutionDemoClAct_VBlank(EvolutionDemoClAct *clact);
static ClActUnit *EvolutionDemoClAct_GetUnit(EvolutionDemoClAct *clact);
static void EvolutionDemoG3dSys_Init(EvolutionDemoG3dSys *g3d, HeapID heapId);
static void EvolutionDemoG3dSys_Free(EvolutionDemoG3dSys *g3d);
static void EvolutionDemoG3dSys_Begin(EvolutionDemoG3dSys *g3d);
static void EvolutionDemoG3dSys_End(EvolutionDemoG3dSys *g3d);

static const BGSysVRAMConfig sVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_64_E,  GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_01_BD, GX_VRAM_TEXPLTT_01_FG,   GX_OBJVRAMMODE_CHAR_1D_64K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const VecFx32 sCameraPosition = { 0, FX32_CONST(10), FX32_CONST(100) };
static const VecFx32 sCameraTarget = { 0, 0, 0 };
static const VecFx32 sCameraUpVector = { 0, FX32_ONE, 0 };

// clang-format off
static const EvolutionDemoBGSetup sBGSetups[BG_COUNT] = {
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
static void EvolutionDemoGraphic_G3DInit(void) {
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
    for (i = 0; i < NELEMS(sLights); i++) {
        GFL_G3DSysLightSet(i, &sLights[i]);
    }
    GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_MANUAL, GX_BUFFERMODE_Z);
}

EvolutionDemoGraphic *EvolutionDemoGraphic_Create(u32 layout, HeapID heapId) {
    EvolutionDemoGraphic *graphic =
        GFL_HeapAllocate(heapId, sizeof(EvolutionDemoGraphic), FALSE, "shinka_demo_graphic.c", 479);

    MI_CpuFill8(graphic, 0, sizeof(EvolutionDemoGraphic));
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
    EvolutionDemoBG_Init(&graphic->bg, heapId);
    EvolutionDemoClAct_Init(&graphic->clact, &sVRAMConfig, heapId);
    EvolutionDemoG3dSys_Init(&graphic->g3d, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(EvolutionDemoGraphic_VBlank, graphic, 0);
    return graphic;
}

void EvolutionDemoGraphic_Free(EvolutionDemoGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    EvolutionDemoG3dSys_Free(&graphic->g3d);
    EvolutionDemoClAct_Free(&graphic->clact);
    EvolutionDemoBG_Free(&graphic->bg);
    GFL_TextRndResetColorIndexLUT();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(EvolutionDemoGraphic));
    GFL_HeapFree(graphic);
}

void EvolutionDemoGraphic_Update(EvolutionDemoGraphic *graphic) {
    EvolutionDemoClAct_Update(&graphic->clact);
    EvolutionDemoBG_Update(&graphic->bg);
}

void EvolutionDemoGraphic_Begin3D(EvolutionDemoGraphic *graphic) {
    EvolutionDemoG3dSys_Begin(&graphic->g3d);
}

void EvolutionDemoGraphic_End3D(EvolutionDemoGraphic *graphic) {
    EvolutionDemoG3dSys_End(&graphic->g3d);
}

ClActUnit *EvolutionDemoGraphic_GetClActUnit(EvolutionDemoGraphic *graphic) {
    return EvolutionDemoClAct_GetUnit(&graphic->clact);
}

void EvolutionDemoGraphic_InitSubBG(EvolutionDemoGraphic *graphic) {
    u32 i;

    GFL_BGSysSetLCDConfigForEngine(&sLCDConfig, BGSYS_ENGINE_SUB);
    for (i = 0; i < NELEMS(sBGSetups); i++) {
        if (sBGSetups[i].bg >= BG_SUB_FIRST) {
            GFL_BGSysCreateBG(sBGSetups[i].bg, &sBGSetups[i].setup, sBGSetups[i].mode);
            GFL_BGSysClearBG(sBGSetups[i].bg);
            GFL_BGSysSetBGEnabled(sBGSetups[i].bg, sBGSetups[i].enabled);
        }
    }
}

void EvolutionDemoGraphic_FreeSubBG(EvolutionDemoGraphic *graphic) {
    u32 i;

    for (i = 0; i < NELEMS(sBGSetups); i++) {
        if (sBGSetups[i].bg >= BG_SUB_FIRST) {
            GFL_BGSysReleaseBG(sBGSetups[i].bg);
        }
    }
}

static void EvolutionDemoGraphic_VBlank(TCB *tcb, void *data) {
    EvolutionDemoGraphic *graphic = data;

    EvolutionDemoBG_VBlank(&graphic->bg);
    EvolutionDemoClAct_VBlank(&graphic->clact);
}

static void EvolutionDemoBG_Init(EvolutionDemoBG *bg, HeapID heapId) {
    u32 i;

    MI_CpuFill8(bg, 0, sizeof(EvolutionDemoBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sLCDConfig);
    for (i = 0; i < NELEMS(sBGSetups); i++) {
        if (sBGSetups[i].bg <= BG_MAIN_LAST) {
            GFL_BGSysCreateBG(sBGSetups[i].bg, &sBGSetups[i].setup, sBGSetups[i].mode);
            GFL_BGSysClearBG(sBGSetups[i].bg);
            GFL_BGSysSetBGEnabled(sBGSetups[i].bg, sBGSetups[i].enabled);
        }
    }
}

static void EvolutionDemoBG_Free(EvolutionDemoBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sBGSetups); i++) {
        if (sBGSetups[i].bg <= BG_MAIN_LAST) {
            GFL_BGSysReleaseBG(sBGSetups[i].bg);
        }
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(EvolutionDemoBG));
}

static void EvolutionDemoBG_Update(EvolutionDemoBG *bg) {
}

static void EvolutionDemoBG_VBlank(EvolutionDemoBG *bg) {
    GFL_BGSysUpdate();
}

static void EvolutionDemoClAct_Init(EvolutionDemoClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(EvolutionDemoClAct));
    ClActSys_Create(&sClActSetup, vramConfig, heapId);
    clact->unit = ClActUnit_Create(128, 0, heapId);
    ClActUnit_UseSystemRenderer(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void EvolutionDemoClAct_Free(EvolutionDemoClAct *clact) {
    ClActUnit_Delete(clact->unit);
    ClActSys_Delete();
    MI_CpuFill8(clact, 0, sizeof(EvolutionDemoClAct));
}

static void EvolutionDemoClAct_Update(EvolutionDemoClAct *clact) {
    ClActSys_Draw();
}

static void EvolutionDemoClAct_VBlank(EvolutionDemoClAct *clact) {
    ClActSys_VBlank();
}

static ClActUnit *EvolutionDemoClAct_GetUnit(EvolutionDemoClAct *clact) {
    return clact->unit;
}

static void EvolutionDemoG3dSys_Init(EvolutionDemoG3dSys *g3d, HeapID heapId) {
    GFL_G3DSysCreate(FALSE, 2, 0, 1, 0, heapId, EvolutionDemoGraphic_G3DInit);
    // A 40 degree field of view
    g3d->camera = GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)),
                                      FX_CosIdx(DEG_TO_IDX(20)), FX32_CONST(4.0 / 3.0), 0, FX32_ONE, FX32_CONST(1024),
                                      0, &sCameraPosition, &sCameraUpVector, &sCameraTarget, heapId);
}

static void EvolutionDemoG3dSys_Free(EvolutionDemoG3dSys *g3d) {
    GFL_G3DCameraFree(g3d->camera);
    GFL_G3DSysFree();
}

static void EvolutionDemoG3dSys_Begin(EvolutionDemoG3dSys *g3d) {
    GFL_G3DSysReset();
    GFL_G3DCameraFlush(g3d->camera);
    GFL_G3DSysMtxViewFlush();
}

static void EvolutionDemoG3dSys_End(EvolutionDemoG3dSys *g3d) {
    GFL_G3DSysReqSwapBuffers();
}
