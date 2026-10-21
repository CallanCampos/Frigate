#include "types.h"
#include "app/unova_link.h"
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

// Unova Link's BGs, cell actors and 3D system, which it turns on for the key animations

typedef struct {
    u32 unk0;
} KeySystemGraphicBG;

typedef struct {
    ClActUnit *unit;
    ClActRenderer *renderer;
} KeySystemGraphicClAct;

typedef struct {
    G3DCamera *camera;
    BOOL active;
} KeySystemG3dSys;

struct KeySystemGraphic {
    KeySystemGraphicBG bg;
    KeySystemGraphicClAct clact;
    KeySystemG3dSys g3d;
    TCB *vblankTask;
    u16 heapId;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} KeySystemBGSetup;

static void KeySystemGraphic_G3DInit(void);
static void KeySystemGraphic_VBlank(TCB *tcb, void *data);
static void KeySystemGraphicBG_Init(KeySystemGraphicBG *bg, HeapID heapId);
static void KeySystemGraphicBG_Free(KeySystemGraphicBG *bg);
static void KeySystemGraphicBG_Update(KeySystemGraphicBG *bg);
static void KeySystemGraphicBG_VBlank(KeySystemGraphicBG *bg);
static void KeySystemGraphicClAct_Init(KeySystemGraphicClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void KeySystemGraphicClAct_Free(KeySystemGraphicClAct *clact);
static void KeySystemGraphicClAct_Update(KeySystemGraphicClAct *clact);
static void KeySystemGraphicClAct_VBlank(KeySystemGraphicClAct *clact);
static ClActUnit *KeySystemGraphicClAct_GetUnit(KeySystemGraphicClAct *clact);
static void KeySystemG3dSys_Init(KeySystemG3dSys *g3d, HeapID heapId);
static void KeySystemG3dSys_Free(KeySystemG3dSys *g3d);
static void KeySystemG3dSys_Begin(KeySystemG3dSys *g3d);
static void KeySystemG3dSys_End(KeySystemG3dSys *g3d);

static const VecFx32 sCameraTarget = { 0, 0, 0 };
static const VecFx32 sCameraUpVector = { 0, FX32_ONE, 0 };
static const VecFx32 sCameraPosition = { 0, 0, FX32_CONST(70) };

static const BGSysLCDConfig sLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };
static const ClActSysSetup sClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

static const ClActSurfaceSetup sSurfaceSetups[] = {
    { 0, 0, 256, 192, 0, 0 },
    { 0, 0, 256, 192, 1, 0 },
};

static const Light sLights[] = {
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
};

static const BGSysVRAMConfig sVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,        GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_D,   GX_VRAM_TEXPLTT_0_F,     GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static const KeySystemBGSetup sBGSetups[] = {
    { 0,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x0c000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 2,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x14000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
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
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x0c000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 7,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x14000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

// The 3D system calls this once it is set up
static void KeySystemGraphic_G3DInit(void) {
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

KeySystemGraphic *KeySystemGraphic_Create(u32 layout, HeapID heapId) {
    KeySystemGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(KeySystemGraphic), FALSE, "key_system_graphic.c", 468);

    MI_CpuFill8(graphic, 0, sizeof(KeySystemGraphic));
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
    KeySystemGraphicBG_Init(&graphic->bg, heapId);
    KeySystemGraphicClAct_Init(&graphic->clact, &sVRAMConfig, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(KeySystemGraphic_VBlank, graphic, 0);
    return graphic;
}

void KeySystemGraphic_Free(KeySystemGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    KeySystemGraphicClAct_Free(&graphic->clact);
    KeySystemGraphicBG_Free(&graphic->bg);
    GFL_TextRndResetColorIndexLUT();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(KeySystemGraphic));
    GFL_HeapFree(graphic);
}

void KeySystemGraphic_Update(KeySystemGraphic *graphic) {
    KeySystemGraphicClAct_Update(&graphic->clact);
    KeySystemGraphicBG_Update(&graphic->bg);
}

void KeySystemGraphic_Begin3D(KeySystemGraphic *graphic) {
    KeySystemG3dSys_Begin(&graphic->g3d);
}

void KeySystemGraphic_End3D(KeySystemGraphic *graphic) {
    KeySystemG3dSys_End(&graphic->g3d);
}

ClActUnit *KeySystemGraphic_GetClActUnit(KeySystemGraphic *graphic) {
    return KeySystemGraphicClAct_GetUnit(&graphic->clact);
}

void KeySystemGraphic_Set3D(KeySystemGraphic *graphic, u32 mode) {
    BGSysLCDConfig lcdConfig;

    switch (mode) {
    case KEY_SYSTEM_GRAPHIC_3D_OFF:
        KeySystemG3dSys_Free(&graphic->g3d);
        GFL_BGSysSetLCDConfig(&sLCDConfig);
        break;
    case KEY_SYSTEM_GRAPHIC_3D_ON:
        lcdConfig = sLCDConfig;
        lcdConfig.bg0Is3D = GX_BG0_AS_3D;
        GFL_BGSysSetLCDConfig(&lcdConfig);
        KeySystemG3dSys_Init(&graphic->g3d, graphic->heapId);
        break;
    }
}

static void KeySystemGraphic_VBlank(TCB *tcb, void *data) {
    KeySystemGraphic *graphic = data;

    KeySystemGraphicBG_VBlank(&graphic->bg);
    KeySystemGraphicClAct_VBlank(&graphic->clact);
}

static void KeySystemGraphicBG_Init(KeySystemGraphicBG *bg, HeapID heapId) {
    u32 i;

    MI_CpuFill8(bg, 0, sizeof(KeySystemGraphicBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sLCDConfig);
    for (i = 0; i < NELEMS(sBGSetups); i++) {
        GFL_BGSysCreateBG(sBGSetups[i].bg, &sBGSetups[i].setup, sBGSetups[i].mode);
        GFL_BGSysClearBG(sBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sBGSetups[i].bg, sBGSetups[i].enabled);
    }
}

static void KeySystemGraphicBG_Free(KeySystemGraphicBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sBGSetups); i++) {
        GFL_BGSysReleaseBG(sBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(KeySystemGraphicBG));
}

static void KeySystemGraphicBG_Update(KeySystemGraphicBG *bg) {
}

static void KeySystemGraphicBG_VBlank(KeySystemGraphicBG *bg) {
    GFL_BGSysUpdate();
}

static void KeySystemGraphicClAct_Init(KeySystemGraphicClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(KeySystemGraphicClAct));
    ClActSys_Create(&sClActSetup, vramConfig, heapId);
    clact->unit = ClActUnit_Create(32, 0, heapId);
    clact->renderer = ClActRenderer_Create(sSurfaceSetups, NELEMS(sSurfaceSetups), heapId);
    ClActUnit_SetRenderer(clact->unit, clact->renderer);
    ClActRenderer_SetCulling(clact->renderer, TRUE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void KeySystemGraphicClAct_Free(KeySystemGraphicClAct *clact) {
    ClActRenderer_Delete(clact->renderer);
    ClActUnit_Delete(clact->unit);
    ClActSys_Delete();
    MI_CpuFill8(clact, 0, sizeof(KeySystemGraphicClAct));
}

static void KeySystemGraphicClAct_Update(KeySystemGraphicClAct *clact) {
    ClActSys_Draw();
}

static void KeySystemGraphicClAct_VBlank(KeySystemGraphicClAct *clact) {
    ClActSys_VBlank();
}

static ClActUnit *KeySystemGraphicClAct_GetUnit(KeySystemGraphicClAct *clact) {
    return clact->unit;
}

static void KeySystemG3dSys_Init(KeySystemG3dSys *g3d, HeapID heapId) {
    MI_CpuFill8(g3d, 0, sizeof(KeySystemG3dSys));
    GFL_G3DSysCreate(FALSE, 1, 0, 1, 0, heapId, KeySystemGraphic_G3DInit);
    g3d->camera =
        GFL_G3DCameraCreate(G3DCAM_PROJECTION_ORTHO, FX32_CONST(24), -FX32_CONST(24), -FX32_CONST(32), FX32_CONST(32),
                            FX32_ONE, FX32_CONST(1024), 0, &sCameraPosition, &sCameraUpVector, &sCameraTarget, heapId);
    Particle_Init(heapId);
    g3d->active = TRUE;
}

static void KeySystemG3dSys_Free(KeySystemG3dSys *g3d) {
    Particle_Free();
    GFL_G3DCameraFree(g3d->camera);
    GFL_G3DSysFree();
    MI_CpuFill8(g3d, 0, sizeof(KeySystemG3dSys));
}

static void KeySystemG3dSys_Begin(KeySystemG3dSys *g3d) {
    if (g3d->active) {
        GFL_G3DSysReset();
        GFL_G3DCameraFlush(g3d->camera);
        GFL_G3DSysMtxViewFlush();
        Particle_DrawAndUpdateAll();
    }
}

static void KeySystemG3dSys_End(KeySystemG3dSys *g3d) {
    if (g3d->active) {
        GFL_G3DSysReqSwapBuffers();
    }
}
