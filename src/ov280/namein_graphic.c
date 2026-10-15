#include "types.h"
#include "app/namein_graphic.h"
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

// The BGs and cell actors of the name-entry screen, named after the ROM's "namein_graphic.c". The name-entry screen
// is overlay 280. The code is the same as box_search_graphic.c's, with other tables

#define BG_COUNT 8

typedef struct {
    u32 unk0;
} NameinGraphicBG;

typedef struct {
    ClActUnit *unit;
} NameinGraphicClAct;

struct NameinGraphic {
    NameinGraphicBG bg;
    NameinGraphicClAct clact;
    u32 unk8;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} NameinGraphicBGSetup;

static void NameinGraphic_VBlank(TCB *tcb, void *data);
static void NameinGraphic_BGInit(NameinGraphicBG *bg, HeapID heapId);
static void NameinGraphic_BGExit(NameinGraphicBG *bg);
static void NameinGraphic_BGMain(NameinGraphicBG *bg);
static void NameinGraphic_BGVBlank(NameinGraphicBG *bg);
static void NameinGraphic_ClActInit(NameinGraphicClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void NameinGraphic_ClActExit(NameinGraphicClAct *clact);
static void NameinGraphic_ClActMain(NameinGraphicClAct *clact);
static void NameinGraphic_ClActVBlank(NameinGraphicClAct *clact);
static ClActUnit *NameinGraphic_ClActGetUnit(NameinGraphicClAct *clact);

static const BGSysLCDConfig sLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

static const ClActSysSetup sClActSysSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

// clang-format off
static const BGSysVRAMConfig sVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE, GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,      GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,   GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

// clang-format on

// clang-format off
static const NameinGraphicBGSetup sBGSetups[BG_COUNT] = {
    { 0,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 2,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 3,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 5,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x08000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 6,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 7,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x14000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
};

// clang-format on

NameinGraphic *NameinGraphic_Create(u32 layout, HeapID heapId) {
    NameinGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(NameinGraphic), FALSE, "namein_graphic.c", 443);

    sys_memset(graphic, 0, sizeof(NameinGraphic));
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
    func_020232d0();
    NameinGraphic_BGInit(&graphic->bg, heapId);
    NameinGraphic_ClActInit(&graphic->clact, &sVRAMConfig, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(NameinGraphic_VBlank, graphic, 0);
    return graphic;
}

void NameinGraphic_Delete(NameinGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    NameinGraphic_ClActExit(&graphic->clact);
    NameinGraphic_BGExit(&graphic->bg);
    func_020232d8();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    sys_memset(graphic, 0, sizeof(NameinGraphic));
    GFL_HeapFree(graphic);
}

void NameinGraphic_Main(NameinGraphic *graphic) {
    NameinGraphic_ClActMain(&graphic->clact);
    NameinGraphic_BGMain(&graphic->bg);
}

ClActUnit *NameinGraphic_GetClActUnit(NameinGraphic *graphic) {
    return NameinGraphic_ClActGetUnit(&graphic->clact);
}

static void NameinGraphic_VBlank(TCB *tcb, void *data) {
    NameinGraphic *graphic = data;

    NameinGraphic_BGVBlank(&graphic->bg);
    NameinGraphic_ClActVBlank(&graphic->clact);
}

static void NameinGraphic_BGInit(NameinGraphicBG *bg, HeapID heapId) {
    u32 i;

    sys_memset(bg, 0, sizeof(NameinGraphicBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sLCDConfig);
    for (i = 0; i < NELEMS(sBGSetups); i++) {
        GFL_BGSysCreateBG(sBGSetups[i].bg, &sBGSetups[i].setup, sBGSetups[i].mode);
        GFL_BGSysClearBG(sBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sBGSetups[i].bg, sBGSetups[i].enabled);
    }
}

static void NameinGraphic_BGExit(NameinGraphicBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sBGSetups); i++) {
        GFL_BGSysReleaseBG(sBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    sys_memset(bg, 0, sizeof(NameinGraphicBG));
}

static void NameinGraphic_BGMain(NameinGraphicBG *bg) {
}

static void NameinGraphic_BGVBlank(NameinGraphicBG *bg) {
    GFL_BGSysUpdate();
}

static void NameinGraphic_ClActInit(NameinGraphicClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    sys_memset(clact, 0, sizeof(NameinGraphicClAct));
    ClActSys_Create(&sClActSysSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void NameinGraphic_ClActExit(NameinGraphicClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    sys_memset(clact, 0, sizeof(NameinGraphicClAct));
}

static void NameinGraphic_ClActMain(NameinGraphicClAct *clact) {
    func_0204b794();
}

static void NameinGraphic_ClActVBlank(NameinGraphicClAct *clact) {
    func_0204b7c8();
}

static ClActUnit *NameinGraphic_ClActGetUnit(NameinGraphicClAct *clact) {
    return clact->unit;
}
