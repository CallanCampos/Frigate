#include "types.h"
#include "app/mystery/mystery_graphic.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/particle.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "system/gf_font.h"

// Mystery Gift's BGs, cell actors and 3D system, which it turns on for the effect of a received gift. Our names; swan
// has none for this overlay

typedef struct {
    u32 unk0;
} MysteryGraphicBG;

typedef struct {
    ClActUnit *unit;
    ClActRenderer *renderer;
} MysteryGraphicClAct;

typedef struct {
    G3DCamera *camera;
    BOOL active;
} MysteryG3dSys;

struct MysteryGraphic {
    MysteryGraphicBG bg;
    MysteryGraphicClAct clact;
    MysteryG3dSys g3d;
    TCB *vblankTask;
    u16 heapId;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} MysteryBGSetup;

static void MysteryGraphic_G3DInit(void);
static void MysteryGraphic_VBlank(TCB *tcb, void *data);
static void MysteryGraphicBG_Init(MysteryGraphicBG *bg, HeapID heapId);
static void MysteryGraphicBG_Free(MysteryGraphicBG *bg);
static void MysteryGraphicBG_Update(MysteryGraphicBG *bg);
static void MysteryGraphicBG_VBlank(MysteryGraphicBG *bg);
static void MysteryGraphicClAct_Init(MysteryGraphicClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void MysteryGraphicClAct_Free(MysteryGraphicClAct *clact);
static void MysteryGraphicClAct_Update(MysteryGraphicClAct *clact);
static void MysteryGraphicClAct_VBlank(MysteryGraphicClAct *clact);
static ClActUnit *MysteryGraphicClAct_GetUnit(MysteryGraphicClAct *clact);
static void MysteryG3dSys_Init(MysteryG3dSys *g3d, HeapID heapId);
static void MysteryG3dSys_Free(MysteryG3dSys *g3d);
static void MysteryG3dSys_Begin(MysteryG3dSys *g3d);
static void MysteryG3dSys_End(MysteryG3dSys *g3d);

static const VecFx32 sCameraUpVector = { 0, FX32_ONE, 0 };
static const VecFx32 sCameraTarget = { 0, 0, 0 };
static const VecFx32 sCameraPosition = { 0, 0, FX32_CONST(70) };

static const BGSysLCDConfig sLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };
static const ClActSysSetup sClActSetup = { 0, 0, 0, 192, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

static const Light sLights[] = {
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
};

static const ClActSurfaceSetup sSurfaceSetups[] = {
    { 0, 0, 256, 384, 0, 0 },
    { 0, 512, 256, 192, 1, 0 },
};

static const BGSysVRAMConfig sVRAMConfig = {
    GX_VRAM_BG_128_A, GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_64_E, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_B,  GX_VRAM_TEXPLTT_01_FG,   GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static const MysteryBGSetup sBGSetups[] = {
    { 0,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x0c000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 2,
      { 0, 0, 0x1000, 0, BGRES_512x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 3,
      { 0, 0, 0x1000, 0, BGRES_512x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x14000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 6,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x0c000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 7,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

// The 3D system calls this once it is set up
static void MysteryGraphic_G3DInit(void) {
    u32 i;

    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, FALSE);
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

MysteryGraphic *MysteryGraphic_Create(u32 layout, HeapID heapId) {
    MysteryGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(MysteryGraphic), FALSE, "mystery_graphic.c", 448);

    MI_CpuFill8(graphic, 0, sizeof(MysteryGraphic));
    graphic->heapId = heapId;
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
    MysteryGraphicBG_Init(&graphic->bg, heapId);
    MysteryGraphicClAct_Init(&graphic->clact, &sVRAMConfig, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(MysteryGraphic_VBlank, graphic, 0);
    return graphic;
}

void MysteryGraphic_Delete(MysteryGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    MysteryGraphicClAct_Free(&graphic->clact);
    MysteryGraphicBG_Free(&graphic->bg);
    GFL_TextRndResetColorIndexLUT();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(MysteryGraphic));
    GFL_HeapFree(graphic);
}

void MysteryGraphic_Update(MysteryGraphic *graphic) {
    MysteryGraphicClAct_Update(&graphic->clact);
    MysteryGraphicBG_Update(&graphic->bg);
}

void MysteryGraphic_BeginFrame3D(MysteryGraphic *graphic) {
    MysteryG3dSys_Begin(&graphic->g3d);
}

void MysteryGraphic_EndFrame3D(MysteryGraphic *graphic) {
    MysteryG3dSys_End(&graphic->g3d);
}

ClActUnit *MysteryGraphic_GetClactUnit(MysteryGraphic *graphic) {
    return MysteryGraphicClAct_GetUnit(&graphic->clact);
}

void MysteryGraphic_Start3D(MysteryGraphic *graphic) {
    BGSysLCDConfig lcdConfig;

    GFL_BGSysSetBGEnabled(0, FALSE);
    lcdConfig = sLCDConfig;
    lcdConfig.bg0Is3D = GX_BG0_AS_3D;
    GFL_BGSysSetLCDConfig(&lcdConfig);
    MysteryG3dSys_Init(&graphic->g3d, graphic->heapId);
    Particle_Init(graphic->heapId);
}

void MysteryGraphic_End3D(MysteryGraphic *graphic) {
    GFL_BGSysSetBGEnabled(0, FALSE);
    Particle_Free();
    MysteryG3dSys_Free(&graphic->g3d);
    GFL_BGSysSetLCDConfig(&sLCDConfig);
}

static void MysteryGraphic_VBlank(TCB *tcb, void *data) {
    MysteryGraphic *graphic = data;

    MysteryGraphicBG_VBlank(&graphic->bg);
    MysteryGraphicClAct_VBlank(&graphic->clact);
}

static void MysteryGraphicBG_Init(MysteryGraphicBG *bg, HeapID heapId) {
    u32 i;

    MI_CpuFill8(bg, 0, sizeof(MysteryGraphicBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sLCDConfig);
    for (i = 0; i < NELEMS(sBGSetups); i++) {
        GFL_BGSysCreateBG(sBGSetups[i].bg, &sBGSetups[i].setup, sBGSetups[i].mode);
        GFL_BGSysClearBG(sBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sBGSetups[i].bg, sBGSetups[i].enabled);
    }
}

static void MysteryGraphicBG_Free(MysteryGraphicBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sBGSetups); i++) {
        GFL_BGSysReleaseBG(sBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(MysteryGraphicBG));
}

static void MysteryGraphicBG_Update(MysteryGraphicBG *bg) {
}

static void MysteryGraphicBG_VBlank(MysteryGraphicBG *bg) {
    GFL_BGSysUpdate();
}

static void MysteryGraphicClAct_Init(MysteryGraphicClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(MysteryGraphicClAct));
    ClActSys_Create(&sClActSetup, vramConfig, heapId);
    clact->unit = ClActUnit_Create(128, 0, heapId);
    clact->renderer = ClActRenderer_Create(sSurfaceSetups, NELEMS(sSurfaceSetups), heapId);
    ClActUnit_SetRenderer(clact->unit, clact->renderer);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void MysteryGraphicClAct_Free(MysteryGraphicClAct *clact) {
    ClActRenderer_Delete(clact->renderer);
    ClActUnit_Delete(clact->unit);
    ClActSys_Delete();
    MI_CpuFill8(clact, 0, sizeof(MysteryGraphicClAct));
}

static void MysteryGraphicClAct_Update(MysteryGraphicClAct *clact) {
    ClActSys_Draw();
}

static void MysteryGraphicClAct_VBlank(MysteryGraphicClAct *clact) {
    ClActSys_VBlank();
}

static ClActUnit *MysteryGraphicClAct_GetUnit(MysteryGraphicClAct *clact) {
    return clact->unit;
}

static void MysteryG3dSys_Init(MysteryG3dSys *g3d, HeapID heapId) {
    GFL_G3DSysCreate(FALSE, 1, 0, 1, 0, heapId, MysteryGraphic_G3DInit);
    g3d->camera =
        GFL_G3DCameraCreate(G3DCAM_PROJECTION_ORTHO, FX32_CONST(24), -FX32_CONST(24), -FX32_CONST(32), FX32_CONST(32),
                            FX32_ONE, FX32_CONST(1024), 0, &sCameraPosition, &sCameraUpVector, &sCameraTarget, heapId);
    g3d->active = TRUE;
}

static void MysteryG3dSys_Free(MysteryG3dSys *g3d) {
    g3d->active = FALSE;
    GFL_G3DCameraFree(g3d->camera);
    GFL_G3DSysFree();
}

static void MysteryG3dSys_Begin(MysteryG3dSys *g3d) {
    if (g3d->active) {
        GFL_G3DSysReset();
        GFL_G3DCameraFlush(g3d->camera);
        GFL_G3DSysMtxViewFlush();
        Particle_DrawAndUpdateAll();
    }
}

static void MysteryG3dSys_End(MysteryG3dSys *g3d) {
    if (g3d->active) {
        GFL_G3DSysReqSwapBuffers();
    }
}
