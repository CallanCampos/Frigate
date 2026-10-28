// The ROM names this file zukan_detail_graphic.c.
#include "types.h"
#include "app/pokedex_detail.h"
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

// The detail screen's BGs, cell actors and 3D system. The 3D system is only created for the pages that use it

typedef struct {
    u32 unk0;
} PokedexDetailBG;

typedef struct {
    ClActUnit *unit;
} PokedexDetailClAct;

typedef struct {
    G3DCamera *camera;
} PokedexDetailG3dSys;

struct PokedexDetailGraphic {
    PokedexDetailBG bg;
    PokedexDetailClAct clact;
    PokedexDetailG3dSys g3d;
    TCB *vblankTask;
    BOOL g3dCreated;
};

static void PokedexDetailGraphic_VBlank(TCB *tcb, void *data);
static void PokedexDetailBG_Init(PokedexDetailBG *bg, HeapID heapId);
static void PokedexDetailBG_Free(PokedexDetailBG *bg);
static void PokedexDetailBG_Update(PokedexDetailBG *bg);
static void PokedexDetailBG_VBlank(PokedexDetailBG *bg);
static void PokedexDetailClAct_Init(PokedexDetailClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void PokedexDetailClAct_Free(PokedexDetailClAct *clact);
static void PokedexDetailClAct_Update(PokedexDetailClAct *clact);
static void PokedexDetailClAct_VBlank(PokedexDetailClAct *clact);
static ClActUnit *PokedexDetailClAct_GetUnit(PokedexDetailClAct *clact);
static void PokedexDetailG3dSys_Init(PokedexDetailG3dSys *g3d, HeapID heapId);
static void PokedexDetailG3dSys_Free(PokedexDetailG3dSys *g3d);
static void PokedexDetailG3dSys_Begin(PokedexDetailG3dSys *g3d);
static void PokedexDetailG3dSys_End(PokedexDetailG3dSys *g3d);

static const BGSysVRAMConfig sVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_23_G,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,        GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_D,   GX_VRAM_TEXPLTT_0123_E,  GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static const VecFx32 sCameraPosition = { 0, 0, FX32_CONST(70) };
static const VecFx32 sCameraTarget = { 0, 0, 0 };
static const VecFx32 sCameraUpVector = { 0, FX32_ONE, 0 };

static const PokedexDetailBGSetup sBGSetups[BGSYS_BG_COUNT] = {
    { 0,
      { 0, 0, 0x1000, 0, BGRES_256x512, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x0c000), 0x4000,
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
      { 0, 0, 0x1000, 0, BGRES_256x512, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 5,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x0c000), 0x4000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 6,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 7,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x18000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

static const Light sLights[] = {
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
};

static const BGSysLCDConfig sLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };
static const ClActSysSetup sClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

// The 3D system calls this once it is set up
static void PokedexDetailGraphic_G3DInit(void) {
    u32 i;

    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, TRUE);
    G2_SetBG0Priority(1);
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
    GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_AUTO, GX_BUFFERMODE_Z);
}

PokedexDetailGraphic *PokedexDetailGraphic_Create(u32 layout, HeapID heapId, BOOL with3D) {
    PokedexDetailGraphic *graphic =
        GFL_HeapAllocate(heapId, sizeof(PokedexDetailGraphic), FALSE, "zukan_detail_graphic.c", 475);

    MI_CpuFill8(graphic, 0, sizeof(PokedexDetailGraphic));
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
    graphic->g3dCreated = FALSE;
    PokedexDetailBG_Init(&graphic->bg, heapId);
    PokedexDetailClAct_Init(&graphic->clact, &sVRAMConfig, heapId);
    if (with3D) {
        PokedexDetailGraphic_Create3D(graphic, heapId);
    }
    graphic->vblankTask = GFL_VBlankTCBAdd(PokedexDetailGraphic_VBlank, graphic, 0);
    return graphic;
}

void PokedexDetailGraphic_Free(PokedexDetailGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    PokedexDetailGraphic_Free3D(graphic);
    PokedexDetailClAct_Free(&graphic->clact);
    PokedexDetailBG_Free(&graphic->bg);
    GFL_TextRndResetColorIndexLUT();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(PokedexDetailGraphic));
    GFL_HeapFree(graphic);
}

void PokedexDetailGraphic_Update(PokedexDetailGraphic *graphic) {
    PokedexDetailClAct_Update(&graphic->clact);
    PokedexDetailBG_Update(&graphic->bg);
}

void PokedexDetailGraphic_Begin3D(PokedexDetailGraphic *graphic) {
    if (graphic->g3dCreated) {
        PokedexDetailG3dSys_Begin(&graphic->g3d);
    }
}

void PokedexDetailGraphic_End3D(PokedexDetailGraphic *graphic) {
    if (graphic->g3dCreated) {
        PokedexDetailG3dSys_End(&graphic->g3d);
    }
}

ClActUnit *PokedexDetailGraphic_GetClActUnit(PokedexDetailGraphic *graphic) {
    return PokedexDetailClAct_GetUnit(&graphic->clact);
}

void PokedexDetailGraphic_Create3D(PokedexDetailGraphic *graphic, HeapID heapId) {
    if (!graphic->g3dCreated) {
        graphic->g3dCreated = TRUE;
        PokedexDetailG3dSys_Init(&graphic->g3d, heapId);
    }
}

void PokedexDetailGraphic_Free3D(PokedexDetailGraphic *graphic) {
    if (graphic->g3dCreated) {
        PokedexDetailG3dSys_Free(&graphic->g3d);
        graphic->g3dCreated = FALSE;
    }
}

static void PokedexDetailGraphic_VBlank(TCB *tcb, void *data) {
    PokedexDetailGraphic *graphic = data;

    PokedexDetailBG_VBlank(&graphic->bg);
    PokedexDetailClAct_VBlank(&graphic->clact);
}

static void PokedexDetailBG_Init(PokedexDetailBG *bg, HeapID heapId) {
    u32 i;

    MI_CpuFill8(bg, 0, sizeof(PokedexDetailBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sLCDConfig);
    for (i = 0; i < NELEMS(sBGSetups); i++) {
        GFL_BGSysCreateBG(sBGSetups[i].bg, &sBGSetups[i].setup, sBGSetups[i].mode);
        GFL_BGSysClearBG(sBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sBGSetups[i].bg, sBGSetups[i].enabled);
    }
}

static void PokedexDetailBG_Free(PokedexDetailBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sBGSetups); i++) {
        GFL_BGSysReleaseBG(sBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(PokedexDetailBG));
}

static void PokedexDetailBG_Update(PokedexDetailBG *bg) {
}

static void PokedexDetailBG_VBlank(PokedexDetailBG *bg) {
    GFL_BGSysUpdate();
}

static void PokedexDetailClAct_Init(PokedexDetailClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(PokedexDetailClAct));
    ClActSys_Create(&sClActSetup, vramConfig, heapId);
    clact->unit = ClActUnit_Create(96, 0, heapId);
    ClActUnit_UseSystemRenderer(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void PokedexDetailClAct_Free(PokedexDetailClAct *clact) {
    ClActUnit_Delete(clact->unit);
    ClActSys_Delete();
    MI_CpuFill8(clact, 0, sizeof(PokedexDetailClAct));
}

static void PokedexDetailClAct_Update(PokedexDetailClAct *clact) {
    ClActSys_Draw();
}

static void PokedexDetailClAct_VBlank(PokedexDetailClAct *clact) {
    ClActSys_VBlank();
}

static ClActUnit *PokedexDetailClAct_GetUnit(PokedexDetailClAct *clact) {
    return clact->unit;
}

static void PokedexDetailG3dSys_Init(PokedexDetailG3dSys *g3d, HeapID heapId) {
    GFL_G3DSysCreate(FALSE, 1, 0, 2, 0, heapId, PokedexDetailGraphic_G3DInit);
    // A 40 degree field of view
    g3d->camera = GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)),
                                      FX_CosIdx(DEG_TO_IDX(20)), FX32_CONST(4.0 / 3.0), 0, 0x107c20, FX32_CONST(1024),
                                      0, &sCameraPosition, &sCameraUpVector, &sCameraTarget, heapId);
}

static void PokedexDetailG3dSys_Free(PokedexDetailG3dSys *g3d) {
    GFL_G3DCameraFree(g3d->camera);
    GFL_G3DSysFree();
}

static void PokedexDetailG3dSys_Begin(PokedexDetailG3dSys *g3d) {
    GFL_G3DSysReset();
    GFL_G3DCameraFlush(g3d->camera);
    GFL_G3DSysMtxViewFlush();
}

static void PokedexDetailG3dSys_End(PokedexDetailG3dSys *g3d) {
    GFL_G3DSysReqSwapBuffers();
}
