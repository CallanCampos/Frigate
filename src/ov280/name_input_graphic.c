#include "types.h"
#include "app/name_input_graphic.h"
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
} NameInputGraphicBG;

typedef struct {
    ClActUnit *unit;
} NameInputGraphicClAct;

struct NameInputGraphic {
    NameInputGraphicBG bg;
    NameInputGraphicClAct clact;
    u32 unk8;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} NameInputGraphicBGSetup;

static void NameInputGraphic_VBlank(TCB *tcb, void *data);
static void NameInputGraphic_BGInit(NameInputGraphicBG *bg, HeapID heapId);
static void NameInputGraphic_BGExit(NameInputGraphicBG *bg);
static void NameInputGraphic_BGMain(NameInputGraphicBG *bg);
static void NameInputGraphic_BGVBlank(NameInputGraphicBG *bg);
static void NameInputGraphic_ClActInit(NameInputGraphicClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void NameInputGraphic_ClActExit(NameInputGraphicClAct *clact);
static void NameInputGraphic_ClActMain(NameInputGraphicClAct *clact);
static void NameInputGraphic_ClActVBlank(NameInputGraphicClAct *clact);
static ClActUnit *NameInputGraphic_ClActGetUnit(NameInputGraphicClAct *clact);

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
static const NameInputGraphicBGSetup sBGSetups[BG_COUNT] = {
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

NameInputGraphic *NameInputGraphic_Create(u32 layout, HeapID heapId) {
    NameInputGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(NameInputGraphic), FALSE, "namein_graphic.c", 443);

    MI_CpuFill8(graphic, 0, sizeof(NameInputGraphic));
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
    GFL_TextRndInitColorIndexLUT();
    NameInputGraphic_BGInit(&graphic->bg, heapId);
    NameInputGraphic_ClActInit(&graphic->clact, &sVRAMConfig, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(NameInputGraphic_VBlank, graphic, 0);
    return graphic;
}

void NameInputGraphic_Delete(NameInputGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    NameInputGraphic_ClActExit(&graphic->clact);
    NameInputGraphic_BGExit(&graphic->bg);
    GFL_TextRndResetColorIndexLUT();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(NameInputGraphic));
    GFL_HeapFree(graphic);
}

void NameInputGraphic_Main(NameInputGraphic *graphic) {
    NameInputGraphic_ClActMain(&graphic->clact);
    NameInputGraphic_BGMain(&graphic->bg);
}

ClActUnit *NameInputGraphic_GetClActUnit(NameInputGraphic *graphic) {
    return NameInputGraphic_ClActGetUnit(&graphic->clact);
}

static void NameInputGraphic_VBlank(TCB *tcb, void *data) {
    NameInputGraphic *graphic = data;

    NameInputGraphic_BGVBlank(&graphic->bg);
    NameInputGraphic_ClActVBlank(&graphic->clact);
}

static void NameInputGraphic_BGInit(NameInputGraphicBG *bg, HeapID heapId) {
    u32 i;

    MI_CpuFill8(bg, 0, sizeof(NameInputGraphicBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sLCDConfig);
    for (i = 0; i < NELEMS(sBGSetups); i++) {
        GFL_BGSysCreateBG(sBGSetups[i].bg, &sBGSetups[i].setup, sBGSetups[i].mode);
        GFL_BGSysClearBG(sBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sBGSetups[i].bg, sBGSetups[i].enabled);
    }
}

static void NameInputGraphic_BGExit(NameInputGraphicBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sBGSetups); i++) {
        GFL_BGSysReleaseBG(sBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(NameInputGraphicBG));
}

static void NameInputGraphic_BGMain(NameInputGraphicBG *bg) {
}

static void NameInputGraphic_BGVBlank(NameInputGraphicBG *bg) {
    GFL_BGSysUpdate();
}

static void NameInputGraphic_ClActInit(NameInputGraphicClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(NameInputGraphicClAct));
    ClActSys_Create(&sClActSysSetup, vramConfig, heapId);
    clact->unit = ClActUnit_Create(128, 0, heapId);
    ClActUnit_UseSystemRenderer(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void NameInputGraphic_ClActExit(NameInputGraphicClAct *clact) {
    ClActUnit_Delete(clact->unit);
    ClActSys_Delete();
    MI_CpuFill8(clact, 0, sizeof(NameInputGraphicClAct));
}

static void NameInputGraphic_ClActMain(NameInputGraphicClAct *clact) {
    ClActSys_Draw();
}

static void NameInputGraphic_ClActVBlank(NameInputGraphicClAct *clact) {
    ClActSys_VBlank();
}

static ClActUnit *NameInputGraphic_ClActGetUnit(NameInputGraphicClAct *clact) {
    return clact->unit;
}
