#include "types.h"
#include "app/un_select_graphic.h"
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

// The BGs, cell actors and 3D system of the Unity Tower's floor select, named after the ROM's "un_select_graphic.c".
// The names are ours

typedef struct {
    u32 unk0;
} UnSelectBG;

typedef struct {
    ClActUnit *unit;
} UnSelectClAct;

typedef struct {
    G3DCamera *camera;
} UnSelectG3dSys;

struct UnSelectGraphic {
    UnSelectBG bg;
    UnSelectClAct clact;
    UnSelectG3dSys g3d;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} UnSelectBGSetup;

static void UnSelectGraphic_VBlank(TCB *tcb, void *data);
static void UnSelectBG_Init(UnSelectBG *bg, HeapID heapId);
static void UnSelectBG_Free(UnSelectBG *bg);
static void UnSelectBG_Update(UnSelectBG *bg);
static void UnSelectBG_VBlank(UnSelectBG *bg);
static void UnSelectClAct_Init(UnSelectClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void UnSelectClAct_Free(UnSelectClAct *clact);
static void UnSelectClAct_Update(UnSelectClAct *clact);
static void UnSelectClAct_VBlank(UnSelectClAct *clact);
static ClActUnit *UnSelectClAct_GetUnit(UnSelectClAct *clact);
static void UnSelectG3dSys_Init(UnSelectG3dSys *g3d, HeapID heapId);
static void UnSelectG3dSys_Free(UnSelectG3dSys *g3d);

static const VecFx32 sUnSelectCameraUpVector = { 0, FX32_CONST(2.6), 0 };
static const VecFx32 sUnSelectCameraPosition = { FX32_CONST(6.7), FX32_CONST(6.7), FX32_CONST(17.3) };
static const VecFx32 sUnSelectCameraTarget = { 0, FX32_ONE, 0 };

static const ClActSysSetup sUnSelectClActSetup = { 0, 192, 0, 0, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };
static const BGSysLCDConfig sUnSelectLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

static const Light sUnSelectLights[] = {
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
};

static const BGSysVRAMConfig sUnSelectVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_D,   GX_VRAM_TEXPLTT_0_F,     GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const UnSelectBGSetup sUnSelectBGSetups[] = {
    { 0,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x4000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x08000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 2,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 3,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x18000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 6,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x08000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 7,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

// The 3D system calls this once it is set up
static void UnSelectGraphic_G3DInit(void) {
    u32 i;

    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, TRUE);
    G2_SetBG0Priority(0);
    G3X_SetShading(GX_SHADING_HIGHLIGHT);
    G3X_AntiAlias(FALSE);
    G3X_AlphaTest(FALSE, 0);
    G3X_AlphaBlend(TRUE);
    G3X_EdgeMarking(FALSE);
    gfxSetFog(FALSE, 0, 0, 0);
    gfxClearColor(GX_RGB(0, 0, 0), 0, 0x7fff, 63, FALSE);
    G3_ViewPort(0, 0, 255, 191);
    for (i = 0; i < NELEMS(sUnSelectLights); i++) {
        GFL_G3DSysLightSet(i, &sUnSelectLights[i]);
    }
    GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_AUTO, GX_BUFFERMODE_Z);
}

UnSelectGraphic *UnSelectGraphic_Create(u32 layout, HeapID heapId) {
    UnSelectGraphic *graphic =
        GFL_HeapAllocate(heapId, sizeof(UnSelectGraphic), FALSE, "un_select_graphic.c", 436);

    sys_memset(graphic, 0, sizeof(UnSelectGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sUnSelectVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    func_020232d0();
    UnSelectBG_Init(&graphic->bg, heapId);
    UnSelectClAct_Init(&graphic->clact, &sUnSelectVRAMConfig, heapId);
    UnSelectG3dSys_Init(&graphic->g3d, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(UnSelectGraphic_VBlank, graphic, 0);
    return graphic;
}

void UnSelectGraphic_Free(UnSelectGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    UnSelectG3dSys_Free(&graphic->g3d);
    UnSelectClAct_Free(&graphic->clact);
    UnSelectBG_Free(&graphic->bg);
    func_020232d8();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    sys_memset(graphic, 0, sizeof(UnSelectGraphic));
    GFL_HeapFree(graphic);
}

void UnSelectGraphic_Update(UnSelectGraphic *graphic) {
    UnSelectClAct_Update(&graphic->clact);
    UnSelectBG_Update(&graphic->bg);
}

ClActUnit *UnSelectGraphic_GetClActUnit(UnSelectGraphic *graphic) {
    return UnSelectClAct_GetUnit(&graphic->clact);
}

static void UnSelectGraphic_VBlank(TCB *tcb, void *data) {
    UnSelectGraphic *graphic = data;

    UnSelectBG_VBlank(&graphic->bg);
    UnSelectClAct_VBlank(&graphic->clact);
}

static void UnSelectBG_Init(UnSelectBG *bg, HeapID heapId) {
    u32 i;

    sys_memset(bg, 0, sizeof(UnSelectBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sUnSelectLCDConfig);
    for (i = 0; i < NELEMS(sUnSelectBGSetups); i++) {
        GFL_BGSysCreateBG(sUnSelectBGSetups[i].bg, &sUnSelectBGSetups[i].setup,
                          sUnSelectBGSetups[i].mode);
        GFL_BGSysClearBG(sUnSelectBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sUnSelectBGSetups[i].bg, sUnSelectBGSetups[i].enabled);
    }
}

static void UnSelectBG_Free(UnSelectBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sUnSelectBGSetups); i++) {
        GFL_BGSysReleaseBG(sUnSelectBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    sys_memset(bg, 0, sizeof(UnSelectBG));
}

static void UnSelectBG_Update(UnSelectBG *bg) {
}

static void UnSelectBG_VBlank(UnSelectBG *bg) {
    GFL_BGSysUpdate();
}

static void UnSelectClAct_Init(UnSelectClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    sys_memset(clact, 0, sizeof(UnSelectClAct));
    ClActSys_Create(&sUnSelectClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void UnSelectClAct_Free(UnSelectClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    sys_memset(clact, 0, sizeof(UnSelectClAct));
}

static void UnSelectClAct_Update(UnSelectClAct *clact) {
    func_0204b794();
}

static void UnSelectClAct_VBlank(UnSelectClAct *clact) {
    func_0204b7c8();
}

static ClActUnit *UnSelectClAct_GetUnit(UnSelectClAct *clact) {
    return clact->unit;
}

static void UnSelectG3dSys_Init(UnSelectG3dSys *g3d, HeapID heapId) {
    GFL_G3DSysCreate(FALSE, 1, 0, 1, 0, heapId, UnSelectGraphic_G3DInit);
    // A 40 degree field of view
    g3d->camera =
        GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)), FX_CosIdx(DEG_TO_IDX(20)),
                            FX32_CONST(4.0 / 3.0), 0, FX32_ONE, FX32_CONST(1024), 0, &sUnSelectCameraPosition,
                            &sUnSelectCameraUpVector, &sUnSelectCameraTarget, heapId);
}

static void UnSelectG3dSys_Free(UnSelectG3dSys *g3d) {
    GFL_G3DCameraFree(g3d->camera);
    GFL_G3DSysFree();
}
