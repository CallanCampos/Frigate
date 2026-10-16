#include "types.h"
#include "app/box_search_graphic.h"
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

// The BGs and cell actors of the PC box's Pokémon search, named after the ROM's "box_search_graphic.c"

#define BG_COUNT 7

typedef struct {
    u32 unk0;
} BoxSearchBG;

typedef struct {
    ClActUnit *unit;
} BoxSearchClAct;

struct BoxSearchGraphic {
    BoxSearchBG bg;
    BoxSearchClAct clact;
    u32 unk8;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} BoxSearchBGSetup;

static void func_ov255_021d6e44(TCB *tcb, void *data);
static void func_ov255_021d6e58(BoxSearchBG *bg, HeapID heapId);
static void func_ov255_021d6eb8(BoxSearchBG *bg);
static void func_ov255_021d6ef0(BoxSearchBG *bg);
static void func_ov255_021d6ef4(BoxSearchBG *bg);
static void func_ov255_021d6efc(BoxSearchClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void func_ov255_021d6f3c(BoxSearchClAct *clact);
static void func_ov255_021d6f58(BoxSearchClAct *clact);
static void func_ov255_021d6f60(BoxSearchClAct *clact);
static ClActUnit *func_ov255_021d6f68(BoxSearchClAct *clact);

static const BGSysLCDConfig data_ov255_021d9490 = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

static const ClActSysSetup data_ov255_021d94a0 = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

// clang-format off
static const BGSysVRAMConfig data_ov255_021d94bc = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE, GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_D,   GX_VRAM_TEXPLTT_0_F,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

// clang-format on

// clang-format off
static const BoxSearchBGSetup data_ov255_021d94ec[BG_COUNT] = {
    { 0,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x4000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x08000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 2,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x8000,
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
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x08000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
    { 6,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x08000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT, TRUE },
};

// clang-format on

BoxSearchGraphic *func_ov255_021d6d28(u32 layout, HeapID heapId) {
    BoxSearchGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(BoxSearchGraphic), FALSE, "box_search_graphic.c", 449);

    MI_CpuFill8(graphic, 0, sizeof(BoxSearchGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&data_ov255_021d94bc);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    func_020232d0();
    func_ov255_021d6e58(&graphic->bg, heapId);
    func_ov255_021d6efc(&graphic->clact, &data_ov255_021d94bc, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(func_ov255_021d6e44, graphic, 0);
    return graphic;
}

void func_ov255_021d6dc8(BoxSearchGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    func_ov255_021d6f3c(&graphic->clact);
    func_ov255_021d6eb8(&graphic->bg);
    func_020232d8();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(BoxSearchGraphic));
    GFL_HeapFree(graphic);
}

void func_ov255_021d6e1c(BoxSearchGraphic *graphic) {
    func_ov255_021d6f58(&graphic->clact);
    func_ov255_021d6ef0(&graphic->bg);
}

void func_ov255_021d6e30(BoxSearchGraphic *graphic) {
}

void func_ov255_021d6e34(BoxSearchGraphic *graphic) {
}

ClActUnit *func_ov255_021d6e38(BoxSearchGraphic *graphic) {
    return func_ov255_021d6f68(&graphic->clact);
}

static void func_ov255_021d6e44(TCB *tcb, void *data) {
    BoxSearchGraphic *graphic = data;

    func_ov255_021d6ef4(&graphic->bg);
    func_ov255_021d6f60(&graphic->clact);
}

static void func_ov255_021d6e58(BoxSearchBG *bg, HeapID heapId) {
    u32 i;

    MI_CpuFill8(bg, 0, sizeof(BoxSearchBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&data_ov255_021d9490);
    for (i = 0; i < NELEMS(data_ov255_021d94ec); i++) {
        GFL_BGSysCreateBG(data_ov255_021d94ec[i].bg, &data_ov255_021d94ec[i].setup, data_ov255_021d94ec[i].mode);
        GFL_BGSysClearBG(data_ov255_021d94ec[i].bg);
        GFL_BGSysSetBGEnabled(data_ov255_021d94ec[i].bg, data_ov255_021d94ec[i].enabled);
    }
}

static void func_ov255_021d6eb8(BoxSearchBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(data_ov255_021d94ec); i++) {
        GFL_BGSysReleaseBG(data_ov255_021d94ec[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(BoxSearchBG));
}

static void func_ov255_021d6ef0(BoxSearchBG *bg) {
}

static void func_ov255_021d6ef4(BoxSearchBG *bg) {
    GFL_BGSysUpdate();
}

static void func_ov255_021d6efc(BoxSearchClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(BoxSearchClAct));
    ClActSys_Create(&data_ov255_021d94a0, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void func_ov255_021d6f3c(BoxSearchClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    MI_CpuFill8(clact, 0, sizeof(BoxSearchClAct));
}

static void func_ov255_021d6f58(BoxSearchClAct *clact) {
    func_0204b794();
}

static void func_ov255_021d6f60(BoxSearchClAct *clact) {
    func_0204b7c8();
}

static ClActUnit *func_ov255_021d6f68(BoxSearchClAct *clact) {
    return clact->unit;
}
