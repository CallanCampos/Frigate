#include "types.h"
#include "app/psel_graphic.h"
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

// The BGs, cell actors and 3D system of the starter Pokémon choice, named after the ROM's "psel_graphic.c"

typedef struct {
    u32 unk0;
} PselBG;

typedef struct {
    ClActUnit *unit;
} PselClAct;

typedef struct {
    G3DCamera *camera;
} PselG3dSys;

struct PselGraphic {
    PselBG bg;
    PselClAct clact;
    PselG3dSys g3d;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} PselBGSetup;

static void PselGraphic_VBlank(TCB *tcb, void *data);
static void PselBG_Init(PselBG *bg, HeapID heapId);
static void PselBG_Free(PselBG *bg);
static void PselBG_Update(PselBG *bg);
static void PselBG_VBlank(PselBG *bg);
static void PselClAct_Init(PselClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void PselClAct_Free(PselClAct *clact);
static void PselClAct_Update(PselClAct *clact);
static void PselClAct_VBlank(PselClAct *clact);
static ClActUnit *PselClAct_GetUnit(PselClAct *clact);
static void PselG3dSys_Init(PselG3dSys *g3d, HeapID heapId);
static void PselG3dSys_Free(PselG3dSys *g3d);
static void PselG3dSys_Begin(PselG3dSys *g3d);
static void PselG3dSys_End(PselG3dSys *g3d);
static G3DCamera *PselG3dSys_GetCamera(PselG3dSys *g3d);

static const VecFx32 sPselCameraTarget = { 0, 0, 0 };
static const VecFx32 sPselCameraPosition = { FX32_CONST(6.7), FX32_CONST(6.7), FX32_CONST(17.3) };
static const VecFx32 sPselCameraUpVector = { 0, FX32_CONST(2.6), 0 };

static const BGSysLCDConfig sPselLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };
static const ClActSysSetup sPselClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

static const Light sPselLights[] = {
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
};

static const BGSysVRAMConfig sPselVRAMConfig = {
    GX_VRAM_BG_64_E,   GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_16_F,  GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,      GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_01_AB, GX_VRAM_TEXPLTT_0_G,     GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

// BG1 and BG2 of the main screen, and all four of the sub screen
static const PselBGSetup sPselBGSetups[6] = {
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x4000), 0x4000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 2,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x8000), 0x4000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_256, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x4000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 5,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_256, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x4000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 6,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0xc000), 0x4000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 7,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1800), GX_BG_CHARBASE(0x10000), 0x4000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

// The 3D system calls this once it is set up
static void PselGraphic_G3DInit(void) {
    u32 i;

    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, TRUE);
    G2_SetBG0Priority(0);
    G3X_SetShading(GX_SHADING_HIGHLIGHT);
    G3X_AntiAlias(TRUE);
    G3X_AlphaTest(FALSE, 0);
    G3X_AlphaBlend(TRUE);
    G3X_EdgeMarking(TRUE);
    gfxSetFog(FALSE, 0, 0, 0);
    gfxClearColor(GX_RGB(0, 0, 0), 0, 0x7fff, 63, FALSE);
    G3_ViewPort(0, 0, 255, 191);
    for (i = 0; i < NELEMS(sPselLights); i++) {
        GFL_G3DSysLightSet(i, &sPselLights[i]);
    }
    GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_AUTO, GX_BUFFERMODE_W);
}

PselGraphic *PselGraphic_Create(u32 layout, HeapID heapId) {
    PselGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(PselGraphic), FALSE, "psel_graphic.c", 465);

    sys_memset(graphic, 0, sizeof(PselGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sPselVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    func_020232d0();
    PselBG_Init(&graphic->bg, heapId);
    PselClAct_Init(&graphic->clact, &sPselVRAMConfig, heapId);
    PselG3dSys_Init(&graphic->g3d, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(PselGraphic_VBlank, graphic, 0);
    return graphic;
}

void PselGraphic_Free(PselGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    PselG3dSys_Free(&graphic->g3d);
    PselClAct_Free(&graphic->clact);
    PselBG_Free(&graphic->bg);
    func_020232d8();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    sys_memset(graphic, 0, sizeof(PselGraphic));
    GFL_HeapFree(graphic);
}

void PselGraphic_Update(PselGraphic *graphic) {
    PselClAct_Update(&graphic->clact);
    PselBG_Update(&graphic->bg);
}

void PselGraphic_Begin3D(PselGraphic *graphic) {
    PselG3dSys_Begin(&graphic->g3d);
}

void PselGraphic_End3D(PselGraphic *graphic) {
    PselG3dSys_End(&graphic->g3d);
}

ClActUnit *PselGraphic_GetClActUnit(PselGraphic *graphic) {
    return PselClAct_GetUnit(&graphic->clact);
}

G3DCamera *PselGraphic_GetCamera(PselGraphic *graphic) {
    return PselG3dSys_GetCamera(&graphic->g3d);
}

static void PselGraphic_VBlank(TCB *tcb, void *data) {
    PselGraphic *graphic = data;

    PselBG_VBlank(&graphic->bg);
    PselClAct_VBlank(&graphic->clact);
}

static void PselBG_Init(PselBG *bg, HeapID heapId) {
    u32 i;

    sys_memset(bg, 0, sizeof(PselBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sPselLCDConfig);
    for (i = 0; i < NELEMS(sPselBGSetups); i++) {
        GFL_BGSysCreateBG(sPselBGSetups[i].bg, &sPselBGSetups[i].setup, sPselBGSetups[i].mode);
        GFL_BGSysClearBG(sPselBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sPselBGSetups[i].bg, sPselBGSetups[i].enabled);
    }
}

static void PselBG_Free(PselBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sPselBGSetups); i++) {
        GFL_BGSysReleaseBG(sPselBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    sys_memset(bg, 0, sizeof(PselBG));
}

static void PselBG_Update(PselBG *bg) {
}

static void PselBG_VBlank(PselBG *bg) {
    GFL_BGSysUpdate();
}

static void PselClAct_Init(PselClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    sys_memset(clact, 0, sizeof(PselClAct));
    ClActSys_Create(&sPselClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void PselClAct_Free(PselClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    sys_memset(clact, 0, sizeof(PselClAct));
}

static void PselClAct_Update(PselClAct *clact) {
    func_0204b794();
}

static void PselClAct_VBlank(PselClAct *clact) {
    func_0204b7c8();
}

static ClActUnit *PselClAct_GetUnit(PselClAct *clact) {
    return clact->unit;
}

static void PselG3dSys_Init(PselG3dSys *g3d, HeapID heapId) {
    GFL_G3DSysCreate(FALSE, 2, 0, 1, 0x1000, heapId, PselGraphic_G3DInit);
    g3d->camera =
        GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(85)), FX_CosIdx(DEG_TO_IDX(85)),
                            FX32_CONST(4.0 / 3.0), 0, FX32_ONE, FX32_CONST(1024), 0, &sPselCameraPosition,
                            &sPselCameraUpVector, &sPselCameraTarget, heapId);
}

static void PselG3dSys_Free(PselG3dSys *g3d) {
    GFL_G3DCameraFree(g3d->camera);
    GFL_G3DSysFree();
}

static void PselG3dSys_Begin(PselG3dSys *g3d) {
    GFL_G3DSysReset();
    GFL_G3DCameraFlush(g3d->camera);
    GFL_G3DSysMtxViewFlush();
}

static void PselG3dSys_End(PselG3dSys *g3d) {
    GFL_G3DSysReqSwapBuffers();
}

static G3DCamera *PselG3dSys_GetCamera(PselG3dSys *g3d) {
    return g3d->camera;
}
