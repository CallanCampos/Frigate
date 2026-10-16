#include "types.h"
#include "app/manual_graphic.h"
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

// The BGs and cell actors of the manual, named after the ROM's "manual_graphic.c". Only the sub engine's BGs are
// created here: the main engine shows a picture on a bitmap BG, which manual_common.c sets up

#define BG_COUNT 4

typedef struct {
    u32 unk0;
} ManualBG;

typedef struct {
    ClActUnit *unit;
} ManualClAct;

struct ManualGraphic {
    ManualBG bg;
    ManualClAct clact;
    u32 unk8;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} ManualBGSetup;

static void ManualGraphic_VBlank(TCB *tcb, void *data);
static void ManualBG_Init(ManualBG *bg, HeapID heapId);
static void ManualBG_Free(ManualBG *bg);
static void ManualBG_Update(ManualBG *bg);
static void ManualBG_VBlank(ManualBG *bg);
static void ManualClAct_Init(ManualClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void ManualClAct_Free(ManualClAct *clact);
static void ManualClAct_Update(ManualClAct *clact);
static void ManualClAct_VBlank(ManualClAct *clact);
static ClActUnit *ManualClAct_GetUnit(ManualClAct *clact);

static const BGSysLCDConfig sManualLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_5, GX_BGMODE_0, GX_BG0_AS_2D };
static const ClActSysSetup sManualClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

static const BGSysVRAMConfig sManualVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,        GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_D,   GX_VRAM_TEXPLTT_0_F,     GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const ManualBGSetup sManualBGSetups[BG_COUNT] = {
    { 4,
      { 0, 0, 0x1000, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x4000), 0x4000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 5,
      { 0, 0, 0x1000, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x8000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 6,
      { 0, 0, 0x1000, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 7,
      { 0, 0, 0x1000, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x18000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE }
};

ManualGraphic *ManualGraphic_Create(u32 layout, HeapID heapId) {
    ManualGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(ManualGraphic), FALSE, "manual_graphic.c", 454);

    MI_CpuFill8(graphic, 0, sizeof(ManualGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sManualVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    func_020232d0();
    ManualBG_Init(&graphic->bg, heapId);
    ManualClAct_Init(&graphic->clact, &sManualVRAMConfig, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(ManualGraphic_VBlank, graphic, 0);
    return graphic;
}

void ManualGraphic_Free(ManualGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    ManualClAct_Free(&graphic->clact);
    ManualBG_Free(&graphic->bg);
    func_020232d8();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(ManualGraphic));
    GFL_HeapFree(graphic);
}

void ManualGraphic_Update(ManualGraphic *graphic) {
    ManualClAct_Update(&graphic->clact);
    ManualBG_Update(&graphic->bg);
}

ClActUnit *ManualGraphic_GetClActUnit(ManualGraphic *graphic) {
    return ManualClAct_GetUnit(&graphic->clact);
}

static void ManualGraphic_VBlank(TCB *tcb, void *data) {
    ManualGraphic *graphic = data;

    ManualBG_VBlank(&graphic->bg);
    ManualClAct_VBlank(&graphic->clact);
}

static void ManualBG_Init(ManualBG *bg, HeapID heapId) {
    u32 i;

    MI_CpuFill8(bg, 0, sizeof(ManualBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sManualLCDConfig);
    for (i = 0; i < NELEMS(sManualBGSetups); i++) {
        GFL_BGSysCreateBG(sManualBGSetups[i].bg, &sManualBGSetups[i].setup, sManualBGSetups[i].mode);
        GFL_BGSysClearBG(sManualBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sManualBGSetups[i].bg, sManualBGSetups[i].enabled);
    }
}

static void ManualBG_Free(ManualBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sManualBGSetups); i++) {
        GFL_BGSysReleaseBG(sManualBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(ManualBG));
}

static void ManualBG_Update(ManualBG *bg) {
}

static void ManualBG_VBlank(ManualBG *bg) {
    GFL_BGSysUpdate();
}

static void ManualClAct_Init(ManualClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(ManualClAct));
    ClActSys_Create(&sManualClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void ManualClAct_Free(ManualClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    MI_CpuFill8(clact, 0, sizeof(ManualClAct));
}

static void ManualClAct_Update(ManualClAct *clact) {
    func_0204b794();
}

static void ManualClAct_VBlank(ManualClAct *clact) {
    func_0204b7c8();
}

static ClActUnit *ManualClAct_GetUnit(ManualClAct *clact) {
    return clact->unit;
}
