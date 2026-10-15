#include "types.h"
#include "app/demo3d/demo3d_graphic.h"
#include "app/demo3d/demo3d_data.h"
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

// The BGs, cell actors and 3D system of the 3D demos, named after the ROM's "demo3d_graphic.c". The file starts at the
// 3D system's set-up callback, as its siblings of the same template do (btl_rec_sel_graphic.c, psel_graphic.c). A
// demo that draws 3D on both screens has neither BGs nor cell actors

#define BG_COUNT 4
#define LIGHT_COUNT 4

typedef struct {
    u32 unk0;
} Demo3DBG;

typedef struct {
    ClActUnit *unit;
} Demo3DClAct;

typedef struct {
    u32 unk0;
    G3DLight *lights;
} Demo3DG3dSys;

struct Demo3DGraphic {
    Demo3DBG bg;
    Demo3DClAct clact;
    Demo3DG3dSys g3d;
    TCB *vblankTask;
    BOOL dual3D;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} Demo3DBGSetup;

static void Demo3DGraphic_G3DInit(void);
static void Demo3DGraphic_VBlank(TCB *tcb, void *data);
static void Demo3DBG_Init(Demo3DBG *bg, HeapID heapId);
static void Demo3DBG_Free(Demo3DBG *bg);
static void Demo3DBG_Update(Demo3DBG *bg);
static void Demo3DBG_VBlank(Demo3DBG *bg);
static void Demo3DClAct_Init(Demo3DClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void Demo3DClAct_Free(Demo3DClAct *clact);
static void Demo3DClAct_Update(Demo3DClAct *clact);
static void Demo3DClAct_VBlank(Demo3DClAct *clact);
static ClActUnit *Demo3DClAct_GetUnit(Demo3DClAct *clact);
static void Demo3DG3dSys_Init(Demo3DG3dSys *g3d, HeapID heapId);
static void Demo3DG3dSys_Free(Demo3DG3dSys *g3d);
static void Demo3DG3dSys_Begin(Demo3DG3dSys *g3d);
static void Demo3DG3dSys_End(Demo3DG3dSys *g3d);

static const LightSetup sDemo3DLightSetups[LIGHT_COUNT] = {
    { 0, { { -(FX16_ONE - 1), -(FX16_ONE - 1), -(FX16_ONE - 1) }, GX_RGB(31, 31, 31) } },
    { 1, { { FX16_ONE - 1, -(FX16_ONE - 1), -(FX16_ONE - 1) }, GX_RGB(31, 31, 31) } },
    { 2, { { -(FX16_ONE - 1), -(FX16_ONE - 1), -(FX16_ONE - 1) }, GX_RGB(31, 31, 31) } },
    { 3, { { -(FX16_ONE - 1), -(FX16_ONE - 1), -(FX16_ONE - 1) }, GX_RGB(31, 31, 31) } },
};

static const LightSetupList sDemo3DLightSetupList = { sDemo3DLightSetups, LIGHT_COUNT };

static const BGSysLCDConfig sDemo3DLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };
static const ClActSysSetup sDemo3DClActSetup = { 0, 0, 0, 512, 4, 124, 4, 112, 0, 32, 32, 32, 32, 16, 16 };

static const Light sDemo3DLights[LIGHT_COUNT] = {
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
};

// The VRAM banks of a demo with 3D on the upper screen only, and of one with 3D on both
static const BGSysVRAMConfig sDemo3DVRAMConfigs[] = {
    {
        GX_VRAM_BG_16_F, GX_VRAM_BGEXTPLTT_NONE, GX_VRAM_SUB_BG_128_C, GX_VRAM_SUB_BGEXTPLTT_NONE,
        GX_VRAM_OBJ_16_G, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I, GX_VRAM_SUB_OBJEXTPLTT_NONE,
        GX_VRAM_TEX_01_AB, GX_VRAM_TEXPLTT_0123_E, GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_32K,
    },
    {
        GX_VRAM_BG_16_F, GX_VRAM_BGEXTPLTT_NONE, GX_VRAM_SUB_BG_32_H, GX_VRAM_SUB_BGEXTPLTT_NONE,
        GX_VRAM_OBJ_16_G, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I, GX_VRAM_SUB_OBJEXTPLTT_NONE,
        GX_VRAM_TEX_01_AB, GX_VRAM_TEXPLTT_0123_E, GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_32K,
    },
};

// BG1 of the main screen and BG0 to BG2 of the sub screen, all hidden until a command or the demo's own code shows
// one
static const Demo3DBGSetup sDemo3DBGSetups[BG_COUNT] = {
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3800), GX_BG_CHARBASE(0), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      FALSE },
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0), GX_BG_CHARBASE(0x4000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      FALSE },
    { 5,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x8000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      FALSE },
    { 6,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_256, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x10000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      FALSE },
};

// The 3D system calls this once it is set up
static void Demo3DGraphic_G3DInit(void) {
    u32 i;

    GFL_BGSysSet3DBGPriority(0);
    G3X_SetShading(GX_SHADING_HIGHLIGHT);
    G3X_AntiAlias(FALSE);
    G3X_AlphaTest(FALSE, 0);
    G3X_AlphaBlend(TRUE);
    G3X_EdgeMarking(FALSE);
    gfxSetFog(FALSE, 0, 0, 0);
    gfxClearColor(GX_RGB(0, 0, 0), 0, 0x7fff, 63, FALSE);
    G3_ViewPort(0, 0, 255, 191);
    for (i = 0; i < NELEMS(sDemo3DLights); i++) {
        GFL_G3DSysLightSet(i, &sDemo3DLights[i]);
    }
    GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_AUTO, GX_BUFFERMODE_Z);
}

Demo3DGraphic *Demo3DGraphic_Create(u32 layout, u32 demoId, HeapID heapId) {
    Demo3DGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(Demo3DGraphic), FALSE, "demo3d_graphic.c", 346);

    sys_memset(graphic, 0, sizeof(Demo3DGraphic));
    graphic->dual3D = Demo3DData_IsDual3D(demoId);
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sDemo3DVRAMConfigs[graphic->dual3D]);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    func_020232d0();
    if (!graphic->dual3D) {
        Demo3DBG_Init(&graphic->bg, heapId);
        Demo3DClAct_Init(&graphic->clact, &sDemo3DVRAMConfigs[0], heapId);
        graphic->vblankTask = GFL_VBlankTCBAdd(Demo3DGraphic_VBlank, graphic, 0);
    }
    Demo3DG3dSys_Init(&graphic->g3d, heapId);
    return graphic;
}

void Demo3DGraphic_Free(Demo3DGraphic *graphic) {
    if (!graphic->dual3D) {
        GFL_TCBRemove(graphic->vblankTask);
    }
    Demo3DG3dSys_Free(&graphic->g3d);
    if (!graphic->dual3D) {
        Demo3DClAct_Free(&graphic->clact);
        Demo3DBG_Free(&graphic->bg);
    }
    func_020232d8();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    sys_memset(graphic, 0, sizeof(Demo3DGraphic));
    GFL_HeapFree(graphic);
}

void Demo3DGraphic_Update(Demo3DGraphic *graphic) {
    if (!graphic->dual3D) {
        Demo3DClAct_Update(&graphic->clact);
        Demo3DBG_Update(&graphic->bg);
    }
}

void Demo3DGraphic_Begin3D(Demo3DGraphic *graphic) {
    Demo3DG3dSys_Begin(&graphic->g3d);
}

void Demo3DGraphic_End3D(Demo3DGraphic *graphic) {
    Demo3DG3dSys_End(&graphic->g3d);
}

ClActUnit *Demo3DGraphic_GetClActUnit(Demo3DGraphic *graphic) {
    return Demo3DClAct_GetUnit(&graphic->clact);
}

void Demo3DGraphic_SetLightColors(Demo3DGraphic *graphic, const GXRgb *colors, u32 unused) {
    Demo3DG3dSys *g3d = &graphic->g3d;

    GFL_G3DLightSetColor(g3d->lights, 0, &colors[0]);
    GFL_G3DLightSetColor(g3d->lights, 1, &colors[1]);
    GFL_G3DLightSetColor(g3d->lights, 2, &colors[2]);
    GFL_G3DLightSetColor(g3d->lights, 3, &colors[3]);
    GFL_G3DLightFlush(g3d->lights);
}

void Demo3DGraphic_SetLightColor(Demo3DGraphic *graphic, u8 light, GXRgb color) {
    GFL_G3DLightSetColor(graphic->g3d.lights, light, &color);
}

void Demo3DGraphic_SetLightVector(Demo3DGraphic *graphic, u8 light, const VecFx16 *direction) {
    VecFx16 normal;

    VEC_Fx16Normalize(direction, &normal);
    GFL_G3DLightSetDirVector(graphic->g3d.lights, light, &normal);
}

static void Demo3DGraphic_VBlank(TCB *tcb, void *data) {
    Demo3DGraphic *graphic = data;

    if (!graphic->dual3D) {
        Demo3DBG_VBlank(&graphic->bg);
        Demo3DClAct_VBlank(&graphic->clact);
    }
}

static void Demo3DBG_Init(Demo3DBG *bg, HeapID heapId) {
    u32 i;

    sys_memset(bg, 0, sizeof(Demo3DBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sDemo3DLCDConfig);
    for (i = 0; i < NELEMS(sDemo3DBGSetups); i++) {
        GFL_BGSysCreateBG(sDemo3DBGSetups[i].bg, &sDemo3DBGSetups[i].setup, sDemo3DBGSetups[i].mode);
        GFL_BGSysClearBG(sDemo3DBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sDemo3DBGSetups[i].bg, sDemo3DBGSetups[i].enabled);
    }
    GFL_BGSysFillChar(1, 0, 1, 0);
}

static void Demo3DBG_Free(Demo3DBG *bg) {
    u32 i;

    GFL_BGSysFreeFilledChar(1, 1, 0);
    for (i = 0; i < NELEMS(sDemo3DBGSetups); i++) {
        GFL_BGSysSetBGEnabled(sDemo3DBGSetups[i].bg, FALSE);
        GFL_BGSysReleaseBG(sDemo3DBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    sys_memset(bg, 0, sizeof(Demo3DBG));
}

static void Demo3DBG_Update(Demo3DBG *bg) {
}

static void Demo3DBG_VBlank(Demo3DBG *bg) {
    GFL_BGSysUpdate();
}

static void Demo3DClAct_Init(Demo3DClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    sys_memset(clact, 0, sizeof(Demo3DClAct));
    ClActSys_Create(&sDemo3DClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(32, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void Demo3DClAct_Free(Demo3DClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    sys_memset(clact, 0, sizeof(Demo3DClAct));
}

static void Demo3DClAct_Update(Demo3DClAct *clact) {
    func_0204b794();
}

static void Demo3DClAct_VBlank(Demo3DClAct *clact) {
    func_0204b7c8();
}

static ClActUnit *Demo3DClAct_GetUnit(Demo3DClAct *clact) {
    return clact->unit;
}

static void Demo3DG3dSys_Init(Demo3DG3dSys *g3d, HeapID heapId) {
    GFL_G3DSysCreate(FALSE, 2, FALSE, 4, 0, heapId, Demo3DGraphic_G3DInit);
    g3d->lights = GFL_G3DLightCreate(&sDemo3DLightSetupList, heapId);
    GFL_G3DLightFlush(g3d->lights);
}

static void Demo3DG3dSys_Free(Demo3DG3dSys *g3d) {
    GFL_G3DLightFree(g3d->lights);
    GFL_G3DSysFree();
}

static void Demo3DG3dSys_Begin(Demo3DG3dSys *g3d) {
    GFL_G3DSysReset();
    GFL_G3DSysMtxViewFlush();
}

static void Demo3DG3dSys_End(Demo3DG3dSys *g3d) {
    GFL_G3DSysReqSwapBuffers();
}
