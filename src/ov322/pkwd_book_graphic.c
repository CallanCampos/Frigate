#include "types.h"
#include "app/pkwd_book_graphic.h"
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
#include "system/palanm.h"

// The BGs and cell actors of Pokéstar Studios' scenario book, named after the ROM's
// "pkwd_book_graphic.c"

#define BG_COUNT 8

typedef struct {
    u32 unk0;
} PkwdBookBG;

typedef struct {
    ClActUnit *unit;
} PkwdBookClAct;

struct PkwdBookGraphic {
    PkwdBookBG bg;
    PkwdBookClAct clact;
    u32 unk8;
    TCB *vblankTask;
    PaletteFade *paletteFade;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} PkwdBookBGSetup;

static void PkwdBookGraphic_VBlank(TCB *tcb, void *data);
static void PkwdBookBG_Init(PkwdBookBG *bg, HeapID heapId);
static void PkwdBookBG_Free(PkwdBookBG *bg);
static void PkwdBookBG_Update(PkwdBookBG *bg);
static void PkwdBookBG_VBlank(PkwdBookBG *bg);
static void PkwdBookClAct_Init(PkwdBookClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void PkwdBookClAct_Free(PkwdBookClAct *clact);
static void PkwdBookClAct_Update(PkwdBookClAct *clact);
static void PkwdBookClAct_VBlank(PkwdBookClAct *clact);
static ClActUnit *PkwdBookClAct_GetUnit(PkwdBookClAct *clact);

static const BGSysLCDConfig sPkwdBookLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };
static const ClActSysSetup sPkwdBookClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

static const BGSysVRAMConfig sPkwdBookVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static const PkwdBookBGSetup sPkwdBookBGSetups[BG_COUNT] = {
    { 0,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x4000), 0x4000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x8000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 2,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 3,
      { 0, 0, 0x2000, 0, BGRES_512x512, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x18000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x4000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 5,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0xc000), 0x4000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 6,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 7,
      { 0, 0, 0x2000, 0, BGRES_512x512, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x18000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE }
};

PkwdBookGraphic *PkwdBookGraphic_Create(u32 layout, HeapID heapId) {
    PkwdBookGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(PkwdBookGraphic), FALSE, "pkwd_book_graphic.c", 454);

    MI_CpuFill8(graphic, 0, sizeof(PkwdBookGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sPkwdBookVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    func_020232d0();
    PkwdBookBG_Init(&graphic->bg, heapId);
    PkwdBookClAct_Init(&graphic->clact, &sPkwdBookVRAMConfig, heapId);
    graphic->paletteFade = PaletteFade_Create(heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(PkwdBookGraphic_VBlank, graphic, 0);
    return graphic;
}

void PkwdBookGraphic_Free(PkwdBookGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    PkwdBookClAct_Free(&graphic->clact);
    PkwdBookBG_Free(&graphic->bg);
    PaletteFade_Free(graphic->paletteFade);
    func_020232d8();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(PkwdBookGraphic));
    GFL_HeapFree(graphic);
}

void PkwdBookGraphic_Update(PkwdBookGraphic *graphic) {
    PkwdBookClAct_Update(&graphic->clact);
    PkwdBookBG_Update(&graphic->bg);
}

void PkwdBookGraphic_Begin3D(PkwdBookGraphic *graphic) {
}

void PkwdBookGraphic_End3D(PkwdBookGraphic *graphic) {
}

ClActUnit *PkwdBookGraphic_GetClActUnit(PkwdBookGraphic *graphic) {
    return PkwdBookClAct_GetUnit(&graphic->clact);
}

static void PkwdBookGraphic_VBlank(TCB *tcb, void *data) {
    PkwdBookGraphic *graphic = data;

    PkwdBookBG_VBlank(&graphic->bg);
    PkwdBookClAct_VBlank(&graphic->clact);
    PaletteFade_Transfer(graphic->paletteFade);
}

static void PkwdBookBG_Init(PkwdBookBG *bg, HeapID heapId) {
    u32 i;

    MI_CpuFill8(bg, 0, sizeof(PkwdBookBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sPkwdBookLCDConfig);
    for (i = 0; i < NELEMS(sPkwdBookBGSetups); i++) {
        GFL_BGSysCreateBG(sPkwdBookBGSetups[i].bg, &sPkwdBookBGSetups[i].setup, sPkwdBookBGSetups[i].mode);
        GFL_BGSysClearBG(sPkwdBookBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sPkwdBookBGSetups[i].bg, sPkwdBookBGSetups[i].enabled);
    }
}

static void PkwdBookBG_Free(PkwdBookBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sPkwdBookBGSetups); i++) {
        GFL_BGSysReleaseBG(sPkwdBookBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(PkwdBookBG));
}

static void PkwdBookBG_Update(PkwdBookBG *bg) {
}

static void PkwdBookBG_VBlank(PkwdBookBG *bg) {
    GFL_BGSysUpdate();
}

static void PkwdBookClAct_Init(PkwdBookClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(PkwdBookClAct));
    ClActSys_Create(&sPkwdBookClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void PkwdBookClAct_Free(PkwdBookClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    MI_CpuFill8(clact, 0, sizeof(PkwdBookClAct));
}

static void PkwdBookClAct_Update(PkwdBookClAct *clact) {
    func_0204b794();
}

static void PkwdBookClAct_VBlank(PkwdBookClAct *clact) {
    func_0204b7c8();
}

static ClActUnit *PkwdBookClAct_GetUnit(PkwdBookClAct *clact) {
    return clact->unit;
}
