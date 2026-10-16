#include "types.h"
#include "app/marine_tube_board.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "system/gf_font.h"

// The BGs and cell actors of the Marine Tube's board

typedef struct {
    u32 unk0;
} MarineTubeBoardBG;

typedef struct {
    ClActUnit *unit;
} MarineTubeBoardClAct;

struct MarineTubeBoardGraphic {
    MarineTubeBoardBG bg;
    MarineTubeBoardClAct clact;
    u32 unk08;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} MarineTubeBoardBGSetup;

static void MarineTubeBoardGraphic_VBlank(TCB *tcb, void *data);
static void MarineTubeBoardBG_Init(MarineTubeBoardBG *bg, HeapID heapId);
static void MarineTubeBoardBG_Free(MarineTubeBoardBG *bg);
static void MarineTubeBoardBG_Update(MarineTubeBoardBG *bg);
static void MarineTubeBoardBG_VBlank(MarineTubeBoardBG *bg);
static void MarineTubeBoardClAct_Init(MarineTubeBoardClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void MarineTubeBoardClAct_Free(MarineTubeBoardClAct *clact);
static void MarineTubeBoardClAct_Update(MarineTubeBoardClAct *clact);
static void MarineTubeBoardClAct_VBlank(MarineTubeBoardClAct *clact);

static const BGSysLCDConfig sMarineTubeBoardLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0,
                                                          GX_BG0_AS_3D };
static const ClActSysSetup sMarineTubeBoardClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

static const BGSysVRAMConfig sMarineTubeBoardVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,        GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_D,   GX_VRAM_TEXPLTT_0_F,     GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const MarineTubeBoardBGSetup sMarineTubeBoardBGSetups[3] = {
    { 1,
      { 0, 0, 0x1000, 0, BGRES_256x512, GX_BG_COLORMODE_256, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x4000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x4000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 5,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x8000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

MarineTubeBoardGraphic *MarineTubeBoardGraphic_Create(u32 layout, HeapID heapId) {
    MarineTubeBoardGraphic *graphic =
        GFL_HeapAllocate(heapId, sizeof(MarineTubeBoardGraphic), FALSE, "marine_tube_board_graphic.c", 453);

    MI_CpuFill8(graphic, 0, sizeof(MarineTubeBoardGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sMarineTubeBoardVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    func_020232d0();
    MarineTubeBoardBG_Init(&graphic->bg, heapId);
    MarineTubeBoardClAct_Init(&graphic->clact, &sMarineTubeBoardVRAMConfig, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(MarineTubeBoardGraphic_VBlank, graphic, 0);
    return graphic;
}

void MarineTubeBoardGraphic_Free(MarineTubeBoardGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    MarineTubeBoardClAct_Free(&graphic->clact);
    MarineTubeBoardBG_Free(&graphic->bg);
    func_020232d8();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(MarineTubeBoardGraphic));
    GFL_HeapFree(graphic);
}

void MarineTubeBoardGraphic_Update(MarineTubeBoardGraphic *graphic) {
    MarineTubeBoardClAct_Update(&graphic->clact);
    MarineTubeBoardBG_Update(&graphic->bg);
}

static void MarineTubeBoardGraphic_VBlank(TCB *tcb, void *data) {
    MarineTubeBoardGraphic *graphic = data;

    MarineTubeBoardBG_VBlank(&graphic->bg);
    MarineTubeBoardClAct_VBlank(&graphic->clact);
}

static void MarineTubeBoardBG_Init(MarineTubeBoardBG *bg, HeapID heapId) {
    u32 i;

    MI_CpuFill8(bg, 0, sizeof(MarineTubeBoardBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sMarineTubeBoardLCDConfig);
    for (i = 0; i < NELEMS(sMarineTubeBoardBGSetups); i++) {
        GFL_BGSysCreateBG(sMarineTubeBoardBGSetups[i].bg, &sMarineTubeBoardBGSetups[i].setup,
                          sMarineTubeBoardBGSetups[i].mode);
        GFL_BGSysClearBG(sMarineTubeBoardBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sMarineTubeBoardBGSetups[i].bg, sMarineTubeBoardBGSetups[i].enabled);
    }
}

static void MarineTubeBoardBG_Free(MarineTubeBoardBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sMarineTubeBoardBGSetups); i++) {
        GFL_BGSysReleaseBG(sMarineTubeBoardBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(MarineTubeBoardBG));
}

static void MarineTubeBoardBG_Update(MarineTubeBoardBG *bg) {
}

static void MarineTubeBoardBG_VBlank(MarineTubeBoardBG *bg) {
    GFL_BGSysUpdate();
}

static void MarineTubeBoardClAct_Init(MarineTubeBoardClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(MarineTubeBoardClAct));
    ClActSys_Create(&sMarineTubeBoardClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void MarineTubeBoardClAct_Free(MarineTubeBoardClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    MI_CpuFill8(clact, 0, sizeof(MarineTubeBoardClAct));
}

static void MarineTubeBoardClAct_Update(MarineTubeBoardClAct *clact) {
    func_0204b794();
}

static void MarineTubeBoardClAct_VBlank(MarineTubeBoardClAct *clact) {
    func_0204b7c8();
}
