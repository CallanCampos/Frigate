#include "types.h"
#include "demo/the_end.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "system/gf_font.h"

// The BGs, cell actors and 3D system of the "THE END" picture

typedef struct {
    u32 unk0;
} TheEndBG;

typedef struct {
    ClActUnit *unit;
} TheEndClAct;

typedef struct {
    G3DCamera *camera;
} TheEndG3dSys;

struct TheEndGraphic {
    TheEndBG bg;
    TheEndClAct clact;
    TheEndG3dSys g3d;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} TheEndBGSetup;

static void TheEndGraphic_VBlank(TCB *tcb, void *data);
static void TheEndBG_Init(TheEndBG *bg, HeapID heapId);
static void TheEndBG_Free(TheEndBG *bg);
static void TheEndBG_Update(TheEndBG *bg);
static void TheEndBG_VBlank(TheEndBG *bg);
static void TheEndClAct_Init(TheEndClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void TheEndClAct_Free(TheEndClAct *clact);
static void TheEndClAct_Update(TheEndClAct *clact);
static void TheEndClAct_VBlank(TheEndClAct *clact);
static void TheEndG3dSys_Init(TheEndG3dSys *g3d, HeapID heapId);
static void TheEndG3dSys_Free(TheEndG3dSys *g3d);

static const VecFx32 sTheEndCameraTarget = { 0, FX32_ONE, 0 };
static const VecFx32 sTheEndCameraPosition = { FX32_CONST(6.7), FX32_CONST(6.7), FX32_CONST(17.3) };
static const VecFx32 sTheEndCameraUpVector = { 0, FX32_CONST(2.6), 0 };

static const BGSysLCDConfig sTheEndLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };
static const ClActSysSetup sTheEndClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

static const Light sTheEndLights[] = {
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
};

static const BGSysVRAMConfig sTheEndVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,        GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_D,   GX_VRAM_TEXPLTT_0_F,     GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const TheEndBGSetup sTheEndBGSetups[2] = {
    { 1,
      { 0, 0, 0x1000, 0, BGRES_256x512, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x4000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 2,
      { 0, 0, 0x1000, 0, BGRES_256x256, GX_BG_COLORMODE_256, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

// The 3D system calls this once it is set up
static void TheEndGraphic_G3DInit(void) {
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
    for (i = 0; i < NELEMS(sTheEndLights); i++) {
        GFL_G3DSysLightSet(i, &sTheEndLights[i]);
    }
    GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_AUTO, GX_BUFFERMODE_Z);
}

TheEndGraphic *TheEndGraphic_Create(u32 layout, HeapID heapId) {
    TheEndGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(TheEndGraphic), FALSE, "the_end_graphic.c", 460);

    MI_CpuFill8(graphic, 0, sizeof(TheEndGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sTheEndVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GFL_TextRndInitColorIndexLUT();
    TheEndBG_Init(&graphic->bg, heapId);
    TheEndClAct_Init(&graphic->clact, &sTheEndVRAMConfig, heapId);
    TheEndG3dSys_Init(&graphic->g3d, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(TheEndGraphic_VBlank, graphic, 0);
    return graphic;
}

void TheEndGraphic_Free(TheEndGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    TheEndG3dSys_Free(&graphic->g3d);
    TheEndClAct_Free(&graphic->clact);
    TheEndBG_Free(&graphic->bg);
    GFL_TextRndResetColorIndexLUT();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(TheEndGraphic));
    GFL_HeapFree(graphic);
}

void TheEndGraphic_Update(TheEndGraphic *graphic) {
    TheEndClAct_Update(&graphic->clact);
    TheEndBG_Update(&graphic->bg);
}

static void TheEndGraphic_VBlank(TCB *tcb, void *data) {
    TheEndGraphic *graphic = data;

    TheEndBG_VBlank(&graphic->bg);
    TheEndClAct_VBlank(&graphic->clact);
}

static void TheEndBG_Init(TheEndBG *bg, HeapID heapId) {
    u32 i;

    MI_CpuFill8(bg, 0, sizeof(TheEndBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sTheEndLCDConfig);
    for (i = 0; i < NELEMS(sTheEndBGSetups); i++) {
        GFL_BGSysCreateBG(sTheEndBGSetups[i].bg, &sTheEndBGSetups[i].setup, sTheEndBGSetups[i].mode);
        GFL_BGSysClearBG(sTheEndBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sTheEndBGSetups[i].bg, sTheEndBGSetups[i].enabled);
    }
}

static void TheEndBG_Free(TheEndBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sTheEndBGSetups); i++) {
        GFL_BGSysReleaseBG(sTheEndBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(TheEndBG));
}

static void TheEndBG_Update(TheEndBG *bg) {
}

static void TheEndBG_VBlank(TheEndBG *bg) {
    GFL_BGSysUpdate();
}

static void TheEndClAct_Init(TheEndClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(TheEndClAct));
    ClActSys_Create(&sTheEndClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void TheEndClAct_Free(TheEndClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    MI_CpuFill8(clact, 0, sizeof(TheEndClAct));
}

static void TheEndClAct_Update(TheEndClAct *clact) {
    func_0204b794();
}

static void TheEndClAct_VBlank(TheEndClAct *clact) {
    func_0204b7c8();
}

static void TheEndG3dSys_Init(TheEndG3dSys *g3d, HeapID heapId) {
    GFL_G3DSysCreate(FALSE, 1, 0, 1, 0, heapId, TheEndGraphic_G3DInit);
    // A 40 degree field of view
    g3d->camera = GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)),
                                      FX_CosIdx(DEG_TO_IDX(20)), FX32_CONST(4.0 / 3.0), 0, FX32_ONE, FX32_CONST(1024),
                                      0, &sTheEndCameraPosition, &sTheEndCameraUpVector, &sTheEndCameraTarget, heapId);
}

static void TheEndG3dSys_Free(TheEndG3dSys *g3d) {
    GFL_G3DCameraFree(g3d->camera);
    GFL_G3DSysFree();
}
