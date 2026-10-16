#include "app/comm_tvt/ctvt_game_cam_graphic.h"
#include "types.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/gx.h"
#include "system/gf_font.h"

// The BGs and cell actors of the camera game's screens: all eight BGs, created at once

#define BG_COUNT 8

typedef struct {
    u32 unk0;
} CtvtGameCamBG;

typedef struct {
    ClActUnit *unit;
} CtvtGameCamClAct;

struct CtvtGameCamGraphic {
    CtvtGameCamBG bg;
    CtvtGameCamClAct clact;
    u32 unk8;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} CtvtGameCamBGSetup;

static void CtvtGameCamBG_Init(CtvtGameCamBG *bg, HeapID heapId);
static void CtvtGameCamBG_Free(CtvtGameCamBG *bg);
static void CtvtGameCamClAct_Init(CtvtGameCamClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void CtvtGameCamClAct_Free(CtvtGameCamClAct *clact);
static ClActUnit *CtvtGameCamClAct_GetUnit(CtvtGameCamClAct *clact);

static const BGSysLCDConfig sCtvtGameCamLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_3, GX_BGMODE_3, GX_BG0_AS_2D };
static const ClActSysSetup sCtvtGameCamClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

static const BGSysVRAMConfig sCtvtGameCamVRAMConfig = {
    GX_VRAM_BG_256_AB, GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_64_E,  GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static const CtvtGameCamBGSetup sCtvtGameCamBGSetups[BG_COUNT] = {
    { 0,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x0c000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 2,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1800), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      FALSE },
    { 3,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x00000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_AFFINE,
      TRUE },
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x18000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      FALSE },
    { 5,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x08000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      FALSE },
    { 6,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1800), GX_BG_CHARBASE(0x0c000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      FALSE },
    { 7,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x00000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_AFFINE,
      TRUE },
};

CtvtGameCamGraphic *CtvtGameCamGraphic_Create(u32 layout, HeapID heapId) {
    CtvtGameCamGraphic *graphic =
        GFL_HeapAllocate(heapId, sizeof(CtvtGameCamGraphic), FALSE, "ctvt_game_cam_graphic.c", 436);

    MI_CpuFill8(graphic, 0, sizeof(CtvtGameCamGraphic));
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sCtvtGameCamVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    func_020232d0();
    CtvtGameCamBG_Init(&graphic->bg, heapId);
    CtvtGameCamClAct_Init(&graphic->clact, &sCtvtGameCamVRAMConfig, heapId);
    return graphic;
}

void CtvtGameCamGraphic_Free(CtvtGameCamGraphic *graphic) {
    CtvtGameCamClAct_Free(&graphic->clact);
    CtvtGameCamBG_Free(&graphic->bg);
    func_020232d8();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(CtvtGameCamGraphic));
    GFL_HeapFree(graphic);
}

ClActUnit *CtvtGameCamGraphic_GetClActUnit(CtvtGameCamGraphic *graphic) {
    return CtvtGameCamClAct_GetUnit(&graphic->clact);
}

static void CtvtGameCamBG_Init(CtvtGameCamBG *bg, HeapID heapId) {
    u32 i;

    MI_CpuFill8(bg, 0, sizeof(CtvtGameCamBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sCtvtGameCamLCDConfig);
    for (i = 0; i < NELEMS(sCtvtGameCamBGSetups); i++) {
        GFL_BGSysCreateBG(sCtvtGameCamBGSetups[i].bg, &sCtvtGameCamBGSetups[i].setup, sCtvtGameCamBGSetups[i].mode);
        GFL_BGSysClearBG(sCtvtGameCamBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sCtvtGameCamBGSetups[i].bg, sCtvtGameCamBGSetups[i].enabled);
    }
}

static void CtvtGameCamBG_Free(CtvtGameCamBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sCtvtGameCamBGSetups); i++) {
        GFL_BGSysReleaseBG(sCtvtGameCamBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(CtvtGameCamBG));
}

static void CtvtGameCamClAct_Init(CtvtGameCamClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(CtvtGameCamClAct));
    ClActSys_Create(&sCtvtGameCamClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(16, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void CtvtGameCamClAct_Free(CtvtGameCamClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    MI_CpuFill8(clact, 0, sizeof(CtvtGameCamClAct));
}

static ClActUnit *CtvtGameCamClAct_GetUnit(CtvtGameCamClAct *clact) {
    return clact->unit;
}
