#include "types.h"
#include "app/comm_btl_demo_graphic.h"
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

// The BGs, cell actors and 3D system of the link battle's introduction and result screen, named after the ROM's
// "comm_btl_demo_graphic.c". The file starts at the 3D system's set-up callback, as its siblings of the same template
// do (btl_rec_sel_graphic.c, psel_graphic.c)

#define BG_COUNT 6

typedef struct {
    u32 unk0;
} CommBtlDemoBG;

typedef struct {
    ClActUnit *unit;
} CommBtlDemoClAct;

typedef struct {
    G3DCamera *camera;
} CommBtlDemoG3dSys;

struct CommBtlDemoGraphic {
    CommBtlDemoBG bg;
    CommBtlDemoClAct clact;
    CommBtlDemoG3dSys g3d;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} CommBtlDemoBGSetup;

static void CommBtlDemoGraphic_VBlank(TCB *tcb, void *data);
static void CommBtlDemoBG_Init(CommBtlDemoBG *bg, HeapID heapId);
static void CommBtlDemoBG_Free(CommBtlDemoBG *bg);
static void CommBtlDemoBG_Update(CommBtlDemoBG *bg);
static void CommBtlDemoBG_VBlank(CommBtlDemoBG *bg);
static void CommBtlDemoClAct_Init(CommBtlDemoClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void CommBtlDemoClAct_Free(CommBtlDemoClAct *clact);
static void CommBtlDemoClAct_Update(CommBtlDemoClAct *clact);
static void CommBtlDemoClAct_VBlank(CommBtlDemoClAct *clact);
static ClActUnit *CommBtlDemoClAct_GetUnit(CommBtlDemoClAct *clact);
static void CommBtlDemoG3dSys_Init(CommBtlDemoG3dSys *g3d, HeapID heapId);
static void CommBtlDemoG3dSys_Free(CommBtlDemoG3dSys *g3d);
static void CommBtlDemoG3dSys_Begin(CommBtlDemoG3dSys *g3d);
static void CommBtlDemoG3dSys_End(CommBtlDemoG3dSys *g3d);

static const VecFx32 sCommBtlDemoCameraTarget = { 0, 0, 0 };
static const VecFx32 sCommBtlDemoCameraPosition = { 0, 0, FX32_CONST(128) };

static const BGSysVRAMConfig sCommBtlDemoVRAMConfig = {
    GX_VRAM_BG_128_A, GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_64_E, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_01_BD, GX_VRAM_TEXPLTT_0_F,    GX_OBJVRAMMODE_CHAR_1D_64K, GX_OBJVRAMMODE_CHAR_1D_64K,
};

// BG1 to BG3 of the main screen and BG0 to BG2 of the sub screen
static const CommBtlDemoBGSetup sCommBtlDemoBGSetups[BG_COUNT] = {
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x8000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
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
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x4000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 5,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x8000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 6,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

static const BGSysLCDConfig sCommBtlDemoLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };
static const ClActSysSetup sCommBtlDemoClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

static const Light sCommBtlDemoLights[] = {
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
};

// The 3D system calls this once it is set up
static void CommBtlDemoGraphic_G3DInit(void) {
    u32 i;

    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, TRUE);
    G2_SetBG0Priority(1);
    G3X_SetShading(GX_SHADING_HIGHLIGHT);
    G3X_AntiAlias(FALSE);
    G3X_AlphaTest(FALSE, 0);
    G3X_AlphaBlend(TRUE);
    G3X_EdgeMarking(FALSE);
    gfxSetFog(FALSE, 0, 0, 0);
    gfxClearColor(GX_RGB(0, 0, 0), 0, 0x7fff, 63, FALSE);
    G3_ViewPort(0, 0, 255, 191);
    for (i = 0; i < NELEMS(sCommBtlDemoLights); i++) {
        GFL_G3DSysLightSet(i, &sCommBtlDemoLights[i]);
    }
    GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_MANUAL, GX_BUFFERMODE_Z);
}

CommBtlDemoGraphic *CommBtlDemoGraphic_Create(u32 layout, HeapID heapId) {
    CommBtlDemoGraphic *graphic =
        GFL_HeapAllocate(heapId, sizeof(CommBtlDemoGraphic), FALSE, "comm_btl_demo_graphic.c", 436);

    sys_memset(graphic, 0, sizeof(CommBtlDemoGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sCommBtlDemoVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    func_020232d0();
    CommBtlDemoBG_Init(&graphic->bg, heapId);
    CommBtlDemoClAct_Init(&graphic->clact, &sCommBtlDemoVRAMConfig, heapId);
    CommBtlDemoG3dSys_Init(&graphic->g3d, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(CommBtlDemoGraphic_VBlank, graphic, 0);
    return graphic;
}

void CommBtlDemoGraphic_Free(CommBtlDemoGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    CommBtlDemoG3dSys_Free(&graphic->g3d);
    CommBtlDemoClAct_Free(&graphic->clact);
    CommBtlDemoBG_Free(&graphic->bg);
    func_020232d8();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    sys_memset(graphic, 0, sizeof(CommBtlDemoGraphic));
    GFL_HeapFree(graphic);
}

void CommBtlDemoGraphic_Update(CommBtlDemoGraphic *graphic) {
    CommBtlDemoClAct_Update(&graphic->clact);
    CommBtlDemoBG_Update(&graphic->bg);
}

void CommBtlDemoGraphic_Begin3D(CommBtlDemoGraphic *graphic) {
    CommBtlDemoG3dSys_Begin(&graphic->g3d);
}

void CommBtlDemoGraphic_End3D(CommBtlDemoGraphic *graphic) {
    CommBtlDemoG3dSys_End(&graphic->g3d);
}

ClActUnit *CommBtlDemoGraphic_GetClActUnit(CommBtlDemoGraphic *graphic) {
    return CommBtlDemoClAct_GetUnit(&graphic->clact);
}

static void CommBtlDemoGraphic_VBlank(TCB *tcb, void *data) {
    CommBtlDemoGraphic *graphic = data;

    CommBtlDemoBG_VBlank(&graphic->bg);
    CommBtlDemoClAct_VBlank(&graphic->clact);
}

static void CommBtlDemoBG_Init(CommBtlDemoBG *bg, HeapID heapId) {
    u32 i;

    sys_memset(bg, 0, sizeof(CommBtlDemoBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sCommBtlDemoLCDConfig);
    for (i = 0; i < NELEMS(sCommBtlDemoBGSetups); i++) {
        GFL_BGSysCreateBG(sCommBtlDemoBGSetups[i].bg, &sCommBtlDemoBGSetups[i].setup, sCommBtlDemoBGSetups[i].mode);
        GFL_BGSysClearBG(sCommBtlDemoBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sCommBtlDemoBGSetups[i].bg, sCommBtlDemoBGSetups[i].enabled);
    }
}

static void CommBtlDemoBG_Free(CommBtlDemoBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sCommBtlDemoBGSetups); i++) {
        GFL_BGSysReleaseBG(sCommBtlDemoBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    sys_memset(bg, 0, sizeof(CommBtlDemoBG));
}

static void CommBtlDemoBG_Update(CommBtlDemoBG *bg) {
}

static void CommBtlDemoBG_VBlank(CommBtlDemoBG *bg) {
    GFL_BGSysUpdate();
}

static void CommBtlDemoClAct_Init(CommBtlDemoClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    sys_memset(clact, 0, sizeof(CommBtlDemoClAct));
    ClActSys_Create(&sCommBtlDemoClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void CommBtlDemoClAct_Free(CommBtlDemoClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    sys_memset(clact, 0, sizeof(CommBtlDemoClAct));
}

static void CommBtlDemoClAct_Update(CommBtlDemoClAct *clact) {
    func_0204b794();
}

static void CommBtlDemoClAct_VBlank(CommBtlDemoClAct *clact) {
    func_0204b7c8();
}

static ClActUnit *CommBtlDemoClAct_GetUnit(CommBtlDemoClAct *clact) {
    return clact->unit;
}

static void CommBtlDemoG3dSys_Init(CommBtlDemoG3dSys *g3d, HeapID heapId) {
    GFL_G3DSysCreate(FALSE, 1, 0, 1, 0, heapId, CommBtlDemoGraphic_G3DInit);
    {
        VecFx32 upVector = { 0, FX32_ONE, 0 };

        g3d->camera =
            GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)), FX_CosIdx(DEG_TO_IDX(20)),
                                FX32_CONST(4.0 / 3.0), 0, FX32_ONE, FX32_CONST(1024), 0, &sCommBtlDemoCameraPosition,
                                &upVector, &sCommBtlDemoCameraTarget, heapId);
    }
}

static void CommBtlDemoG3dSys_Free(CommBtlDemoG3dSys *g3d) {
    GFL_G3DCameraFree(g3d->camera);
    GFL_G3DSysFree();
}

static void CommBtlDemoG3dSys_Begin(CommBtlDemoG3dSys *g3d) {
    GFL_G3DSysReset();
    GFL_G3DCameraFlush(g3d->camera);
    GFL_G3DSysMtxViewFlush();
}

static void CommBtlDemoG3dSys_End(CommBtlDemoG3dSys *g3d) {
    GFL_G3DSysReqSwapBuffers();
}
