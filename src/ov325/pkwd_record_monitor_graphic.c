#include "types.h"
#include "app/pkwd_record_monitor_graphic.h"
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

// The BGs and cell actors of Pokéstar Studios' record monitor, named after the ROM's
// "pkwd_record_monitor_graphic.c"

#define BG_COUNT 8

typedef struct {
    u32 unk0;
} PkwdRecordMonitorBG;

typedef struct {
    ClActUnit *unit;
} PkwdRecordMonitorClAct;

struct PkwdRecordMonitorGraphic {
    PkwdRecordMonitorBG bg;
    PkwdRecordMonitorClAct clact;
    u32 unk8;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} PkwdRecordMonitorBGSetup;

static void PkwdRecordMonitorGraphic_VBlank(TCB *tcb, void *data);
static void PkwdRecordMonitorBG_Init(PkwdRecordMonitorBG *bg, HeapID heapId);
static void PkwdRecordMonitorBG_Free(PkwdRecordMonitorBG *bg);
static void PkwdRecordMonitorBG_Update(PkwdRecordMonitorBG *bg);
static void PkwdRecordMonitorBG_VBlank(PkwdRecordMonitorBG *bg);
static void PkwdRecordMonitorClAct_Init(PkwdRecordMonitorClAct *clact, const BGSysVRAMConfig *vramConfig,
                                        HeapID heapId);
static void PkwdRecordMonitorClAct_Free(PkwdRecordMonitorClAct *clact);
static void PkwdRecordMonitorClAct_Update(PkwdRecordMonitorClAct *clact);
static void PkwdRecordMonitorClAct_VBlank(PkwdRecordMonitorClAct *clact);
static ClActUnit *PkwdRecordMonitorClAct_GetUnit(PkwdRecordMonitorClAct *clact);

static const BGSysLCDConfig sPkwdRecordMonitorLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0,
                                                            GX_BG0_AS_2D };
static const ClActSysSetup sPkwdRecordMonitorClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

static const BGSysVRAMConfig sPkwdRecordMonitorVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static const PkwdRecordMonitorBGSetup sPkwdRecordMonitorBGSetups[BG_COUNT] = {
    { 0,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x0000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x8000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      TRUE },
    { 2,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1800), GX_BG_CHARBASE(0x8000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      FALSE },
    { 3,
      { 0, 0, 0x2000, 0, BGRES_512x512, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x8000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT,
      FALSE },
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

PkwdRecordMonitorGraphic *PkwdRecordMonitorGraphic_Create(u32 layout, HeapID heapId) {
    PkwdRecordMonitorGraphic *graphic =
        GFL_HeapAllocate(heapId, sizeof(PkwdRecordMonitorGraphic), FALSE, "pkwd_record_monitor_graphic.c", 447);

    MI_CpuFill8(graphic, 0, sizeof(PkwdRecordMonitorGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sPkwdRecordMonitorVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GFL_TextRndInitColorIndexLUT();
    PkwdRecordMonitorBG_Init(&graphic->bg, heapId);
    PkwdRecordMonitorClAct_Init(&graphic->clact, &sPkwdRecordMonitorVRAMConfig, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(PkwdRecordMonitorGraphic_VBlank, graphic, 0);
    return graphic;
}

void PkwdRecordMonitorGraphic_Free(PkwdRecordMonitorGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    PkwdRecordMonitorClAct_Free(&graphic->clact);
    PkwdRecordMonitorBG_Free(&graphic->bg);
    GFL_TextRndResetColorIndexLUT();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(PkwdRecordMonitorGraphic));
    GFL_HeapFree(graphic);
}

void PkwdRecordMonitorGraphic_Update(PkwdRecordMonitorGraphic *graphic) {
    PkwdRecordMonitorClAct_Update(&graphic->clact);
    PkwdRecordMonitorBG_Update(&graphic->bg);
}

void PkwdRecordMonitorGraphic_Begin3D(PkwdRecordMonitorGraphic *graphic) {
}

void PkwdRecordMonitorGraphic_End3D(PkwdRecordMonitorGraphic *graphic) {
}

ClActUnit *PkwdRecordMonitorGraphic_GetClActUnit(PkwdRecordMonitorGraphic *graphic) {
    return PkwdRecordMonitorClAct_GetUnit(&graphic->clact);
}

static void PkwdRecordMonitorGraphic_VBlank(TCB *tcb, void *data) {
    PkwdRecordMonitorGraphic *graphic = data;

    PkwdRecordMonitorBG_VBlank(&graphic->bg);
    PkwdRecordMonitorClAct_VBlank(&graphic->clact);
}

static void PkwdRecordMonitorBG_Init(PkwdRecordMonitorBG *bg, HeapID heapId) {
    u32 i;

    MI_CpuFill8(bg, 0, sizeof(PkwdRecordMonitorBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sPkwdRecordMonitorLCDConfig);
    for (i = 0; i < NELEMS(sPkwdRecordMonitorBGSetups); i++) {
        GFL_BGSysCreateBG(sPkwdRecordMonitorBGSetups[i].bg, &sPkwdRecordMonitorBGSetups[i].setup,
                          sPkwdRecordMonitorBGSetups[i].mode);
        GFL_BGSysClearBG(sPkwdRecordMonitorBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sPkwdRecordMonitorBGSetups[i].bg, sPkwdRecordMonitorBGSetups[i].enabled);
    }
}

static void PkwdRecordMonitorBG_Free(PkwdRecordMonitorBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sPkwdRecordMonitorBGSetups); i++) {
        GFL_BGSysReleaseBG(sPkwdRecordMonitorBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(PkwdRecordMonitorBG));
}

static void PkwdRecordMonitorBG_Update(PkwdRecordMonitorBG *bg) {
}

static void PkwdRecordMonitorBG_VBlank(PkwdRecordMonitorBG *bg) {
    GFL_BGSysUpdate();
}

static void PkwdRecordMonitorClAct_Init(PkwdRecordMonitorClAct *clact, const BGSysVRAMConfig *vramConfig,
                                        HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(PkwdRecordMonitorClAct));
    ClActSys_Create(&sPkwdRecordMonitorClActSetup, vramConfig, heapId);
    clact->unit = ClActUnit_Create(128, 0, heapId);
    ClActUnit_UseSystemRenderer(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void PkwdRecordMonitorClAct_Free(PkwdRecordMonitorClAct *clact) {
    ClActUnit_Delete(clact->unit);
    ClActSys_Delete();
    MI_CpuFill8(clact, 0, sizeof(PkwdRecordMonitorClAct));
}

static void PkwdRecordMonitorClAct_Update(PkwdRecordMonitorClAct *clact) {
    ClActSys_Draw();
}

static void PkwdRecordMonitorClAct_VBlank(PkwdRecordMonitorClAct *clact) {
    ClActSys_VBlank();
}

static ClActUnit *PkwdRecordMonitorClAct_GetUnit(PkwdRecordMonitorClAct *clact) {
    return clact->unit;
}
