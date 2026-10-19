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

static void BoxSearchGraphic_VBlank(TCB *tcb, void *data);
static void BoxSearchBG_Create(BoxSearchBG *bg, HeapID heapId);
static void BoxSearchBG_Delete(BoxSearchBG *bg);
static void BoxSearchBG_Update(BoxSearchBG *bg);
static void BoxSearchBG_VBlank(BoxSearchBG *bg);
static void BoxSearchClAct_Create(BoxSearchClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void BoxSearchClAct_Delete(BoxSearchClAct *clact);
static void BoxSearchClAct_Update(BoxSearchClAct *clact);
static void BoxSearchClAct_VBlank(BoxSearchClAct *clact);
static ClActUnit *BoxSearchClAct_GetUnit(BoxSearchClAct *clact);

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

BoxSearchGraphic *BoxSearchGraphic_Create(u32 layout, HeapID heapId) {
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
    GFL_TextRndInitColorIndexLUT();
    BoxSearchBG_Create(&graphic->bg, heapId);
    BoxSearchClAct_Create(&graphic->clact, &data_ov255_021d94bc, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(BoxSearchGraphic_VBlank, graphic, 0);
    return graphic;
}

void BoxSearchGraphic_Delete(BoxSearchGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    BoxSearchClAct_Delete(&graphic->clact);
    BoxSearchBG_Delete(&graphic->bg);
    GFL_TextRndResetColorIndexLUT();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(BoxSearchGraphic));
    GFL_HeapFree(graphic);
}

void BoxSearchGraphic_Update(BoxSearchGraphic *graphic) {
    BoxSearchClAct_Update(&graphic->clact);
    BoxSearchBG_Update(&graphic->bg);
}

void BoxSearchGraphic_Begin3D(BoxSearchGraphic *graphic) {
}

void BoxSearchGraphic_End3D(BoxSearchGraphic *graphic) {
}

ClActUnit *BoxSearchGraphic_GetClActUnit(BoxSearchGraphic *graphic) {
    return BoxSearchClAct_GetUnit(&graphic->clact);
}

static void BoxSearchGraphic_VBlank(TCB *tcb, void *data) {
    BoxSearchGraphic *graphic = data;

    BoxSearchBG_VBlank(&graphic->bg);
    BoxSearchClAct_VBlank(&graphic->clact);
}

static void BoxSearchBG_Create(BoxSearchBG *bg, HeapID heapId) {
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

static void BoxSearchBG_Delete(BoxSearchBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(data_ov255_021d94ec); i++) {
        GFL_BGSysReleaseBG(data_ov255_021d94ec[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(BoxSearchBG));
}

static void BoxSearchBG_Update(BoxSearchBG *bg) {
}

static void BoxSearchBG_VBlank(BoxSearchBG *bg) {
    GFL_BGSysUpdate();
}

static void BoxSearchClAct_Create(BoxSearchClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(BoxSearchClAct));
    ClActSys_Create(&data_ov255_021d94a0, vramConfig, heapId);
    clact->unit = ClActUnit_Create(128, 0, heapId);
    ClActUnit_UseSystemRenderer(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void BoxSearchClAct_Delete(BoxSearchClAct *clact) {
    ClActUnit_Delete(clact->unit);
    ClActSys_Delete();
    MI_CpuFill8(clact, 0, sizeof(BoxSearchClAct));
}

static void BoxSearchClAct_Update(BoxSearchClAct *clact) {
    ClActSys_Draw();
}

static void BoxSearchClAct_VBlank(BoxSearchClAct *clact) {
    ClActSys_VBlank();
}

static ClActUnit *BoxSearchClAct_GetUnit(BoxSearchClAct *clact) {
    return clact->unit;
}
