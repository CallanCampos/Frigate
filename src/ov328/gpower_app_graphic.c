#include "types.h"
#include "app/gpower_app_graphic.h"
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

// The BGs and cell actors of the Pass Power screen, named after the ROM's "gpower_app_graphic.c"

#define BG_COUNT 8

typedef struct {
    u32 unk0;
} GPowerAppBG;

typedef struct {
    ClActUnit *unit;
} GPowerAppClAct;

struct GPowerAppGraphic {
    GPowerAppBG bg;
    GPowerAppClAct clact;
    u32 unk8;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} GPowerAppBGSetup;

static void GPowerAppGraphic_VBlank(TCB *tcb, void *data);
static void GPowerAppBG_Init(GPowerAppBG *bg, HeapID heapId);
static void GPowerAppBG_Free(GPowerAppBG *bg);
static void GPowerAppBG_Update(GPowerAppBG *bg);
static void GPowerAppBG_VBlank(GPowerAppBG *bg);
static void GPowerAppClAct_Init(GPowerAppClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void GPowerAppClAct_Free(GPowerAppClAct *clact);
static void GPowerAppClAct_Update(GPowerAppClAct *clact);
static void GPowerAppClAct_VBlank(GPowerAppClAct *clact);
static ClActUnit *GPowerAppClAct_GetUnit(GPowerAppClAct *clact);

static const BGSysLCDConfig sGPowerAppLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };
static const ClActSysSetup sGPowerAppClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

static const BGSysVRAMConfig sGPowerAppVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const GPowerAppBGSetup sGPowerAppBGSetups[BG_COUNT] = {
    { 0,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x4000), 0x8000,
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
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1800), GX_BG_CHARBASE(0x18000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x4000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 5,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x8000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 6,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 7,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1800), GX_BG_CHARBASE(0x18000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE }
};

GPowerAppGraphic *GPowerAppGraphic_Create(u32 layout, HeapID heapId) {
    GPowerAppGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(GPowerAppGraphic), FALSE, "gpower_app_graphic.c", 450);

    sys_memset(graphic, 0, sizeof(GPowerAppGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sGPowerAppVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    func_020232d0();
    GPowerAppBG_Init(&graphic->bg, heapId);
    GPowerAppClAct_Init(&graphic->clact, &sGPowerAppVRAMConfig, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(GPowerAppGraphic_VBlank, graphic, 0);
    return graphic;
}

void GPowerAppGraphic_Free(GPowerAppGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    GPowerAppClAct_Free(&graphic->clact);
    GPowerAppBG_Free(&graphic->bg);
    func_020232d8();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    sys_memset(graphic, 0, sizeof(GPowerAppGraphic));
    GFL_HeapFree(graphic);
}

void GPowerAppGraphic_Update(GPowerAppGraphic *graphic) {
    GPowerAppClAct_Update(&graphic->clact);
    GPowerAppBG_Update(&graphic->bg);
}

void GPowerAppGraphic_Begin3D(GPowerAppGraphic *graphic) {
}

void GPowerAppGraphic_End3D(GPowerAppGraphic *graphic) {
}

ClActUnit *GPowerAppGraphic_GetClActUnit(GPowerAppGraphic *graphic) {
    return GPowerAppClAct_GetUnit(&graphic->clact);
}

static void GPowerAppGraphic_VBlank(TCB *tcb, void *data) {
    GPowerAppGraphic *graphic = data;

    GPowerAppBG_VBlank(&graphic->bg);
    GPowerAppClAct_VBlank(&graphic->clact);
}

static void GPowerAppBG_Init(GPowerAppBG *bg, HeapID heapId) {
    u32 i;

    sys_memset(bg, 0, sizeof(GPowerAppBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sGPowerAppLCDConfig);
    for (i = 0; i < NELEMS(sGPowerAppBGSetups); i++) {
        GFL_BGSysCreateBG(sGPowerAppBGSetups[i].bg, &sGPowerAppBGSetups[i].setup, sGPowerAppBGSetups[i].mode);
        GFL_BGSysClearBG(sGPowerAppBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sGPowerAppBGSetups[i].bg, sGPowerAppBGSetups[i].enabled);
    }
}

static void GPowerAppBG_Free(GPowerAppBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sGPowerAppBGSetups); i++) {
        GFL_BGSysReleaseBG(sGPowerAppBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    sys_memset(bg, 0, sizeof(GPowerAppBG));
}

static void GPowerAppBG_Update(GPowerAppBG *bg) {
}

static void GPowerAppBG_VBlank(GPowerAppBG *bg) {
    GFL_BGSysUpdate();
}

static void GPowerAppClAct_Init(GPowerAppClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    sys_memset(clact, 0, sizeof(GPowerAppClAct));
    ClActSys_Create(&sGPowerAppClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void GPowerAppClAct_Free(GPowerAppClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    sys_memset(clact, 0, sizeof(GPowerAppClAct));
}

static void GPowerAppClAct_Update(GPowerAppClAct *clact) {
    func_0204b794();
}

static void GPowerAppClAct_VBlank(GPowerAppClAct *clact) {
    func_0204b7c8();
}

static ClActUnit *GPowerAppClAct_GetUnit(GPowerAppClAct *clact) {
    return clact->unit;
}
