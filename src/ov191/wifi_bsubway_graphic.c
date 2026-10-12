#include "types.h"
#include "app/wifi_bsubway_graphic.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "system/gf_font.h"

// The BGs of the Wi-Fi Battle Subway's screen, named after the ROM's "wifi_bsubway_graphic.c". It is the graphic
// template of pkwd_selectscreen_graphic.c without the cell actors

#define BG_COUNT 8

typedef struct {
    u32 unk0;
} WifiBSubwayBG;

struct WifiBSubwayGraphic {
    WifiBSubwayBG bg;
    u32 unk4;
    u32 unk8;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} WifiBSubwayBGSetup;

static void WifiBSubwayGraphic_VBlank(TCB *tcb, void *data);
static void WifiBSubwayBG_Init(WifiBSubwayBG *bg, HeapID heapId);
static void WifiBSubwayBG_Free(WifiBSubwayBG *bg);
static void WifiBSubwayBG_VBlank(WifiBSubwayBG *bg);

static const BGSysLCDConfig sWifiBSubwayLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

static const BGSysVRAMConfig sWifiBSubwayVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static const WifiBSubwayBGSetup sWifiBSubwayBGSetups[BG_COUNT] = {
    { 0,
      { 0, 0, 0x1000, 0, BGRES_512x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x4000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x8000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 2,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 3,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x18000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x4000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 5,
      { 0, 0, 0x1000, 0, BGRES_512x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x8000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 6,
      { 0, 0, 0x1000, 0, BGRES_512x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 7,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE }
};

WifiBSubwayGraphic *WifiBSubwayGraphic_Create(u32 layout, HeapID heapId) {
    WifiBSubwayGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(WifiBSubwayGraphic), FALSE, "wifi_bsubway_graphic.c", 444);

    sys_memset(graphic, 0, sizeof(WifiBSubwayGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sWifiBSubwayVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    func_020232d0();
    WifiBSubwayBG_Init(&graphic->bg, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(WifiBSubwayGraphic_VBlank, graphic, 0);
    return graphic;
}

void WifiBSubwayGraphic_Free(WifiBSubwayGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    WifiBSubwayBG_Free(&graphic->bg);
    func_020232d8();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    sys_memset(graphic, 0, sizeof(WifiBSubwayGraphic));
    GFL_HeapFree(graphic);
}

static void WifiBSubwayGraphic_VBlank(TCB *tcb, void *data) {
    WifiBSubwayGraphic *graphic = data;

    WifiBSubwayBG_VBlank(&graphic->bg);
}

static void WifiBSubwayBG_Init(WifiBSubwayBG *bg, HeapID heapId) {
    u32 i;

    sys_memset(bg, 0, sizeof(WifiBSubwayBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sWifiBSubwayLCDConfig);
    for (i = 0; i < NELEMS(sWifiBSubwayBGSetups); i++) {
        GFL_BGSysCreateBG(sWifiBSubwayBGSetups[i].bg, &sWifiBSubwayBGSetups[i].setup, sWifiBSubwayBGSetups[i].mode);
        GFL_BGSysClearBG(sWifiBSubwayBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sWifiBSubwayBGSetups[i].bg, sWifiBSubwayBGSetups[i].enabled);
    }
}

static void WifiBSubwayBG_Free(WifiBSubwayBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sWifiBSubwayBGSetups); i++) {
        GFL_BGSysReleaseBG(sWifiBSubwayBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    sys_memset(bg, 0, sizeof(WifiBSubwayBG));
}

static void WifiBSubwayBG_VBlank(WifiBSubwayBG *bg) {
    GFL_BGSysUpdate();
}
