#include "types.h"
#include "app/local_tvt_local.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "system/gf_font.h"

// The BGs and cell actors of the Xtransceiver's calls with the game's characters, named after the ROM's
// "local_tvt_graphic.c". It is the graphic template of the applications without the 3D part

#define BG_COUNT 8

typedef struct {
    u32 unk0;
} LocalTvtGraphicBG;

typedef struct {
    ClActUnit *unit;
} LocalTvtGraphicClAct;

struct LocalTvtGraphic {
    LocalTvtGraphicBG bg;
    LocalTvtGraphicClAct clact;
    u32 unk8;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} LocalTvtBGSetup;

static void LocalTvtGraphic_VBlank(TCB *tcb, void *data);
static void LocalTvtGraphicBG_Init(LocalTvtGraphicBG *bg, HeapID heapId);
static void LocalTvtGraphicBG_Free(LocalTvtGraphicBG *bg);
static void LocalTvtGraphicBG_Update(LocalTvtGraphicBG *bg);
static void LocalTvtGraphicBG_VBlank(LocalTvtGraphicBG *bg);
static void LocalTvtGraphicClAct_Init(LocalTvtGraphicClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void LocalTvtGraphicClAct_Free(LocalTvtGraphicClAct *clact);
static void LocalTvtGraphicClAct_Update(LocalTvtGraphicClAct *clact);
static void LocalTvtGraphicClAct_VBlank(LocalTvtGraphicClAct *clact);
static ClActUnit *LocalTvtGraphicClAct_GetUnit(LocalTvtGraphicClAct *clact);

static const BGSysLCDConfig sLocalTvtLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_5, GX_BGMODE_0, GX_BG0_AS_2D };
static const ClActSysSetup sLocalTvtClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

static const BGSysVRAMConfig sLocalTvtVRAMConfig = {
    GX_VRAM_A | GX_VRAM_D, GX_VRAM_BGEXTPLTT_23_G,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B,     GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,        GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,      GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

// BG0 to BG3 of the upper screen, of which BG2 and BG3 show the faces, then those of the lower screen
static const LocalTvtBGSetup sLocalTvtBGSetups[BG_COUNT] = {
    { 0,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x7800), GX_BG_CHARBASE(0x8000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x6000), GX_BG_CHARBASE(0), 0x6000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 2,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_256, GX_BG_SCRBASE(0x6800), GX_BG_CHARBASE(0x10000), 0xc000,
        GX_BG_EXTPLTT_23, 1, GX_BG_AREAOVER_REPEAT, FALSE },
      BGMODE_EXTENDED,
      TRUE },
    { 3,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_256, GX_BG_SCRBASE(0x7000), GX_BG_CHARBASE(0x28000), 0xc000,
        GX_BG_EXTPLTT_23, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_EXTENDED,
      TRUE },
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x5000), GX_BG_CHARBASE(0x18000), 0x8000,
        GX_BG_EXTPLTT_23, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 5,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x6000), GX_BG_CHARBASE(0), 0x5000,
        GX_BG_EXTPLTT_23, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 6,
      { 0, 0, 0x1000, 0, BGRES_256x512, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x6800), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_23, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 7,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x7800), GX_BG_CHARBASE(0x8000), 0x8000,
        GX_BG_EXTPLTT_23, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

LocalTvtGraphic *LocalTvtGraphic_Create(u32 layout, HeapID heapId) {
    LocalTvtGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(LocalTvtGraphic), FALSE, "local_tvt_graphic.c", 443);

    sys_memset(graphic, 0, sizeof(LocalTvtGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sLocalTvtVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    func_020232d0();
    LocalTvtGraphicBG_Init(&graphic->bg, heapId);
    LocalTvtGraphicClAct_Init(&graphic->clact, &sLocalTvtVRAMConfig, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(LocalTvtGraphic_VBlank, graphic, 0);
    return graphic;
}

void LocalTvtGraphic_Free(LocalTvtGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    LocalTvtGraphicClAct_Free(&graphic->clact);
    LocalTvtGraphicBG_Free(&graphic->bg);
    func_020232d8();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    sys_memset(graphic, 0, sizeof(LocalTvtGraphic));
    GFL_HeapFree(graphic);
}

void LocalTvtGraphic_Update(LocalTvtGraphic *graphic) {
    LocalTvtGraphicClAct_Update(&graphic->clact);
    LocalTvtGraphicBG_Update(&graphic->bg);
}

ClActUnit *LocalTvtGraphic_GetClActUnit(LocalTvtGraphic *graphic) {
    return LocalTvtGraphicClAct_GetUnit(&graphic->clact);
}

static void LocalTvtGraphic_VBlank(TCB *tcb, void *data) {
    LocalTvtGraphic *graphic = data;

    LocalTvtGraphicBG_VBlank(&graphic->bg);
    LocalTvtGraphicClAct_VBlank(&graphic->clact);
}

static void LocalTvtGraphicBG_Init(LocalTvtGraphicBG *bg, HeapID heapId) {
    u32 i;

    sys_memset(bg, 0, sizeof(LocalTvtGraphicBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sLocalTvtLCDConfig);
    for (i = 0; i < NELEMS(sLocalTvtBGSetups); i++) {
        GFL_BGSysCreateBG(sLocalTvtBGSetups[i].bg, &sLocalTvtBGSetups[i].setup, sLocalTvtBGSetups[i].mode);
        GFL_BGSysClearBG(sLocalTvtBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sLocalTvtBGSetups[i].bg, sLocalTvtBGSetups[i].enabled);
    }
}

static void LocalTvtGraphicBG_Free(LocalTvtGraphicBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sLocalTvtBGSetups); i++) {
        GFL_BGSysReleaseBG(sLocalTvtBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    sys_memset(bg, 0, sizeof(LocalTvtGraphicBG));
}

static void LocalTvtGraphicBG_Update(LocalTvtGraphicBG *bg) {
}

static void LocalTvtGraphicBG_VBlank(LocalTvtGraphicBG *bg) {
    GFL_BGSysUpdate();
}

static void LocalTvtGraphicClAct_Init(LocalTvtGraphicClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    sys_memset(clact, 0, sizeof(LocalTvtGraphicClAct));
    ClActSys_Create(&sLocalTvtClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(32, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void LocalTvtGraphicClAct_Free(LocalTvtGraphicClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    sys_memset(clact, 0, sizeof(LocalTvtGraphicClAct));
}

static void LocalTvtGraphicClAct_Update(LocalTvtGraphicClAct *clact) {
    func_0204b794();
}

static void LocalTvtGraphicClAct_VBlank(LocalTvtGraphicClAct *clact) {
    func_0204b7c8();
}

static ClActUnit *LocalTvtGraphicClAct_GetUnit(LocalTvtGraphicClAct *clact) {
    return clact->unit;
}
