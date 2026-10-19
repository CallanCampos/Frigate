#include "types.h"
#include "demo/shinka_demo.h"
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
} ShinkaDemoBG;

typedef struct {
    ClActUnit *unit;
} ShinkaDemoClAct;

typedef struct {
    G3DCamera *camera;
} ShinkaDemoG3dSys;

struct ShinkaDemoGraphic {
    ShinkaDemoBG bg;
    ShinkaDemoClAct clact;
    ShinkaDemoG3dSys g3d;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} ShinkaDemoBGSetup;

static void ShinkaDemoGraphic_VBlank(TCB *tcb, void *data);
static void ShinkaDemoBG_Init(ShinkaDemoBG *bg, HeapID heapId);
static void ShinkaDemoBG_Free(ShinkaDemoBG *bg);
static void ShinkaDemoBG_Update(ShinkaDemoBG *bg);
static void ShinkaDemoBG_VBlank(ShinkaDemoBG *bg);
static void ShinkaDemoClAct_Init(ShinkaDemoClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void ShinkaDemoClAct_Free(ShinkaDemoClAct *clact);
static void ShinkaDemoClAct_Update(ShinkaDemoClAct *clact);
static void ShinkaDemoClAct_VBlank(ShinkaDemoClAct *clact);
static ClActUnit *ShinkaDemoClAct_GetUnit(ShinkaDemoClAct *clact);
static void ShinkaDemoG3dSys_Init(ShinkaDemoG3dSys *g3d, HeapID heapId);
static void ShinkaDemoG3dSys_Free(ShinkaDemoG3dSys *g3d);
static void ShinkaDemoG3dSys_Begin(ShinkaDemoG3dSys *g3d);
static void ShinkaDemoG3dSys_End(ShinkaDemoG3dSys *g3d);

static const BGSysVRAMConfig sVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_64_E,  GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_01_BD, GX_VRAM_TEXPLTT_01_FG,   GX_OBJVRAMMODE_CHAR_1D_64K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const VecFx32 sCameraPosition = { 0, FX32_CONST(10), FX32_CONST(100) };
static const VecFx32 sCameraTarget = { 0, 0, 0 };
static const VecFx32 sCameraUpVector = { 0, FX32_ONE, 0 };

// clang-format off
static const ShinkaDemoBGSetup sBGSetups[BG_COUNT] = {
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
static void ShinkaDemoGraphic_G3DInit(void) {
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
    GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_MANUAL, GX_BUFFERMODE_Z);
}

ShinkaDemoGraphic *ShinkaDemoGraphic_Create(u32 layout, HeapID heapId) {
    ShinkaDemoGraphic *graphic =
        GFL_HeapAllocate(heapId, sizeof(ShinkaDemoGraphic), FALSE, "shinka_demo_graphic.c", 479);

    MI_CpuFill8(graphic, 0, sizeof(ShinkaDemoGraphic));
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
    ShinkaDemoBG_Init(&graphic->bg, heapId);
    ShinkaDemoClAct_Init(&graphic->clact, &sVRAMConfig, heapId);
    ShinkaDemoG3dSys_Init(&graphic->g3d, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(ShinkaDemoGraphic_VBlank, graphic, 0);
    return graphic;
}

void ShinkaDemoGraphic_Free(ShinkaDemoGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    ShinkaDemoG3dSys_Free(&graphic->g3d);
    ShinkaDemoClAct_Free(&graphic->clact);
    ShinkaDemoBG_Free(&graphic->bg);
    GFL_TextRndResetColorIndexLUT();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(ShinkaDemoGraphic));
    GFL_HeapFree(graphic);
}

void ShinkaDemoGraphic_Update(ShinkaDemoGraphic *graphic) {
    ShinkaDemoClAct_Update(&graphic->clact);
    ShinkaDemoBG_Update(&graphic->bg);
}

void ShinkaDemoGraphic_Begin3D(ShinkaDemoGraphic *graphic) {
    ShinkaDemoG3dSys_Begin(&graphic->g3d);
}

void ShinkaDemoGraphic_End3D(ShinkaDemoGraphic *graphic) {
    ShinkaDemoG3dSys_End(&graphic->g3d);
}

ClActUnit *ShinkaDemoGraphic_GetClActUnit(ShinkaDemoGraphic *graphic) {
    return ShinkaDemoClAct_GetUnit(&graphic->clact);
}

void ShinkaDemoGraphic_InitSubBG(ShinkaDemoGraphic *graphic) {
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

void ShinkaDemoGraphic_FreeSubBG(ShinkaDemoGraphic *graphic) {
    u32 i;

    for (i = 0; i < NELEMS(sBGSetups); i++) {
        if (sBGSetups[i].bg >= BG_SUB_FIRST) {
            GFL_BGSysReleaseBG(sBGSetups[i].bg);
        }
    }
}

static void ShinkaDemoGraphic_VBlank(TCB *tcb, void *data) {
    ShinkaDemoGraphic *graphic = data;

    ShinkaDemoBG_VBlank(&graphic->bg);
    ShinkaDemoClAct_VBlank(&graphic->clact);
}

static void ShinkaDemoBG_Init(ShinkaDemoBG *bg, HeapID heapId) {
    u32 i;

    MI_CpuFill8(bg, 0, sizeof(ShinkaDemoBG));
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

static void ShinkaDemoBG_Free(ShinkaDemoBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sBGSetups); i++) {
        if (sBGSetups[i].bg <= BG_MAIN_LAST) {
            GFL_BGSysReleaseBG(sBGSetups[i].bg);
        }
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(ShinkaDemoBG));
}

static void ShinkaDemoBG_Update(ShinkaDemoBG *bg) {
}

static void ShinkaDemoBG_VBlank(ShinkaDemoBG *bg) {
    GFL_BGSysUpdate();
}

static void ShinkaDemoClAct_Init(ShinkaDemoClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(ShinkaDemoClAct));
    ClActSys_Create(&sClActSetup, vramConfig, heapId);
    clact->unit = ClActUnit_Create(128, 0, heapId);
    ClActUnit_UseSystemRenderer(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void ShinkaDemoClAct_Free(ShinkaDemoClAct *clact) {
    ClActUnit_Delete(clact->unit);
    ClActSys_Delete();
    MI_CpuFill8(clact, 0, sizeof(ShinkaDemoClAct));
}

static void ShinkaDemoClAct_Update(ShinkaDemoClAct *clact) {
    ClActSys_Draw();
}

static void ShinkaDemoClAct_VBlank(ShinkaDemoClAct *clact) {
    ClActSys_VBlank();
}

static ClActUnit *ShinkaDemoClAct_GetUnit(ShinkaDemoClAct *clact) {
    return clact->unit;
}

static void ShinkaDemoG3dSys_Init(ShinkaDemoG3dSys *g3d, HeapID heapId) {
    GFL_G3DSysCreate(FALSE, 2, 0, 1, 0, heapId, ShinkaDemoGraphic_G3DInit);
    // A 40 degree field of view
    g3d->camera = GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)),
                                      FX_CosIdx(DEG_TO_IDX(20)), FX32_CONST(4.0 / 3.0), 0, FX32_ONE, FX32_CONST(1024),
                                      0, &sCameraPosition, &sCameraUpVector, &sCameraTarget, heapId);
}

static void ShinkaDemoG3dSys_Free(ShinkaDemoG3dSys *g3d) {
    GFL_G3DCameraFree(g3d->camera);
    GFL_G3DSysFree();
}

static void ShinkaDemoG3dSys_Begin(ShinkaDemoG3dSys *g3d) {
    GFL_G3DSysReset();
    GFL_G3DCameraFlush(g3d->camera);
    GFL_G3DSysMtxViewFlush();
}

static void ShinkaDemoG3dSys_End(ShinkaDemoG3dSys *g3d) {
    GFL_G3DSysReqSwapBuffers();
}
