#include "types.h"
#include "app/win_record_graphic.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "system/gf_font.h"

// The BGs and cell actors of the Pokémon World Tournament's win record, named after the ROM's "win_record_graphic.c".
// The names are ours

typedef struct {
    u32 unused;
} WinRecordBG;

typedef struct {
    ClActUnit *unit;
} WinRecordClAct;

struct WinRecordGraphic {
    WinRecordBG bg;
    WinRecordClAct clact;
    u32 unused;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} WinRecordBGSetup;

static void WinRecordGraphic_VBlank(TCB *tcb, void *data);
static void WinRecordGraphic_BGInit(WinRecordBG *bg, HeapID heapId);
static void WinRecordGraphic_BGExit(WinRecordBG *bg);
static void WinRecordGraphic_BGMain(WinRecordBG *bg);
static void WinRecordGraphic_BGVBlank(WinRecordBG *bg);
static void WinRecordGraphic_ClActInit(WinRecordClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void WinRecordGraphic_ClActExit(WinRecordClAct *clact);
static void WinRecordGraphic_ClActMain(WinRecordClAct *clact);
static void WinRecordGraphic_ClActVBlank(WinRecordClAct *clact);
static ClActUnit *WinRecordGraphic_ClActGetUnit(WinRecordClAct *clact);

static const BGSysLCDConfig sWinRecordLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

static const ClActSysSetup sWinRecordClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

static const BGSysVRAMConfig sWinRecordVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const WinRecordBGSetup sWinRecordBGSetups[] = {
    { 0,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x08000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 2,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 3,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1800), GX_BG_CHARBASE(0x18000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x4000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 5,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x08000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 6,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 7,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1800), GX_BG_CHARBASE(0x18000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

WinRecordGraphic *WinRecordGraphic_Create(u32 layout, HeapID heapId) {
    WinRecordGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(WinRecordGraphic), FALSE, "win_record_graphic.c", 450);

    MI_CpuFill8(graphic, 0, sizeof(WinRecordGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sWinRecordVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GFL_TextRndInitColorIndexLUT();
    WinRecordGraphic_BGInit(&graphic->bg, heapId);
    WinRecordGraphic_ClActInit(&graphic->clact, &sWinRecordVRAMConfig, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(WinRecordGraphic_VBlank, graphic, 0);
    return graphic;
}

void WinRecordGraphic_Delete(WinRecordGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    WinRecordGraphic_ClActExit(&graphic->clact);
    WinRecordGraphic_BGExit(&graphic->bg);
    GFL_TextRndResetColorIndexLUT();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(WinRecordGraphic));
    GFL_HeapFree(graphic);
}

void WinRecordGraphic_Main(WinRecordGraphic *graphic) {
    WinRecordGraphic_ClActMain(&graphic->clact);
    WinRecordGraphic_BGMain(&graphic->bg);
}

void WinRecordGraphic_Begin3D(WinRecordGraphic *graphic) {
}

void WinRecordGraphic_End3D(WinRecordGraphic *graphic) {
}

ClActUnit *WinRecordGraphic_GetClActUnit(WinRecordGraphic *graphic) {
    return WinRecordGraphic_ClActGetUnit(&graphic->clact);
}

static void WinRecordGraphic_VBlank(TCB *tcb, void *data) {
    WinRecordGraphic *graphic = data;

    WinRecordGraphic_BGVBlank(&graphic->bg);
    WinRecordGraphic_ClActVBlank(&graphic->clact);
}

static void WinRecordGraphic_BGInit(WinRecordBG *bg, HeapID heapId) {
    u32 i;

    MI_CpuFill8(bg, 0, sizeof(WinRecordBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sWinRecordLCDConfig);
    for (i = 0; i < NELEMS(sWinRecordBGSetups); i++) {
        GFL_BGSysCreateBG(sWinRecordBGSetups[i].bg, &sWinRecordBGSetups[i].setup, sWinRecordBGSetups[i].mode);
        GFL_BGSysClearBG(sWinRecordBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sWinRecordBGSetups[i].bg, sWinRecordBGSetups[i].enabled);
    }
}

static void WinRecordGraphic_BGExit(WinRecordBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sWinRecordBGSetups); i++) {
        GFL_BGSysReleaseBG(sWinRecordBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(WinRecordBG));
}

static void WinRecordGraphic_BGMain(WinRecordBG *bg) {
}

static void WinRecordGraphic_BGVBlank(WinRecordBG *bg) {
    GFL_BGSysUpdate();
}

static void WinRecordGraphic_ClActInit(WinRecordClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(WinRecordClAct));
    ClActSys_Create(&sWinRecordClActSetup, vramConfig, heapId);
    clact->unit = ClActUnit_Create(128, 0, heapId);
    ClActUnit_UseSystemRenderer(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void WinRecordGraphic_ClActExit(WinRecordClAct *clact) {
    ClActUnit_Delete(clact->unit);
    ClActSys_Delete();
    MI_CpuFill8(clact, 0, sizeof(WinRecordClAct));
}

static void WinRecordGraphic_ClActMain(WinRecordClAct *clact) {
    ClActSys_Draw();
}

static void WinRecordGraphic_ClActVBlank(WinRecordClAct *clact) {
    ClActSys_VBlank();
}

static ClActUnit *WinRecordGraphic_ClActGetUnit(WinRecordClAct *clact) {
    return clact->unit;
}
