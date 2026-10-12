#include "types.h"
#include "app/btl_rec_sel_graphic.h"
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

// The BGs, cell actors and 3D system of the "save this battle video?" screen, named after the ROM's
// "btl_rec_sel_graphic.c". The file starts at the 3D system's set-up callback, as its siblings of the same template do
// (psel_graphic.c, pkwd_selectscreen_graphic.c); the three functions before it use btl_rec_sel.c's work

#define BG_COUNT 3

typedef struct {
    u32 unk0;
} BtlRecSelBG;

typedef struct {
    ClActUnit *unit;
} BtlRecSelClAct;

typedef struct {
    G3DCamera *camera;
} BtlRecSelG3dSys;

struct BtlRecSelGraphic {
    BtlRecSelBG bg;
    BtlRecSelClAct clact;
    BtlRecSelG3dSys g3d;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} BtlRecSelBGSetup;

static void BtlRecSelGraphic_VBlank(TCB *tcb, void *data);
static void BtlRecSelBG_Init(BtlRecSelBG *bg, HeapID heapId);
static void BtlRecSelBG_Free(BtlRecSelBG *bg);
static void BtlRecSelBG_Update(BtlRecSelBG *bg);
static void BtlRecSelBG_VBlank(BtlRecSelBG *bg);
static void BtlRecSelClAct_Init(BtlRecSelClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void BtlRecSelClAct_Free(BtlRecSelClAct *clact);
static void BtlRecSelClAct_Update(BtlRecSelClAct *clact);
static void BtlRecSelClAct_VBlank(BtlRecSelClAct *clact);
static ClActUnit *BtlRecSelClAct_GetUnit(BtlRecSelClAct *clact);
static void BtlRecSelG3dSys_Init(BtlRecSelG3dSys *g3d, HeapID heapId);
static void BtlRecSelG3dSys_Free(BtlRecSelG3dSys *g3d);
static void BtlRecSelG3dSys_Begin(BtlRecSelG3dSys *g3d);
static void BtlRecSelG3dSys_End(BtlRecSelG3dSys *g3d);

static const VecFx32 sBtlRecSelCameraTarget = { 0, FX32_ONE, 0 };
static const VecFx32 sBtlRecSelCameraPosition = { FX32_CONST(6.7), FX32_CONST(6.7), FX32_CONST(17.3) };
static const VecFx32 sBtlRecSelCameraUpVector = { 0, FX32_CONST(2.6), 0 };

static const BGSysLCDConfig sBtlRecSelLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };
static const ClActSysSetup sBtlRecSelClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

static const Light sBtlRecSelLights[] = {
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
};

static const BGSysVRAMConfig sBtlRecSelVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,        GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_D,   GX_VRAM_TEXPLTT_0_F,     GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

// BG1 to BG3 of the main screen
static const BtlRecSelBGSetup sBtlRecSelBGSetups[BG_COUNT] = {
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x8000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 2,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 3,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x14000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

// The 3D system calls this once it is set up
static void BtlRecSelGraphic_G3DInit(void) {
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
    for (i = 0; i < NELEMS(sBtlRecSelLights); i++) {
        GFL_G3DSysLightSet(i, &sBtlRecSelLights[i]);
    }
    GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_AUTO, GX_BUFFERMODE_Z);
}

BtlRecSelGraphic *BtlRecSelGraphic_Create(u32 layout, HeapID heapId) {
    BtlRecSelGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(BtlRecSelGraphic), FALSE, "btl_rec_sel_graphic.c", 458);

    sys_memset(graphic, 0, sizeof(BtlRecSelGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sBtlRecSelVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    func_020232d0();
    BtlRecSelBG_Init(&graphic->bg, heapId);
    BtlRecSelClAct_Init(&graphic->clact, &sBtlRecSelVRAMConfig, heapId);
    BtlRecSelG3dSys_Init(&graphic->g3d, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(BtlRecSelGraphic_VBlank, graphic, 0);
    return graphic;
}

void BtlRecSelGraphic_Free(BtlRecSelGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    BtlRecSelG3dSys_Free(&graphic->g3d);
    BtlRecSelClAct_Free(&graphic->clact);
    BtlRecSelBG_Free(&graphic->bg);
    func_020232d8();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    sys_memset(graphic, 0, sizeof(BtlRecSelGraphic));
    GFL_HeapFree(graphic);
}

void BtlRecSelGraphic_Update(BtlRecSelGraphic *graphic) {
    BtlRecSelClAct_Update(&graphic->clact);
    BtlRecSelBG_Update(&graphic->bg);
}

void BtlRecSelGraphic_Begin3D(BtlRecSelGraphic *graphic) {
    BtlRecSelG3dSys_Begin(&graphic->g3d);
}

void BtlRecSelGraphic_End3D(BtlRecSelGraphic *graphic) {
    BtlRecSelG3dSys_End(&graphic->g3d);
}

ClActUnit *BtlRecSelGraphic_GetClActUnit(BtlRecSelGraphic *graphic) {
    return BtlRecSelClAct_GetUnit(&graphic->clact);
}

static void BtlRecSelGraphic_VBlank(TCB *tcb, void *data) {
    BtlRecSelGraphic *graphic = data;

    BtlRecSelBG_VBlank(&graphic->bg);
    BtlRecSelClAct_VBlank(&graphic->clact);
}

static void BtlRecSelBG_Init(BtlRecSelBG *bg, HeapID heapId) {
    u32 i;

    sys_memset(bg, 0, sizeof(BtlRecSelBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sBtlRecSelLCDConfig);
    for (i = 0; i < NELEMS(sBtlRecSelBGSetups); i++) {
        GFL_BGSysCreateBG(sBtlRecSelBGSetups[i].bg, &sBtlRecSelBGSetups[i].setup, sBtlRecSelBGSetups[i].mode);
        GFL_BGSysClearBG(sBtlRecSelBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sBtlRecSelBGSetups[i].bg, sBtlRecSelBGSetups[i].enabled);
    }
}

static void BtlRecSelBG_Free(BtlRecSelBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sBtlRecSelBGSetups); i++) {
        GFL_BGSysReleaseBG(sBtlRecSelBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    sys_memset(bg, 0, sizeof(BtlRecSelBG));
}

static void BtlRecSelBG_Update(BtlRecSelBG *bg) {
}

static void BtlRecSelBG_VBlank(BtlRecSelBG *bg) {
    GFL_BGSysUpdate();
}

static void BtlRecSelClAct_Init(BtlRecSelClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    sys_memset(clact, 0, sizeof(BtlRecSelClAct));
    ClActSys_Create(&sBtlRecSelClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void BtlRecSelClAct_Free(BtlRecSelClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    sys_memset(clact, 0, sizeof(BtlRecSelClAct));
}

static void BtlRecSelClAct_Update(BtlRecSelClAct *clact) {
    func_0204b794();
}

static void BtlRecSelClAct_VBlank(BtlRecSelClAct *clact) {
    func_0204b7c8();
}

static ClActUnit *BtlRecSelClAct_GetUnit(BtlRecSelClAct *clact) {
    return clact->unit;
}

static void BtlRecSelG3dSys_Init(BtlRecSelG3dSys *g3d, HeapID heapId) {
    GFL_G3DSysCreate(FALSE, 1, 0, 1, 0, heapId, BtlRecSelGraphic_G3DInit);
    g3d->camera =
        GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)), FX_CosIdx(DEG_TO_IDX(20)),
                            FX32_CONST(4.0 / 3.0), 0, FX32_ONE, FX32_CONST(1024), 0, &sBtlRecSelCameraPosition,
                            &sBtlRecSelCameraUpVector, &sBtlRecSelCameraTarget, heapId);
}

static void BtlRecSelG3dSys_Free(BtlRecSelG3dSys *g3d) {
    GFL_G3DCameraFree(g3d->camera);
    GFL_G3DSysFree();
}

static void BtlRecSelG3dSys_Begin(BtlRecSelG3dSys *g3d) {
    GFL_G3DSysReset();
    GFL_G3DCameraFlush(g3d->camera);
    GFL_G3DSysMtxViewFlush();
}

static void BtlRecSelG3dSys_End(BtlRecSelG3dSys *g3d) {
    GFL_G3DSysReqSwapBuffers();
}
