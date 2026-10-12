#include "types.h"
#include "app/wbt_list_graphic.h"
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

// The BGs and cell actors of the Pokémon World Tournament's list of Trainers, named after the ROM's
// "wbt_list_graphic.c"

#define BG_COUNT 7

typedef struct {
    u32 unk0;
} WbtListBG;

typedef struct {
    ClActUnit *unit;
} WbtListClAct;

struct WbtListGraphic {
    WbtListBG bg;
    WbtListClAct clact;
    u32 unk8;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} WbtListBGSetup;

static void WbtListGraphic_VBlank(TCB *tcb, void *data);
static void WbtListBG_Init(WbtListBG *bg, HeapID heapId);
static void WbtListBG_Free(WbtListBG *bg);
static void WbtListBG_Update(WbtListBG *bg);
static void WbtListBG_VBlank(WbtListBG *bg);
static void WbtListClAct_Init(WbtListClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void WbtListClAct_Free(WbtListClAct *clact);
static void WbtListClAct_Update(WbtListClAct *clact);
static void WbtListClAct_VBlank(WbtListClAct *clact);
static ClActUnit *WbtListClAct_GetUnit(WbtListClAct *clact);

static const BGSysLCDConfig sWbtListLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };
static const ClActSysSetup sWbtListClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

static const BGSysVRAMConfig sWbtListVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_32K,  GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const WbtListBGSetup sWbtListBGSetups[BG_COUNT] = {
    { 0,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x4000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x8000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 2,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x14000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 3,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x14000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x4000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 5,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 6,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE }
};

WbtListGraphic *WbtListGraphic_Create(u32 layout, HeapID heapId) {
    WbtListGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(WbtListGraphic), FALSE, "wbt_list_graphic.c", 455);

    sys_memset(graphic, 0, sizeof(WbtListGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sWbtListVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    func_020232d0();
    WbtListBG_Init(&graphic->bg, heapId);
    WbtListClAct_Init(&graphic->clact, &sWbtListVRAMConfig, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(WbtListGraphic_VBlank, graphic, 0);
    return graphic;
}

void WbtListGraphic_Free(WbtListGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    WbtListClAct_Free(&graphic->clact);
    WbtListBG_Free(&graphic->bg);
    func_020232d8();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    sys_memset(graphic, 0, sizeof(WbtListGraphic));
    GFL_HeapFree(graphic);
}

void WbtListGraphic_Update(WbtListGraphic *graphic) {
    WbtListClAct_Update(&graphic->clact);
    WbtListBG_Update(&graphic->bg);
}

void WbtListGraphic_Begin3D(WbtListGraphic *graphic) {
}

void WbtListGraphic_End3D(WbtListGraphic *graphic) {
}

ClActUnit *WbtListGraphic_GetClActUnit(WbtListGraphic *graphic) {
    return WbtListClAct_GetUnit(&graphic->clact);
}

static void WbtListGraphic_VBlank(TCB *tcb, void *data) {
    WbtListGraphic *graphic = data;

    WbtListBG_VBlank(&graphic->bg);
    WbtListClAct_VBlank(&graphic->clact);
}

static void WbtListBG_Init(WbtListBG *bg, HeapID heapId) {
    u32 i;

    sys_memset(bg, 0, sizeof(WbtListBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sWbtListLCDConfig);
    for (i = 0; i < NELEMS(sWbtListBGSetups); i++) {
        GFL_BGSysCreateBG(sWbtListBGSetups[i].bg, &sWbtListBGSetups[i].setup, sWbtListBGSetups[i].mode);
        GFL_BGSysClearBG(sWbtListBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sWbtListBGSetups[i].bg, sWbtListBGSetups[i].enabled);
    }
}

static void WbtListBG_Free(WbtListBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sWbtListBGSetups); i++) {
        GFL_BGSysReleaseBG(sWbtListBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    sys_memset(bg, 0, sizeof(WbtListBG));
}

static void WbtListBG_Update(WbtListBG *bg) {
}

static void WbtListBG_VBlank(WbtListBG *bg) {
    GFL_BGSysUpdate();
}

static void WbtListClAct_Init(WbtListClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    sys_memset(clact, 0, sizeof(WbtListClAct));
    ClActSys_Create(&sWbtListClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void WbtListClAct_Free(WbtListClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    sys_memset(clact, 0, sizeof(WbtListClAct));
}

static void WbtListClAct_Update(WbtListClAct *clact) {
    func_0204b794();
}

static void WbtListClAct_VBlank(WbtListClAct *clact) {
    func_0204b7c8();
}

static ClActUnit *WbtListClAct_GetUnit(WbtListClAct *clact) {
    return clact->unit;
}
