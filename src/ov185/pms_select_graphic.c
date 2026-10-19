#include "types.h"
#include "app/pms_select_graphic.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "system/gf_font.h"

// The BGs and cell actors of the phrase select, named after the ROM's "pms_select_graphic.c". The names are ours

typedef struct {
    u32 unused;
} PMSSelectBG;

typedef struct {
    ClActUnit *unit;
} PMSSelectClAct;

struct PMSSelectGraphic {
    PMSSelectBG bg;
    PMSSelectClAct clact;
    u32 unused;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} PMSSelectBGSetup;

static void PMSSelectGraphic_VBlank(TCB *tcb, void *data);
static void PMSSelectGraphic_BGInit(PMSSelectBG *bg, HeapID heapId);
static void PMSSelectGraphic_BGExit(PMSSelectBG *bg);
static void PMSSelectGraphic_BGMain(PMSSelectBG *bg);
static void PMSSelectGraphic_BGVBlank(PMSSelectBG *bg);
static void PMSSelectGraphic_ClActInit(PMSSelectClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void PMSSelectGraphic_ClActExit(PMSSelectClAct *clact);
static void PMSSelectGraphic_ClActMain(PMSSelectClAct *clact);
static void PMSSelectGraphic_ClActVBlank(PMSSelectClAct *clact);
static ClActUnit *PMSSelectGraphic_ClActGetUnit(PMSSelectClAct *clact);

static const BGSysLCDConfig sPMSSelectLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

static const ClActSysSetup sPMSSelectClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

static const BGSysVRAMConfig sPMSSelectVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,      GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_64K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const PMSSelectBGSetup sPMSSelectBGSetups[] = {
    { 0,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x4000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x08000), 0x8000,
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
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 5,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x0c000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

PMSSelectGraphic *PMSSelectGraphic_Create(u32 layout, HeapID heapId) {
    PMSSelectGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(PMSSelectGraphic), FALSE, "pms_select_graphic.c", 337);

    MI_CpuFill8(graphic, 0, sizeof(PMSSelectGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sPMSSelectVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GFL_TextRndInitColorIndexLUT();
    PMSSelectGraphic_BGInit(&graphic->bg, heapId);
    PMSSelectGraphic_ClActInit(&graphic->clact, &sPMSSelectVRAMConfig, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(PMSSelectGraphic_VBlank, graphic, 0);
    return graphic;
}

void PMSSelectGraphic_Delete(PMSSelectGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    PMSSelectGraphic_ClActExit(&graphic->clact);
    PMSSelectGraphic_BGExit(&graphic->bg);
    GFL_TextRndResetColorIndexLUT();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(PMSSelectGraphic));
    GFL_HeapFree(graphic);
}

void PMSSelectGraphic_Main(PMSSelectGraphic *graphic) {
    PMSSelectGraphic_ClActMain(&graphic->clact);
    PMSSelectGraphic_BGMain(&graphic->bg);
}

ClActUnit *PMSSelectGraphic_GetClActUnit(PMSSelectGraphic *graphic) {
    return PMSSelectGraphic_ClActGetUnit(&graphic->clact);
}

static void PMSSelectGraphic_VBlank(TCB *tcb, void *data) {
    PMSSelectGraphic *graphic = data;

    PMSSelectGraphic_BGVBlank(&graphic->bg);
    PMSSelectGraphic_ClActVBlank(&graphic->clact);
}

static void PMSSelectGraphic_BGInit(PMSSelectBG *bg, HeapID heapId) {
    u32 i;

    MI_CpuFill8(bg, 0, sizeof(PMSSelectBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sPMSSelectLCDConfig);
    for (i = 0; i < NELEMS(sPMSSelectBGSetups); i++) {
        GFL_BGSysCreateBG(sPMSSelectBGSetups[i].bg, &sPMSSelectBGSetups[i].setup, sPMSSelectBGSetups[i].mode);
        GFL_BGSysClearBG(sPMSSelectBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sPMSSelectBGSetups[i].bg, sPMSSelectBGSetups[i].enabled);
    }
}

static void PMSSelectGraphic_BGExit(PMSSelectBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sPMSSelectBGSetups); i++) {
        GFL_BGSysReleaseBG(sPMSSelectBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(PMSSelectBG));
}

static void PMSSelectGraphic_BGMain(PMSSelectBG *bg) {
}

static void PMSSelectGraphic_BGVBlank(PMSSelectBG *bg) {
    GFL_BGSysUpdate();
}

static void PMSSelectGraphic_ClActInit(PMSSelectClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(PMSSelectClAct));
    ClActSys_Create(&sPMSSelectClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void PMSSelectGraphic_ClActExit(PMSSelectClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    MI_CpuFill8(clact, 0, sizeof(PMSSelectClAct));
}

static void PMSSelectGraphic_ClActMain(PMSSelectClAct *clact) {
    func_0204b794();
}

static void PMSSelectGraphic_ClActVBlank(PMSSelectClAct *clact) {
    func_0204b7c8();
}

static ClActUnit *PMSSelectGraphic_ClActGetUnit(PMSSelectClAct *clact) {
    return clact->unit;
}
