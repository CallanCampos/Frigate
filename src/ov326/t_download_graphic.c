#include "types.h"
#include "app/t_download/t_download_graphic.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "system/gf_font.h"

// The BGs and cell actors of the Pokémon World Tournament's downloaded tournaments, named after the ROM's
// "t_download_graphic.c". The names are ours

typedef struct {
    u32 unused;
} TDownloadBG;

typedef struct {
    ClActUnit *unit;
} TDownloadClAct;

struct TDownloadGraphic {
    TDownloadBG bg;
    TDownloadClAct clact;
    u32 unused;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} TDownloadBGSetup;

static void TDownloadGraphic_VBlank(TCB *tcb, void *data);
static void TDownloadGraphic_BGInit(TDownloadBG *bg, HeapID heapId);
static void TDownloadGraphic_BGExit(TDownloadBG *bg);
static void TDownloadGraphic_BGMain(TDownloadBG *bg);
static void TDownloadGraphic_BGVBlank(TDownloadBG *bg);
static void TDownloadGraphic_ClActInit(TDownloadClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void TDownloadGraphic_ClActExit(TDownloadClAct *clact);
static void TDownloadGraphic_ClActMain(TDownloadClAct *clact);
static void TDownloadGraphic_ClActVBlank(TDownloadClAct *clact);
static ClActUnit *TDownloadGraphic_ClActGetUnit(TDownloadClAct *clact);

static const BGSysLCDConfig sTDownloadLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

static const ClActSysSetup sTDownloadClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

static const BGSysVRAMConfig sTDownloadVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const TDownloadBGSetup sTDownloadBGSetups[] = {
    { 0,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x18000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 2,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 3,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1800), GX_BG_CHARBASE(0x18000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 5,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x08000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 6,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 7,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1800), GX_BG_CHARBASE(0x18000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

TDownloadGraphic *TDownloadGraphic_Create(u32 layout, HeapID heapId) {
    TDownloadGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(TDownloadGraphic), FALSE, "t_download_graphic.c", 450);

    MI_CpuFill8(graphic, 0, sizeof(TDownloadGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sTDownloadVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GFL_TextRndInitColorIndexLUT();
    TDownloadGraphic_BGInit(&graphic->bg, heapId);
    TDownloadGraphic_ClActInit(&graphic->clact, &sTDownloadVRAMConfig, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(TDownloadGraphic_VBlank, graphic, 0);
    return graphic;
}

void TDownloadGraphic_Delete(TDownloadGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    TDownloadGraphic_ClActExit(&graphic->clact);
    TDownloadGraphic_BGExit(&graphic->bg);
    GFL_TextRndResetColorIndexLUT();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(TDownloadGraphic));
    GFL_HeapFree(graphic);
}

void TDownloadGraphic_Main(TDownloadGraphic *graphic) {
    TDownloadGraphic_ClActMain(&graphic->clact);
    TDownloadGraphic_BGMain(&graphic->bg);
}

void TDownloadGraphic_Begin3D(TDownloadGraphic *graphic) {
}

void TDownloadGraphic_End3D(TDownloadGraphic *graphic) {
}

ClActUnit *TDownloadGraphic_GetClActUnit(TDownloadGraphic *graphic) {
    return TDownloadGraphic_ClActGetUnit(&graphic->clact);
}

static void TDownloadGraphic_VBlank(TCB *tcb, void *data) {
    TDownloadGraphic *graphic = data;

    TDownloadGraphic_BGVBlank(&graphic->bg);
    TDownloadGraphic_ClActVBlank(&graphic->clact);
}

static void TDownloadGraphic_BGInit(TDownloadBG *bg, HeapID heapId) {
    u32 i;

    MI_CpuFill8(bg, 0, sizeof(TDownloadBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sTDownloadLCDConfig);
    for (i = 0; i < NELEMS(sTDownloadBGSetups); i++) {
        GFL_BGSysCreateBG(sTDownloadBGSetups[i].bg, &sTDownloadBGSetups[i].setup, sTDownloadBGSetups[i].mode);
        GFL_BGSysClearBG(sTDownloadBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sTDownloadBGSetups[i].bg, sTDownloadBGSetups[i].enabled);
    }
}

static void TDownloadGraphic_BGExit(TDownloadBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sTDownloadBGSetups); i++) {
        GFL_BGSysReleaseBG(sTDownloadBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(TDownloadBG));
}

static void TDownloadGraphic_BGMain(TDownloadBG *bg) {
}

static void TDownloadGraphic_BGVBlank(TDownloadBG *bg) {
    GFL_BGSysUpdate();
}

static void TDownloadGraphic_ClActInit(TDownloadClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(TDownloadClAct));
    ClActSys_Create(&sTDownloadClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void TDownloadGraphic_ClActExit(TDownloadClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    MI_CpuFill8(clact, 0, sizeof(TDownloadClAct));
}

static void TDownloadGraphic_ClActMain(TDownloadClAct *clact) {
    func_0204b794();
}

static void TDownloadGraphic_ClActVBlank(TDownloadClAct *clact) {
    func_0204b7c8();
}

static ClActUnit *TDownloadGraphic_ClActGetUnit(TDownloadClAct *clact) {
    return clact->unit;
}
