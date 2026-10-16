#include "types.h"
#include "demo/intro.h"
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

// The intro's BGs, cell actors and 3D system. Once the player or the rival has been named, the intro sets them up
// differently

typedef struct {
    u32 unk0;
} IntroBG;

typedef struct {
    ClActUnit *unit;
} IntroClAct;

typedef struct {
    G3DCamera *camera;
    G3DLight *light;
} IntroG3dSys;

struct IntroGraphic {
    IntroBG bg;
    IntroClAct clact;
    IntroG3dSys g3d;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} IntroBGSetup;

static void IntroGraphic_VBlank(TCB *tcb, void *data);
static void IntroBG_Init(IntroBG *bg, HeapID heapId);
static void IntroBG_Free(IntroBG *bg);
static void IntroBG_Update(IntroBG *bg);
static void IntroBG_VBlank(IntroBG *bg);
static void IntroBG_InitNamed(IntroBG *bg, HeapID heapId);
static void IntroClAct_Init(IntroClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void IntroClAct_Free(IntroClAct *clact);
static void IntroClAct_Update(IntroClAct *clact);
static void IntroClAct_VBlank(IntroClAct *clact);
static ClActUnit *IntroClAct_GetUnit(IntroClAct *clact);
static void IntroG3dSys_Init(IntroG3dSys *g3d, HeapID heapId);
static void IntroG3dSys_InitNamed(IntroG3dSys *g3d, HeapID heapId);
static void IntroG3dSys_Free(IntroG3dSys *g3d);
static void IntroG3dSys_Begin(IntroG3dSys *g3d);
static void IntroG3dSys_End(IntroG3dSys *g3d);

// clang-format off
static const BGSysVRAMConfig sVRAMConfig = {
    GX_VRAM_BG_16_F,       GX_VRAM_BGEXTPLTT_NONE,     GX_VRAM_SUB_BG_32_H,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_16_G,      GX_VRAM_OBJEXTPLTT_NONE,    GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0123_ABCD, GX_VRAM_TEXPLTT_0123_E,     GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

// clang-format on

// clang-format off
static const BGSysVRAMConfig sVRAMConfigNamed = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,      GX_VRAM_SUB_BG_32_H,         GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE,     GX_VRAM_SUB_OBJ_16_I,        GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_01_CD, GX_VRAM_TEXPLTT_0123_E,      GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

// clang-format on

static const VecFx32 sCameraPosition = { 0, FX32_CONST(0.043), FX32_CONST(70) };
static const VecFx32 sCameraUpVector = { 0, FX32_CONST(2.6), 0 };
static const VecFx32 sCameraTarget = { 0, FX32_ONE, 0 };

static const Light sLights[] = {
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
};

// clang-format off
static const IntroBGSetup sBGSetups[] = {
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x00000), 0x4000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 3,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x00000), 0x4000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x7800), GX_BG_CHARBASE(0x00000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 6,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x7000), GX_BG_CHARBASE(0x00000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
};

// clang-format on

static const BGSysLCDConfig sLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };
static const ClActSysSetup sClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

// The 3D system calls this once it is set up
static void IntroGraphic_G3DInit(void) {
    u32 i;

    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, TRUE);
    G2_SetBG0Priority(1);
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

IntroGraphic *IntroGraphic_Create(u32 layout, u32 mode, HeapID heapId) {
    IntroGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(IntroGraphic), FALSE, "intro_graphic.c", 406);

    MI_CpuFill8(graphic, 0, sizeof(IntroGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    if (mode == INTRO_MODE_PLAYER_NAMED || mode == INTRO_MODE_RIVAL_NAMED) {
        GFL_BGSysSetVRAMBanks(&sVRAMConfigNamed);
        IntroBG_InitNamed(&graphic->bg, heapId);
        IntroClAct_Init(&graphic->clact, &sVRAMConfig, heapId);
        IntroG3dSys_InitNamed(&graphic->g3d, heapId);
    } else {
        GFL_BGSysSetVRAMBanks(&sVRAMConfig);
        IntroBG_Init(&graphic->bg, heapId);
        IntroClAct_Init(&graphic->clact, &sVRAMConfig, heapId);
        IntroG3dSys_Init(&graphic->g3d, heapId);
    }
    func_020232d0();
    graphic->vblankTask = GFL_VBlankTCBAdd(IntroGraphic_VBlank, graphic, 0);
    return graphic;
}

void IntroGraphic_Free(IntroGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    IntroG3dSys_Free(&graphic->g3d);
    IntroClAct_Free(&graphic->clact);
    IntroBG_Free(&graphic->bg);
    func_020232d8();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(IntroGraphic));
    GFL_HeapFree(graphic);
}

void IntroGraphic_Update(IntroGraphic *graphic) {
    IntroClAct_Update(&graphic->clact);
    IntroBG_Update(&graphic->bg);
}

void IntroGraphic_Begin3D(IntroGraphic *graphic) {
    IntroG3dSys_Begin(&graphic->g3d);
}

void IntroGraphic_End3D(IntroGraphic *graphic) {
    IntroG3dSys_End(&graphic->g3d);
}

ClActUnit *IntroGraphic_GetClActUnit(IntroGraphic *graphic) {
    return IntroClAct_GetUnit(&graphic->clact);
}

static void IntroGraphic_VBlank(TCB *tcb, void *data) {
    IntroGraphic *graphic = data;

    IntroBG_VBlank(&graphic->bg);
    IntroClAct_VBlank(&graphic->clact);
}

static void IntroBG_Init(IntroBG *bg, HeapID heapId) {
    u32 i;

    MI_CpuFill8(bg, 0, sizeof(IntroBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sLCDConfig);
    for (i = 0; i < NELEMS(sBGSetups); i++) {
        GFL_BGSysCreateBG(sBGSetups[i].bg, &sBGSetups[i].setup, sBGSetups[i].mode);
        GFL_BGSysClearBG(sBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sBGSetups[i].bg, sBGSetups[i].enabled);
    }
}

static void IntroBG_Free(IntroBG *bg) {
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(IntroBG));
}

// Scrolls BG 2 to the right, one pixel each frame
static void IntroBG_Update(IntroBG *bg) {
    if (GFL_BGSysGetEnabledBGsA() & GX_PLANEMASK_BG2) {
        GFL_BGSysMoveBGReq(2, BG_MOVE_RIGHT, 1);
    }
}

static void IntroBG_VBlank(IntroBG *bg) {
    GFL_BGSysUpdate();
}

static void IntroBG_InitNamed(IntroBG *bg, HeapID heapId) {
    MI_CpuFill8(bg, 0, sizeof(IntroBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sLCDConfig);
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xf800),
                          GX_BG_CHARBASE(0x00000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          0,
                          GX_BG_AREAOVER_XLU,
                          FALSE };
        GFL_BGSysCreateBG(1, &setup, BGMODE_TEXT);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x1000,
                          0,
                          BGRES_512x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xe800),
                          GX_BG_CHARBASE(0x10000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          2,
                          GX_BG_AREAOVER_XLU,
                          FALSE };
        GFL_BGSysCreateBG(2, &setup, BGMODE_TEXT);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xe000),
                          GX_BG_CHARBASE(0x18000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          3,
                          GX_BG_AREAOVER_XLU,
                          FALSE };
        GFL_BGSysCreateBG(3, &setup, BGMODE_TEXT);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0x7800),
                          GX_BG_CHARBASE(0x00000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          0,
                          GX_BG_AREAOVER_XLU,
                          FALSE };
        GFL_BGSysCreateBG(4, &setup, BGMODE_TEXT);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0x7000),
                          GX_BG_CHARBASE(0x00000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          2,
                          GX_BG_AREAOVER_XLU,
                          FALSE };
        GFL_BGSysCreateBG(6, &setup, BGMODE_TEXT);
    }
    GFL_BGSysSetBGEnabled(1, TRUE);
    GFL_BGSysSetBGEnabled(3, TRUE);
    GFL_BGSysSetBGEnabled(4, TRUE);
    GFL_BGSysSetBGEnabled(6, TRUE);
}

static void IntroClAct_Init(IntroClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(IntroClAct));
    ClActSys_Create(&sClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void IntroClAct_Free(IntroClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    MI_CpuFill8(clact, 0, sizeof(IntroClAct));
}

static void IntroClAct_Update(IntroClAct *clact) {
    func_0204b794();
}

static void IntroClAct_VBlank(IntroClAct *clact) {
    func_0204b7c8();
}

static ClActUnit *IntroClAct_GetUnit(IntroClAct *clact) {
    return clact->unit;
}

// The compiler lays out data by size, breaking ties in an order that depends on where each object is declared. These
// are declared after IntroBG_InitNamed's BG setups to keep the original layout
static const LightSetup sLightSetups[] = {
    { 0, { { -(FX16_ONE - 1), -(FX16_ONE - 1), -(FX16_ONE - 1) }, GX_RGB(31, 31, 31) } },
    { 1, { { FX16_ONE - 1, -(FX16_ONE - 1), -(FX16_ONE - 1) }, GX_RGB(31, 31, 31) } },
    { 2, { { -(FX16_ONE - 1), -(FX16_ONE - 1), -(FX16_ONE - 1) }, GX_RGB(31, 31, 31) } },
    { 3, { { -(FX16_ONE - 1), -(FX16_ONE - 1), -(FX16_ONE - 1) }, GX_RGB(31, 31, 31) } },
};

static const LightSetupList sLightSetupList = { sLightSetups, NELEMS(sLightSetups) };

static const LightSetup sLightSetupsNamed[] = {
    { 0, { { -(FX16_ONE - 1), -(FX16_ONE - 1), -(FX16_ONE - 1) }, GX_RGB(31, 31, 31) } },
    { 1, { { FX16_ONE - 1, -(FX16_ONE - 1), -(FX16_ONE - 1) }, GX_RGB(31, 31, 31) } },
    { 2, { { -(FX16_ONE - 1), -(FX16_ONE - 1), -(FX16_ONE - 1) }, GX_RGB(31, 31, 31) } },
    { 3, { { -(FX16_ONE - 1), -(FX16_ONE - 1), -(FX16_ONE - 1) }, GX_RGB(31, 31, 31) } },
};

static const LightSetupList sLightSetupListNamed = { sLightSetupsNamed, NELEMS(sLightSetupsNamed) };

static void IntroG3dSys_Init(IntroG3dSys *g3d, HeapID heapId) {
    fx32 far;
    fx32 near;

    GFL_G3DSysCreate(FALSE, 4, 0, 2, 0, heapId, IntroGraphic_G3DInit);
    // A 40 degree field of view
    g3d->camera = GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)),
                                      FX_CosIdx(DEG_TO_IDX(20)), FX32_CONST(4.0 / 3.0), 0, FX32_ONE, FX32_CONST(1024),
                                      0, &sCameraPosition, &sCameraUpVector, &sCameraTarget, heapId);
    far = FX32_CONST(2048);
    near = FX32_CONST(0.1);
    GFL_G3DCameraSetProjectionZFar(g3d->camera, &far);
    GFL_G3DCameraSetProjectionZNear(g3d->camera, &near);
    g3d->light = GFL_G3DLightCreate(&sLightSetupList, heapId);
    GFL_G3DLightFlush(g3d->light);
}

static void IntroG3dSys_InitNamed(IntroG3dSys *g3d, HeapID heapId) {
    fx32 far;
    fx32 near;

    GFL_G3DSysCreate(FALSE, 2, 0, 2, 0, heapId, IntroGraphic_G3DInit);
    g3d->camera = GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)),
                                      FX_CosIdx(DEG_TO_IDX(20)), FX32_CONST(4.0 / 3.0), 0, FX32_ONE, FX32_CONST(1024),
                                      0, &sCameraPosition, &sCameraUpVector, &sCameraTarget, heapId);
    far = FX32_CONST(2048);
    near = FX32_CONST(0.1);
    GFL_G3DCameraSetProjectionZFar(g3d->camera, &far);
    GFL_G3DCameraSetProjectionZNear(g3d->camera, &near);
    g3d->light = GFL_G3DLightCreate(&sLightSetupListNamed, heapId);
    GFL_G3DLightFlush(g3d->light);
}

static void IntroG3dSys_Free(IntroG3dSys *g3d) {
    GFL_G3DLightFree(g3d->light);
    GFL_G3DCameraFree(g3d->camera);
    GFL_G3DSysFree();
}

static void IntroG3dSys_Begin(IntroG3dSys *g3d) {
    GFL_G3DCameraFlush(g3d->camera);
    GFL_G3DSysReset();
    GFL_G3DSysMtxViewFlush();
}

static void IntroG3dSys_End(IntroG3dSys *g3d) {
    GFL_G3DSysReqSwapBuffers();
}
