#include "types.h"
#include "app/wbt_download_net_graphic.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "system/gf_font.h"

// The tournament download's BGs and cell actor system. The name is the ROM's string, from GFL_HeapAllocate's call.
// Function names are ours

#define BG_COUNT 8

typedef struct {
    u32 unused;
} WbtDownloadBG;

typedef struct {
    ClActUnit *unit;
    ClActRenderer *renderer;
} WbtDownloadClAct;

struct WbtDownloadGraphic {
    WbtDownloadBG bg;
    WbtDownloadClAct clact;
    u32 unusedC;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} WbtDownloadBGSetup;

static void WbtDownloadGraphic_VBlank(TCB *tcb, void *data);
static void WbtDownloadBG_Init(WbtDownloadBG *bg, HeapID heapId);
static void WbtDownloadBG_Free(WbtDownloadBG *bg);
static void WbtDownloadBG_Update(WbtDownloadBG *bg);
static void WbtDownloadBG_VBlank(WbtDownloadBG *bg);
static void WbtDownloadClAct_Init(WbtDownloadClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void WbtDownloadClAct_Free(WbtDownloadClAct *clact);
static void WbtDownloadClAct_Update(WbtDownloadClAct *clact);
static void WbtDownloadClAct_VBlank(WbtDownloadClAct *clact);

static const BGSysLCDConfig sWbtDownloadLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };
static const ClActSysSetup sWbtDownloadClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };
static const ClActSurfaceSetup sWbtDownloadSurfaceSetups[2] = {
    { 0, 0, 256, 192, CLACT_SURFACE_MAIN, 0 },
    { 0, 0, 256, 192, CLACT_SURFACE_SUB, 0 },
};

static const BGSysVRAMConfig sWbtDownloadVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,        GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_D,   GX_VRAM_TEXPLTT_0_F,     GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static const WbtDownloadBGSetup sWbtDownloadBGSetups[BG_COUNT] = {
    { 0,
      { 0, 0, 0x1000, 0, BGRES_512x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x0c000), 0x8000,
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
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 5,
      { 0, 0, 0x1000, 0, BGRES_512x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x08000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 6,
      { 0, 0, 0x1000, 0, BGRES_512x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x0c000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 7,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x14000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

WbtDownloadGraphic *WbtDownloadGraphic_Create(u32 layout, HeapID heapId) {
    WbtDownloadGraphic *graphic =
        GFL_HeapAllocate(heapId, sizeof(WbtDownloadGraphic), FALSE, "wbt_download_net_graphic.c", 477);

    MI_CpuFill8(graphic, 0, sizeof(WbtDownloadGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sWbtDownloadVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    func_020232d0();
    WbtDownloadBG_Init(&graphic->bg, heapId);
    WbtDownloadClAct_Init(&graphic->clact, &sWbtDownloadVRAMConfig, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(WbtDownloadGraphic_VBlank, graphic, 0);
    return graphic;
}

void WbtDownloadGraphic_Free(WbtDownloadGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    WbtDownloadClAct_Free(&graphic->clact);
    WbtDownloadBG_Free(&graphic->bg);
    func_020232d8();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(WbtDownloadGraphic));
    GFL_HeapFree(graphic);
}

void WbtDownloadGraphic_Update(WbtDownloadGraphic *graphic) {
    WbtDownloadClAct_Update(&graphic->clact);
    WbtDownloadBG_Update(&graphic->bg);
}

static void WbtDownloadGraphic_VBlank(TCB *tcb, void *data) {
    WbtDownloadGraphic *graphic = data;

    WbtDownloadBG_VBlank(&graphic->bg);
    WbtDownloadClAct_VBlank(&graphic->clact);
}

static void WbtDownloadBG_Init(WbtDownloadBG *bg, HeapID heapId) {
    u32 i;

    MI_CpuFill8(bg, 0, sizeof(WbtDownloadBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sWbtDownloadLCDConfig);
    for (i = 0; i < NELEMS(sWbtDownloadBGSetups); i++) {
        GFL_BGSysCreateBG(sWbtDownloadBGSetups[i].bg, &sWbtDownloadBGSetups[i].setup, sWbtDownloadBGSetups[i].mode);
        GFL_BGSysClearBG(sWbtDownloadBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sWbtDownloadBGSetups[i].bg, sWbtDownloadBGSetups[i].enabled);
    }
}

static void WbtDownloadBG_Free(WbtDownloadBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sWbtDownloadBGSetups); i++) {
        GFL_BGSysReleaseBG(sWbtDownloadBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(WbtDownloadBG));
}

static void WbtDownloadBG_Update(WbtDownloadBG *bg) {
}

static void WbtDownloadBG_VBlank(WbtDownloadBG *bg) {
    GFL_BGSysUpdate();
}

static void WbtDownloadClAct_Init(WbtDownloadClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(WbtDownloadClAct));
    ClActSys_Create(&sWbtDownloadClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(32, 0, heapId);
    clact->renderer = func_0204be9c(sWbtDownloadSurfaceSetups, NELEMS(sWbtDownloadSurfaceSetups), heapId);
    func_0204c018(clact->unit, clact->renderer);
    func_0204bf14(clact->renderer, TRUE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void WbtDownloadClAct_Free(WbtDownloadClAct *clact) {
    func_0204becc(clact->renderer);
    func_0204bf98(clact->unit);
    func_0204b758();
    MI_CpuFill8(clact, 0, sizeof(WbtDownloadClAct));
}

static void WbtDownloadClAct_Update(WbtDownloadClAct *clact) {
    func_0204b794();
}

static void WbtDownloadClAct_VBlank(WbtDownloadClAct *clact) {
    func_0204b7c8();
}
