#include "types.h"
#include "app/wifibattlematch_graphic.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "system/gf_font.h"

// The BGs and cell actor system that the Wi-Fi and live competition screens of overlay 261 draw on (the file name is
// the ROM's own string)

#define BG_COUNT 8

typedef struct {
    u32 unk0;
} WifiBattleMatchGraphicBG;

typedef struct {
    ClActUnit *unit;
} WifiBattleMatchGraphicClAct;

struct WifiBattleMatchGraphic {
    WifiBattleMatchGraphicBG bg;
    WifiBattleMatchGraphicClAct clact;
    u32 unk8;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} WifiBattleMatchGraphicBGSetup;

static void WifiBattleMatchGraphic_VBlank(TCB *tcb, void *data);
static void WifiBattleMatchGraphic_BGInit(WifiBattleMatchGraphicBG *bg, HeapID heapId);
static void WifiBattleMatchGraphic_BGFree(WifiBattleMatchGraphicBG *bg);
static void WifiBattleMatchGraphic_BGUpdate(WifiBattleMatchGraphicBG *bg);
static void WifiBattleMatchGraphic_BGVBlank(WifiBattleMatchGraphicBG *bg);
static void WifiBattleMatchGraphic_ClActInit(WifiBattleMatchGraphicClAct *clact, const BGSysVRAMConfig *vramConfig,
                                             HeapID heapId);
static void WifiBattleMatchGraphic_ClActFree(WifiBattleMatchGraphicClAct *clact);
static void WifiBattleMatchGraphic_ClActUpdate(WifiBattleMatchGraphicClAct *clact);
static void WifiBattleMatchGraphic_ClActVBlank(WifiBattleMatchGraphicClAct *clact);
static ClActUnit *WifiBattleMatchGraphic_ClActGetUnit(WifiBattleMatchGraphicClAct *clact);

static const BGSysLCDConfig sLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

static const ClActSysSetup sClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

// clang-format off
static const BGSysVRAMConfig sVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,      GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

// clang-format on

// clang-format off
static const WifiBattleMatchGraphicBGSetup sBGSetups[BG_COUNT] = {
    { 0,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 1,
      { 0, 0, 0x1000, 0, BGRES_512x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x08000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 2,
      { 0, 0, 0x1000, 0, BGRES_512x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 3,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 5,
      { 0, 0, 0x1000, 0, BGRES_512x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x08000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 6,
      { 0, 0, 0x1000, 0, BGRES_512x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 7,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
};

// clang-format on

WifiBattleMatchGraphic *WifiBattleMatchGraphic_Create(u32 layout, HeapID heapId) {
    WifiBattleMatchGraphic *graphic =
        GFL_HeapAllocate(heapId, sizeof(WifiBattleMatchGraphic), FALSE, "wifibattlematch_graphic.c", 443);

    sys_memset(graphic, 0, sizeof(WifiBattleMatchGraphic));
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
    WifiBattleMatchGraphic_BGInit(&graphic->bg, heapId);
    WifiBattleMatchGraphic_ClActInit(&graphic->clact, &sVRAMConfig, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(WifiBattleMatchGraphic_VBlank, graphic, 0);
    return graphic;
}

void WifiBattleMatchGraphic_Free(WifiBattleMatchGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    WifiBattleMatchGraphic_ClActFree(&graphic->clact);
    WifiBattleMatchGraphic_BGFree(&graphic->bg);
    func_020232d8();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    sys_memset(graphic, 0, sizeof(WifiBattleMatchGraphic));
    GFL_HeapFree(graphic);
}

void WifiBattleMatchGraphic_Update(WifiBattleMatchGraphic *graphic) {
    WifiBattleMatchGraphic_ClActUpdate(&graphic->clact);
    WifiBattleMatchGraphic_BGUpdate(&graphic->bg);
}

ClActUnit *WifiBattleMatchGraphic_GetClActUnit(WifiBattleMatchGraphic *graphic) {
    return WifiBattleMatchGraphic_ClActGetUnit(&graphic->clact);
}

static void WifiBattleMatchGraphic_VBlank(TCB *tcb, void *data) {
    WifiBattleMatchGraphic *graphic = data;

    WifiBattleMatchGraphic_BGVBlank(&graphic->bg);
    WifiBattleMatchGraphic_ClActVBlank(&graphic->clact);
}

static void WifiBattleMatchGraphic_BGInit(WifiBattleMatchGraphicBG *bg, HeapID heapId) {
    u32 i;

    sys_memset(bg, 0, sizeof(WifiBattleMatchGraphicBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sLCDConfig);
    for (i = 0; i < NELEMS(sBGSetups); i++) {
        GFL_BGSysCreateBG(sBGSetups[i].bg, &sBGSetups[i].setup, sBGSetups[i].mode);
        GFL_BGSysClearBG(sBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sBGSetups[i].bg, sBGSetups[i].enabled);
    }
}

static void WifiBattleMatchGraphic_BGFree(WifiBattleMatchGraphicBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sBGSetups); i++) {
        GFL_BGSysReleaseBG(sBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    sys_memset(bg, 0, sizeof(WifiBattleMatchGraphicBG));
}

static void WifiBattleMatchGraphic_BGUpdate(WifiBattleMatchGraphicBG *bg) {
}

static void WifiBattleMatchGraphic_BGVBlank(WifiBattleMatchGraphicBG *bg) {
    GFL_BGSysUpdate();
}

static void WifiBattleMatchGraphic_ClActInit(WifiBattleMatchGraphicClAct *clact, const BGSysVRAMConfig *vramConfig,
                                             HeapID heapId) {
    sys_memset(clact, 0, sizeof(WifiBattleMatchGraphicClAct));
    ClActSys_Create(&sClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void WifiBattleMatchGraphic_ClActFree(WifiBattleMatchGraphicClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    sys_memset(clact, 0, sizeof(WifiBattleMatchGraphicClAct));
}

static void WifiBattleMatchGraphic_ClActUpdate(WifiBattleMatchGraphicClAct *clact) {
    func_0204b794();
}

static void WifiBattleMatchGraphic_ClActVBlank(WifiBattleMatchGraphicClAct *clact) {
    func_0204b7c8();
}

static ClActUnit *WifiBattleMatchGraphic_ClActGetUnit(WifiBattleMatchGraphicClAct *clact) {
    return clact->unit;
}
