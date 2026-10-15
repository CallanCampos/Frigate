#include "types.h"
#include "app/subway_map.h"
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

// The BGs, cell actors and 3D system of the subway map

typedef struct {
    u32 unk0;
} SubwayMapBG;

typedef struct {
    ClActUnit *unit;
} SubwayMapClAct;

typedef struct {
    G3DCamera *camera;
} SubwayMapG3dSys;

struct SubwayMapGraphic {
    SubwayMapBG bg;
    SubwayMapClAct clact;
    SubwayMapG3dSys g3d;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} SubwayMapBGSetup;

static void SubwayMapGraphic_VBlank(TCB *tcb, void *data);
static void SubwayMapBG_Init(SubwayMapBG *bg, HeapID heapId);
static void SubwayMapBG_Free(SubwayMapBG *bg);
static void SubwayMapBG_Update(SubwayMapBG *bg);
static void SubwayMapBG_VBlank(SubwayMapBG *bg);
static void SubwayMapClAct_Init(SubwayMapClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void SubwayMapClAct_Free(SubwayMapClAct *clact);
static void SubwayMapClAct_Update(SubwayMapClAct *clact);
static void SubwayMapClAct_VBlank(SubwayMapClAct *clact);
static void SubwayMapG3dSys_Init(SubwayMapG3dSys *g3d, HeapID heapId);
static void SubwayMapG3dSys_Free(SubwayMapG3dSys *g3d);

static const VecFx32 sSubwayMapCameraTarget = { 0, FX32_ONE, 0 };
static const VecFx32 sSubwayMapCameraPosition = { FX32_CONST(6.7), FX32_CONST(6.7), FX32_CONST(17.3) };
static const VecFx32 sSubwayMapCameraUpVector = { 0, FX32_CONST(2.6), 0 };

static const BGSysLCDConfig sSubwayMapLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };
static const ClActSysSetup sSubwayMapClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

static const Light sSubwayMapLights[] = {
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
};

static const BGSysVRAMConfig sSubwayMapVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,        GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_D,   GX_VRAM_TEXPLTT_0_F,     GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const SubwayMapBGSetup sSubwayMapBGSetups[2] = {
    { 1,
      { 0, 0, 0x1000, 0, BGRES_256x512, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x4000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 2,
      { 0, 0, 0x1000, 0, BGRES_256x512, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x4000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

// The 3D system calls this once it is set up
static void SubwayMapGraphic_G3DInit(void) {
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
    for (i = 0; i < NELEMS(sSubwayMapLights); i++) {
        GFL_G3DSysLightSet(i, &sSubwayMapLights[i]);
    }
    GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_AUTO, GX_BUFFERMODE_Z);
}

SubwayMapGraphic *SubwayMapGraphic_Create(u32 layout, HeapID heapId) {
    SubwayMapGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(SubwayMapGraphic), FALSE, "subway_map_graphic.c", 460);

    sys_memset(graphic, 0, sizeof(SubwayMapGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sSubwayMapVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    func_020232d0();
    SubwayMapBG_Init(&graphic->bg, heapId);
    SubwayMapClAct_Init(&graphic->clact, &sSubwayMapVRAMConfig, heapId);
    SubwayMapG3dSys_Init(&graphic->g3d, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(SubwayMapGraphic_VBlank, graphic, 0);
    return graphic;
}

void SubwayMapGraphic_Free(SubwayMapGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    SubwayMapG3dSys_Free(&graphic->g3d);
    SubwayMapClAct_Free(&graphic->clact);
    SubwayMapBG_Free(&graphic->bg);
    func_020232d8();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    sys_memset(graphic, 0, sizeof(SubwayMapGraphic));
    GFL_HeapFree(graphic);
}

void SubwayMapGraphic_Update(SubwayMapGraphic *graphic) {
    SubwayMapClAct_Update(&graphic->clact);
    SubwayMapBG_Update(&graphic->bg);
}

static void SubwayMapGraphic_VBlank(TCB *tcb, void *data) {
    SubwayMapGraphic *graphic = data;

    SubwayMapBG_VBlank(&graphic->bg);
    SubwayMapClAct_VBlank(&graphic->clact);
}

static void SubwayMapBG_Init(SubwayMapBG *bg, HeapID heapId) {
    u32 i;

    sys_memset(bg, 0, sizeof(SubwayMapBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sSubwayMapLCDConfig);
    for (i = 0; i < NELEMS(sSubwayMapBGSetups); i++) {
        GFL_BGSysCreateBG(sSubwayMapBGSetups[i].bg, &sSubwayMapBGSetups[i].setup, sSubwayMapBGSetups[i].mode);
        GFL_BGSysClearBG(sSubwayMapBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sSubwayMapBGSetups[i].bg, sSubwayMapBGSetups[i].enabled);
    }
}

static void SubwayMapBG_Free(SubwayMapBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sSubwayMapBGSetups); i++) {
        GFL_BGSysReleaseBG(sSubwayMapBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    sys_memset(bg, 0, sizeof(SubwayMapBG));
}

static void SubwayMapBG_Update(SubwayMapBG *bg) {
}

static void SubwayMapBG_VBlank(SubwayMapBG *bg) {
    GFL_BGSysUpdate();
}

static void SubwayMapClAct_Init(SubwayMapClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    sys_memset(clact, 0, sizeof(SubwayMapClAct));
    ClActSys_Create(&sSubwayMapClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void SubwayMapClAct_Free(SubwayMapClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    sys_memset(clact, 0, sizeof(SubwayMapClAct));
}

static void SubwayMapClAct_Update(SubwayMapClAct *clact) {
    func_0204b794();
}

static void SubwayMapClAct_VBlank(SubwayMapClAct *clact) {
    func_0204b7c8();
}

static void SubwayMapG3dSys_Init(SubwayMapG3dSys *g3d, HeapID heapId) {
    GFL_G3DSysCreate(FALSE, 1, 0, 1, 0, heapId, SubwayMapGraphic_G3DInit);
    // A 40 degree field of view
    g3d->camera =
        GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)), FX_CosIdx(DEG_TO_IDX(20)),
                            FX32_CONST(4.0 / 3.0), 0, FX32_ONE, FX32_CONST(1024), 0, &sSubwayMapCameraPosition,
                            &sSubwayMapCameraUpVector, &sSubwayMapCameraTarget, heapId);
}

static void SubwayMapG3dSys_Free(SubwayMapG3dSys *g3d) {
    GFL_G3DCameraFree(g3d->camera);
    GFL_G3DSysFree();
}
