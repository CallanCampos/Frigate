#include "types.h"
#include "app/pkwd_selectscreen_graphic.h"
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

// The BGs and cell actors of Pokéstar Studios' movie select screen, named after the ROM's
// "pkwd_selectscreen_graphic.c"

#define BG_COUNT 8

typedef struct {
    u32 unk0;
} PkwdSelectScreenBG;

typedef struct {
    ClActUnit *unit;
} PkwdSelectScreenClAct;

struct PkwdSelectScreenGraphic {
    PkwdSelectScreenBG bg;
    PkwdSelectScreenClAct clact;
    u32 unk8;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} PkwdSelectScreenBGSetup;

static void PkwdSelectScreenGraphic_VBlank(TCB *tcb, void *data);
static void PkwdSelectScreenBG_Init(PkwdSelectScreenBG *bg, HeapID heapId);
static void PkwdSelectScreenBG_Free(PkwdSelectScreenBG *bg);
static void PkwdSelectScreenBG_Update(PkwdSelectScreenBG *bg);
static void PkwdSelectScreenBG_VBlank(PkwdSelectScreenBG *bg);
static void PkwdSelectScreenClAct_Init(PkwdSelectScreenClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void PkwdSelectScreenClAct_Free(PkwdSelectScreenClAct *clact);
static void PkwdSelectScreenClAct_Update(PkwdSelectScreenClAct *clact);
static void PkwdSelectScreenClAct_VBlank(PkwdSelectScreenClAct *clact);
static ClActUnit *PkwdSelectScreenClAct_GetUnit(PkwdSelectScreenClAct *clact);

static const BGSysLCDConfig sPkwdSelectScreenLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };
static const ClActSysSetup sPkwdSelectScreenClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

static const BGSysVRAMConfig sPkwdSelectScreenVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static const PkwdSelectScreenBGSetup sPkwdSelectScreenBGSetups[BG_COUNT] = {
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
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, 0 },
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
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x8000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 6,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 7,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x14000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE }
};

PkwdSelectScreenGraphic *PkwdSelectScreenGraphic_Create(u32 layout, HeapID heapId) {
    PkwdSelectScreenGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(PkwdSelectScreenGraphic), FALSE, "pkwd_selectscreen_graphic.c", 319);

    sys_memset(graphic, 0, sizeof(PkwdSelectScreenGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sPkwdSelectScreenVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    func_020232d0();
    PkwdSelectScreenBG_Init(&graphic->bg, heapId);
    PkwdSelectScreenClAct_Init(&graphic->clact, &sPkwdSelectScreenVRAMConfig, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(PkwdSelectScreenGraphic_VBlank, graphic, 0);
    return graphic;
}

void PkwdSelectScreenGraphic_Free(PkwdSelectScreenGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    PkwdSelectScreenClAct_Free(&graphic->clact);
    PkwdSelectScreenBG_Free(&graphic->bg);
    func_020232d8();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    sys_memset(graphic, 0, sizeof(PkwdSelectScreenGraphic));
    GFL_HeapFree(graphic);
}

void PkwdSelectScreenGraphic_Update(PkwdSelectScreenGraphic *graphic) {
    PkwdSelectScreenClAct_Update(&graphic->clact);
    PkwdSelectScreenBG_Update(&graphic->bg);
}

ClActUnit *PkwdSelectScreenGraphic_GetClActUnit(PkwdSelectScreenGraphic *graphic) {
    return PkwdSelectScreenClAct_GetUnit(&graphic->clact);
}

static void PkwdSelectScreenGraphic_VBlank(TCB *tcb, void *data) {
    PkwdSelectScreenGraphic *graphic = data;

    PkwdSelectScreenBG_VBlank(&graphic->bg);
    PkwdSelectScreenClAct_VBlank(&graphic->clact);
}

static void PkwdSelectScreenBG_Init(PkwdSelectScreenBG *bg, HeapID heapId) {
    u32 i;

    sys_memset(bg, 0, sizeof(PkwdSelectScreenBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sPkwdSelectScreenLCDConfig);
    for (i = 0; i < NELEMS(sPkwdSelectScreenBGSetups); i++) {
        GFL_BGSysCreateBG(sPkwdSelectScreenBGSetups[i].bg, &sPkwdSelectScreenBGSetups[i].setup, sPkwdSelectScreenBGSetups[i].mode);
        GFL_BGSysClearBG(sPkwdSelectScreenBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sPkwdSelectScreenBGSetups[i].bg, sPkwdSelectScreenBGSetups[i].enabled);
    }
}

static void PkwdSelectScreenBG_Free(PkwdSelectScreenBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sPkwdSelectScreenBGSetups); i++) {
        GFL_BGSysReleaseBG(sPkwdSelectScreenBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    sys_memset(bg, 0, sizeof(PkwdSelectScreenBG));
}

static void PkwdSelectScreenBG_Update(PkwdSelectScreenBG *bg) {
}

static void PkwdSelectScreenBG_VBlank(PkwdSelectScreenBG *bg) {
    GFL_BGSysUpdate();
}

static void PkwdSelectScreenClAct_Init(PkwdSelectScreenClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    sys_memset(clact, 0, sizeof(PkwdSelectScreenClAct));
    ClActSys_Create(&sPkwdSelectScreenClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void PkwdSelectScreenClAct_Free(PkwdSelectScreenClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    sys_memset(clact, 0, sizeof(PkwdSelectScreenClAct));
}

static void PkwdSelectScreenClAct_Update(PkwdSelectScreenClAct *clact) {
    func_0204b794();
}

static void PkwdSelectScreenClAct_VBlank(PkwdSelectScreenClAct *clact) {
    func_0204b7c8();
}

static ClActUnit *PkwdSelectScreenClAct_GetUnit(PkwdSelectScreenClAct *clact) {
    return clact->unit;
}
