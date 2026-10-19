#include "types.h"
#include "app/beacon_detail_gra.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "system/gf_font.h"

// The BGs and cell actors of the screen of a beacon's details, named after the ROM's "beacon_detail_gra.c"

#define BG_COUNT 8

typedef struct {
    u32 unk0;
} BeaconDetailBG;

typedef struct {
    ClActUnit *unit;
    ClActRenderer *renderer;
} BeaconDetailClAct;

struct BeaconDetailGraphic {
    BeaconDetailBG bg;
    BeaconDetailClAct clact;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} BeaconDetailBGSetup;

static void BeaconDetailGraphic_VBlank(TCB *tcb, void *data);
static void BeaconDetailBG_Init(BeaconDetailBG *bg, HeapID heapId);
static void BeaconDetailBG_Free(BeaconDetailBG *bg);
static void BeaconDetailBG_Update(BeaconDetailBG *bg);
static void BeaconDetailBG_VBlank(BeaconDetailBG *bg);
static void BeaconDetailClAct_Init(BeaconDetailClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void BeaconDetailClAct_Free(BeaconDetailClAct *clact);
static void BeaconDetailClAct_Update(BeaconDetailClAct *clact);
static void BeaconDetailClAct_VBlank(BeaconDetailClAct *clact);
static ClActUnit *BeaconDetailClAct_GetUnit(BeaconDetailClAct *clact);

static const BGSysLCDConfig sBeaconDetailLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_5, GX_BGMODE_0, GX_BG0_AS_2D };
static const ClActSysSetup sBeaconDetailClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

static const BGSysVRAMConfig sBeaconDetailVRAMConfig = {
    GX_VRAM_A | GX_VRAM_D, GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B,     GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,      GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

// The upper screen's panels scroll in from above and below, so their BGs are two screens tall
static const BeaconDetailBGSetup sBeaconDetailBGSetups[BG_COUNT] = {
    { 0,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x4000), 0x4000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x8000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 2,
      { 0, 0, 0x1000, 0, BGRES_256x256, GX_BG_COLORMODE_256, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x20000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_EXTENDED,
      TRUE },
    { 3,
      { 0, 0, 0x1000, 0, BGRES_256x256, GX_BG_COLORMODE_256, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x30000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_EXTENDED,
      TRUE },
    { 4,
      { 0, 0, 0x1000, 0, BGRES_256x512, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 5,
      { 0, 0, 0x1000, 0, BGRES_256x512, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x18000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 6,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x4000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 7,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x4000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE }
};

BeaconDetailGraphic *BeaconDetailGraphic_Create(u32 layout, HeapID heapId) {
    BeaconDetailGraphic *graphic =
        GFL_HeapAllocate(heapId, sizeof(BeaconDetailGraphic), FALSE, "beacon_detail_gra.c", 317);

    MI_CpuFill8(graphic, 0, sizeof(BeaconDetailGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sBeaconDetailVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GFL_TextRndInitColorIndexLUT();
    BeaconDetailBG_Init(&graphic->bg, heapId);
    BeaconDetailClAct_Init(&graphic->clact, &sBeaconDetailVRAMConfig, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(BeaconDetailGraphic_VBlank, graphic, 0);
    return graphic;
}

void BeaconDetailGraphic_Free(BeaconDetailGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    BeaconDetailClAct_Free(&graphic->clact);
    BeaconDetailBG_Free(&graphic->bg);
    GFL_TextRndResetColorIndexLUT();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(BeaconDetailGraphic));
    GFL_HeapFree(graphic);
}

void BeaconDetailGraphic_Update(BeaconDetailGraphic *graphic) {
    BeaconDetailClAct_Update(&graphic->clact);
    BeaconDetailBG_Update(&graphic->bg);
}

ClActUnit *BeaconDetailGraphic_GetClActUnit(BeaconDetailGraphic *graphic) {
    return BeaconDetailClAct_GetUnit(&graphic->clact);
}

static void BeaconDetailGraphic_VBlank(TCB *tcb, void *data) {
    BeaconDetailGraphic *graphic = data;

    BeaconDetailBG_VBlank(&graphic->bg);
    BeaconDetailClAct_VBlank(&graphic->clact);
}

static void BeaconDetailBG_Init(BeaconDetailBG *bg, HeapID heapId) {
    u32 i;

    MI_CpuFill8(bg, 0, sizeof(BeaconDetailBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sBeaconDetailLCDConfig);
    for (i = 0; i < NELEMS(sBeaconDetailBGSetups); i++) {
        GFL_BGSysCreateBG(sBeaconDetailBGSetups[i].bg, &sBeaconDetailBGSetups[i].setup, sBeaconDetailBGSetups[i].mode);
        GFL_BGSysClearBG(sBeaconDetailBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sBeaconDetailBGSetups[i].bg, sBeaconDetailBGSetups[i].enabled);
    }
}

static void BeaconDetailBG_Free(BeaconDetailBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sBeaconDetailBGSetups); i++) {
        GFL_BGSysReleaseBG(sBeaconDetailBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(BeaconDetailBG));
}

static void BeaconDetailBG_Update(BeaconDetailBG *bg) {
}

static void BeaconDetailBG_VBlank(BeaconDetailBG *bg) {
    GFL_BGSysUpdate();
}

// The unit draws through a renderer of its own, whose sub screen's surface is the default one's: the screens'
// actors are positioned on a surface
static void BeaconDetailClAct_Init(BeaconDetailClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(BeaconDetailClAct));
    ClActSys_Create(&sBeaconDetailClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    {
        ClActSurfaceSetup surfaces[] = {
            { 0, 0, 256, 192, 0, 0 },
            { 0, 512, 256, 192, 1, 0 },
        };

        clact->renderer = func_0204be9c(surfaces, NELEMS(surfaces), heapId);
    }
    func_0204bf14(clact->renderer, TRUE);
    func_0204c018(clact->unit, clact->renderer);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void BeaconDetailClAct_Free(BeaconDetailClAct *clact) {
    func_0204becc(clact->renderer);
    func_0204bf98(clact->unit);
    func_0204b758();
    MI_CpuFill8(clact, 0, sizeof(BeaconDetailClAct));
}

static void BeaconDetailClAct_Update(BeaconDetailClAct *clact) {
    func_0204b794();
}

static void BeaconDetailClAct_VBlank(BeaconDetailClAct *clact) {
    func_0204b7c8();
}

static ClActUnit *BeaconDetailClAct_GetUnit(BeaconDetailClAct *clact) {
    return clact->unit;
}
