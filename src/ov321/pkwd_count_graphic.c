#include "types.h"
#include "app/pkwd_count_graphic.h"
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

// The BGs, cell actors and palette fades of the Pokéstar Studios movie's opening, named after the ROM's
// "pkwd_count_graphic.c"

#define BG_COUNT 8

typedef struct {
    u32 unk0;
} PkwdCountBG;

typedef struct {
    ClActUnit *unit;
} PkwdCountClAct;

struct PkwdCountGraphic {
    PkwdCountBG bg;
    PkwdCountClAct clact;
    u32 unk8;
    TCB *vblankTask;
    PaletteFade *paletteFade;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} PkwdCountBGSetup;

static void PkwdCountGraphic_VBlank(TCB *tcb, void *data);
static void PkwdCountBG_Init(PkwdCountBG *bg, HeapID heapId);
static void PkwdCountBG_Free(PkwdCountBG *bg);
static void PkwdCountBG_Update(PkwdCountBG *bg);
static void PkwdCountBG_VBlank(PkwdCountBG *bg);
static void PkwdCountClAct_Init(PkwdCountClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void PkwdCountClAct_Free(PkwdCountClAct *clact);
static void PkwdCountClAct_Update(PkwdCountClAct *clact);
static void PkwdCountClAct_VBlank(PkwdCountClAct *clact);
static ClActUnit *PkwdCountClAct_GetUnit(PkwdCountClAct *clact);

static const BGSysLCDConfig sPkwdCountLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };
static const ClActSysSetup sPkwdCountClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

static const BGSysVRAMConfig sPkwdCountVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const PkwdCountBGSetup sPkwdCountBGSetups[BG_COUNT] = {
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
      { 0, 0, 0x2000, 0, BGRES_512x512, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x10000), 0x8000,
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
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x10000), 0x8000,
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

PkwdCountGraphic *PkwdCountGraphic_Create(u32 layout, HeapID heapId) {
    PkwdCountGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(PkwdCountGraphic), FALSE, "pkwd_count_graphic.c", 454);

    sys_memset(graphic, 0, sizeof(PkwdCountGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sPkwdCountVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    func_020232d0();
    PkwdCountBG_Init(&graphic->bg, heapId);
    PkwdCountClAct_Init(&graphic->clact, &sPkwdCountVRAMConfig, heapId);
    graphic->paletteFade = PaletteFade_Create(heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(PkwdCountGraphic_VBlank, graphic, 0);
    return graphic;
}

void PkwdCountGraphic_Free(PkwdCountGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    PkwdCountClAct_Free(&graphic->clact);
    PkwdCountBG_Free(&graphic->bg);
    PaletteFade_Free(graphic->paletteFade);
    func_020232d8();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    sys_memset(graphic, 0, sizeof(PkwdCountGraphic));
    GFL_HeapFree(graphic);
}

void PkwdCountGraphic_Update(PkwdCountGraphic *graphic) {
    PkwdCountClAct_Update(&graphic->clact);
    PkwdCountBG_Update(&graphic->bg);
}

void PkwdCountGraphic_Unused1(PkwdCountGraphic *graphic) {
}

void PkwdCountGraphic_Unused2(PkwdCountGraphic *graphic) {
}

ClActUnit *PkwdCountGraphic_GetClActUnit(PkwdCountGraphic *graphic) {
    return PkwdCountClAct_GetUnit(&graphic->clact);
}

PaletteFade *PkwdCountGraphic_GetPaletteFade(PkwdCountGraphic *graphic) {
    return graphic->paletteFade;
}

static void PkwdCountGraphic_VBlank(TCB *tcb, void *data) {
    PkwdCountGraphic *graphic = data;

    PkwdCountBG_VBlank(&graphic->bg);
    PkwdCountClAct_VBlank(&graphic->clact);
    PaletteFade_Transfer(graphic->paletteFade);
}

static void PkwdCountBG_Init(PkwdCountBG *bg, HeapID heapId) {
    u32 i;

    sys_memset(bg, 0, sizeof(PkwdCountBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sPkwdCountLCDConfig);
    for (i = 0; i < NELEMS(sPkwdCountBGSetups); i++) {
        GFL_BGSysCreateBG(sPkwdCountBGSetups[i].bg, &sPkwdCountBGSetups[i].setup, sPkwdCountBGSetups[i].mode);
        GFL_BGSysClearBG(sPkwdCountBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sPkwdCountBGSetups[i].bg, sPkwdCountBGSetups[i].enabled);
    }
}

static void PkwdCountBG_Free(PkwdCountBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sPkwdCountBGSetups); i++) {
        GFL_BGSysReleaseBG(sPkwdCountBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    sys_memset(bg, 0, sizeof(PkwdCountBG));
}

static void PkwdCountBG_Update(PkwdCountBG *bg) {
}

static void PkwdCountBG_VBlank(PkwdCountBG *bg) {
    GFL_BGSysUpdate();
}

static void PkwdCountClAct_Init(PkwdCountClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    sys_memset(clact, 0, sizeof(PkwdCountClAct));
    ClActSys_Create(&sPkwdCountClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void PkwdCountClAct_Free(PkwdCountClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    sys_memset(clact, 0, sizeof(PkwdCountClAct));
}

static void PkwdCountClAct_Update(PkwdCountClAct *clact) {
    func_0204b794();
}

static void PkwdCountClAct_VBlank(PkwdCountClAct *clact) {
    func_0204b7c8();
}

static ClActUnit *PkwdCountClAct_GetUnit(PkwdCountClAct *clact) {
    return clact->unit;
}
