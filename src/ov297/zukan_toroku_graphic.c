#include "types.h"
#include "app/zukan_toroku_graphic.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "system/gf_font.h"

// The BGs and the cell actor system of the Pokédex registration

#define BG_COUNT 4

typedef struct {
    u32 unk0;
} ZukanTorokuBG;

typedef struct {
    ClActUnit *unit;
} ZukanTorokuClAct;

struct ZukanTorokuGraphic {
    ZukanTorokuBG bg;
    ZukanTorokuClAct clact;
    // The 3D system of the template that this is copied from, which this does not use
    u32 unused8;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} ZukanTorokuBGSetup;

static void ZukanTorokuGraphic_VBlank(TCB *tcb, void *data);
static void ZukanTorokuBG_Init(ZukanTorokuBG *bg, HeapID heapId);
static void ZukanTorokuBG_Free(ZukanTorokuBG *bg);
static void ZukanTorokuBG_Update(ZukanTorokuBG *bg);
static void ZukanTorokuBG_VBlank(ZukanTorokuBG *bg);
static void ZukanTorokuClAct_Init(ZukanTorokuClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void ZukanTorokuClAct_Free(ZukanTorokuClAct *clact);
static void ZukanTorokuClAct_Update(ZukanTorokuClAct *clact);
static void ZukanTorokuClAct_VBlank(ZukanTorokuClAct *clact);
static ClActUnit *ZukanTorokuClAct_GetUnit(ZukanTorokuClAct *clact);

static const BGSysLCDConfig sZukanTorokuLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };
static const ClActSysSetup sZukanTorokuClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

static const BGSysVRAMConfig sZukanTorokuVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,        GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_D,   GX_VRAM_TEXPLTT_0_F,     GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const ZukanTorokuBGSetup sZukanTorokuBGSetups[BG_COUNT] = {
    { 0,
      { 0, 0, 0x1000, 0, BGRES_256x512, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x0c000), 0x4000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 2,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 3,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x18000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

ZukanTorokuGraphic *ZukanTorokuGraphic_Create(u32 layout, HeapID heapId) {
    ZukanTorokuGraphic *graphic =
        GFL_HeapAllocate(heapId, sizeof(ZukanTorokuGraphic), FALSE, "zukan_toroku_graphic.c", 455);

    sys_memset(graphic, 0, sizeof(ZukanTorokuGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sZukanTorokuVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    func_020232d0();
    ZukanTorokuBG_Init(&graphic->bg, heapId);
    ZukanTorokuClAct_Init(&graphic->clact, &sZukanTorokuVRAMConfig, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(ZukanTorokuGraphic_VBlank, graphic, 0);
    return graphic;
}

void ZukanTorokuGraphic_Free(ZukanTorokuGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    ZukanTorokuClAct_Free(&graphic->clact);
    ZukanTorokuBG_Free(&graphic->bg);
    func_020232d8();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    sys_memset(graphic, 0, sizeof(ZukanTorokuGraphic));
    GFL_HeapFree(graphic);
}

void ZukanTorokuGraphic_Update(ZukanTorokuGraphic *graphic) {
    ZukanTorokuClAct_Update(&graphic->clact);
    ZukanTorokuBG_Update(&graphic->bg);
}

void ZukanTorokuGraphic_Begin3D(ZukanTorokuGraphic *graphic) {
}

void ZukanTorokuGraphic_End3D(ZukanTorokuGraphic *graphic) {
}

ClActUnit *ZukanTorokuGraphic_GetClActUnit(ZukanTorokuGraphic *graphic) {
    return ZukanTorokuClAct_GetUnit(&graphic->clact);
}

static void ZukanTorokuGraphic_VBlank(TCB *tcb, void *data) {
    ZukanTorokuGraphic *graphic = data;

    ZukanTorokuBG_VBlank(&graphic->bg);
    ZukanTorokuClAct_VBlank(&graphic->clact);
}

static void ZukanTorokuBG_Init(ZukanTorokuBG *bg, HeapID heapId) {
    u32 i;

    sys_memset(bg, 0, sizeof(ZukanTorokuBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sZukanTorokuLCDConfig);
    for (i = 0; i < NELEMS(sZukanTorokuBGSetups); i++) {
        GFL_BGSysCreateBG(sZukanTorokuBGSetups[i].bg, &sZukanTorokuBGSetups[i].setup, sZukanTorokuBGSetups[i].mode);
        GFL_BGSysClearBG(sZukanTorokuBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sZukanTorokuBGSetups[i].bg, sZukanTorokuBGSetups[i].enabled);
    }
}

static void ZukanTorokuBG_Free(ZukanTorokuBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sZukanTorokuBGSetups); i++) {
        GFL_BGSysReleaseBG(sZukanTorokuBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    sys_memset(bg, 0, sizeof(ZukanTorokuBG));
}

static void ZukanTorokuBG_Update(ZukanTorokuBG *bg) {
}

static void ZukanTorokuBG_VBlank(ZukanTorokuBG *bg) {
    GFL_BGSysUpdate();
}

static void ZukanTorokuClAct_Init(ZukanTorokuClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    sys_memset(clact, 0, sizeof(ZukanTorokuClAct));
    ClActSys_Create(&sZukanTorokuClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void ZukanTorokuClAct_Free(ZukanTorokuClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    sys_memset(clact, 0, sizeof(ZukanTorokuClAct));
}

static void ZukanTorokuClAct_Update(ZukanTorokuClAct *clact) {
    func_0204b794();
}

static void ZukanTorokuClAct_VBlank(ZukanTorokuClAct *clact) {
    func_0204b7c8();
}

static ClActUnit *ZukanTorokuClAct_GetUnit(ZukanTorokuClAct *clact) {
    return clact->unit;
}
