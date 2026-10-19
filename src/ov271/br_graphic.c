// The Battle Recorder's graphics system: the BG system and its seven BGs, the cell actor system and the VBlank task
// that transfers both. The name is the ROM's string, from GFL_HeapAllocate's asserts

#include "types.h"
#include "app/battle_recorder/br_graphic.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "system/gf_font.h"

#define BR_GRAPHIC_BG_NUM 7

// A BG the Battle Recorder sets up
typedef struct {
    u32 frame;
    BGSetup setup;
    u32 mode;
    BOOL isVisible;
} BrGraphicBGData;

typedef struct {
    u32 dummy;
} BrGraphicBG;

typedef struct {
    ClActUnit *unit;
} BrGraphicOBJ;

struct BrGraphic {
    BrGraphicBG bg;
    BrGraphicOBJ obj;
    // Unused here; the slot of a 3D system in the other apps' graphics works
    u32 unk8;
    TCB *vblankTask;
};

static void BrGraphic_VBlankTask(TCB *tcb, void *p_wk_adrs);
static void BrGraphic_BG_Init(BrGraphicBG *p_wk, HeapID heapId);
static void BrGraphic_BG_Exit(BrGraphicBG *p_wk);
static void BrGraphic_BG_Main(BrGraphicBG *p_wk);
static void BrGraphic_BG_VBlank(BrGraphicBG *p_wk);
static void BrGraphic_OBJ_Init(BrGraphicOBJ *p_wk, const BGSysVRAMConfig *cp_vram, HeapID heapId);
static void BrGraphic_OBJ_Exit(BrGraphicOBJ *p_wk);
static void BrGraphic_OBJ_Main(BrGraphicOBJ *p_wk);
static void BrGraphic_OBJ_VBlank(BrGraphicOBJ *p_wk);
static ClActUnit *BrGraphic_OBJ_GetClunit(const BrGraphicOBJ *p_wk);

static const BGSysLCDConfig sc_bgsys_header = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };
const BGSysLCDConfig BR_GRAPHIC_LCD_CONFIG_3D = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };

static const ClActSysSetup sc_clsys_init = {
    0, 0, 0, 512, 4, 124, 4, 124, 0, 64, 32, 64, 64, 16, 16,
};

// The VRAM banks of the Battle Recorder, and of its musical photos, which use 3D
static const BGSysVRAMConfig sc_vram_bank = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,      GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};
static const BGSysVRAMConfig sc_vram_bank_musical = {
    GX_VRAM_BG_128_D,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_32_FG, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_01_AB, GX_VRAM_TEXPLTT_0123_E,  GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

// The BGs: the main engine's four and the sub engine's first three
static const BrGraphicBGData sc_bgcnt_data[BR_GRAPHIC_BG_NUM] = {
    { 0,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x08000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 2,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 3,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x14000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 5,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x08000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 6,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

BrGraphic *BrGraphic_Init(u32 type, u32 displayLayout, HeapID heapId) {
    BrGraphic *p_wk = GFL_HeapAllocate(heapId, sizeof(BrGraphic), FALSE, "br_graphic.c", 469);
    const BGSysVRAMConfig *cp_vram;

    MI_CpuFill8(p_wk, 0, sizeof(BrGraphic));
    switch (type) {
    case BR_GRAPHIC_TYPE_NORMAL:
        cp_vram = &sc_vram_bank;
        break;
    case BR_GRAPHIC_TYPE_MUSICAL:
        cp_vram = &sc_vram_bank_musical;
        break;
    }

    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(0);
    GFL_BGSysSetVRAMBanks(cp_vram);
    GFL_BGSysSetDisplayLayout(displayLayout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GFL_TextRndInitColorIndexLUT();

    BrGraphic_BG_Init(&p_wk->bg, heapId);
    BrGraphic_OBJ_Init(&p_wk->obj, cp_vram, heapId);
    p_wk->vblankTask = GFL_VBlankTCBAdd(BrGraphic_VBlankTask, p_wk, 0);
    return p_wk;
}

void BrGraphic_Exit(BrGraphic *p_wk) {
    GFL_TCBRemove(p_wk->vblankTask);
    BrGraphic_OBJ_Exit(&p_wk->obj);
    BrGraphic_BG_Exit(&p_wk->bg);
    GFL_TextRndResetColorIndexLUT();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(p_wk, 0, sizeof(BrGraphic));
    GFL_HeapFree(p_wk);
}

void BrGraphic_Main(BrGraphic *p_wk) {
    BrGraphic_OBJ_Main(&p_wk->obj);
    BrGraphic_BG_Main(&p_wk->bg);
}

ClActUnit *BrGraphic_GetClunit(const BrGraphic *p_wk) {
    return BrGraphic_OBJ_GetClunit(&p_wk->obj);
}

static void BrGraphic_VBlankTask(TCB *tcb, void *p_wk_adrs) {
    BrGraphic *p_wk = p_wk_adrs;

    BrGraphic_BG_VBlank(&p_wk->bg);
    BrGraphic_OBJ_VBlank(&p_wk->obj);
}

static void BrGraphic_BG_Init(BrGraphicBG *p_wk, HeapID heapId) {
    u32 i;

    MI_CpuFill8(p_wk, 0, sizeof(BrGraphicBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sc_bgsys_header);
    for (i = 0; i < BR_GRAPHIC_BG_NUM; i++) {
        GFL_BGSysCreateBG(sc_bgcnt_data[i].frame, &sc_bgcnt_data[i].setup, sc_bgcnt_data[i].mode);
        GFL_BGSysClearBG(sc_bgcnt_data[i].frame);
        GFL_BGSysSetBGEnabled(sc_bgcnt_data[i].frame, sc_bgcnt_data[i].isVisible);
    }
}

static void BrGraphic_BG_Exit(BrGraphicBG *p_wk) {
    u32 i;

    for (i = 0; i < BR_GRAPHIC_BG_NUM; i++) {
        GFL_BGSysReleaseBG(sc_bgcnt_data[i].frame);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(p_wk, 0, sizeof(BrGraphicBG));
}

static void BrGraphic_BG_Main(BrGraphicBG *p_wk) {
}

static void BrGraphic_BG_VBlank(BrGraphicBG *p_wk) {
    GFL_BGSysUpdate();
}

static void BrGraphic_OBJ_Init(BrGraphicOBJ *p_wk, const BGSysVRAMConfig *cp_vram, HeapID heapId) {
    MI_CpuFill8(p_wk, 0, sizeof(BrGraphicOBJ));
    ClActSys_Create(&sc_clsys_init, cp_vram, heapId);
    p_wk->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(p_wk->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void BrGraphic_OBJ_Exit(BrGraphicOBJ *p_wk) {
    func_0204bf98(p_wk->unit);
    func_0204b758();
    MI_CpuFill8(p_wk, 0, sizeof(BrGraphicOBJ));
}

static void BrGraphic_OBJ_Main(BrGraphicOBJ *p_wk) {
    func_0204b794();
}

static void BrGraphic_OBJ_VBlank(BrGraphicOBJ *p_wk) {
    func_0204b7c8();
}

static ClActUnit *BrGraphic_OBJ_GetClunit(const BrGraphicOBJ *p_wk) {
    return p_wk->unit;
}

// Releases the main engine's BGs and makes BG0 3D, for the musical photos
void BrGraphic_StartMain3D(BrGraphic *p_wk, HeapID heapId) {
    u32 i;

    for (i = 0; i < BR_GRAPHIC_BG_NUM; i++) {
        if (sc_bgcnt_data[i].frame < BGSYS_BG_SUB) {
            GFL_BGSysReleaseBG(sc_bgcnt_data[i].frame);
        }
    }
    GX_SetGraphicsMode(GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BG0_AS_3D);
}

void BrGraphic_EndMain3D(BrGraphic *p_wk, HeapID heapId) {
    u32 i;

    G2_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GX_SetGraphicsMode(GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BG0_AS_2D);
    for (i = 0; i < BR_GRAPHIC_BG_NUM; i++) {
        if (sc_bgcnt_data[i].frame < BGSYS_BG_SUB) {
            GFL_BGSysCreateBG(sc_bgcnt_data[i].frame, &sc_bgcnt_data[i].setup, sc_bgcnt_data[i].mode);
            GFL_BGSysClearBG(sc_bgcnt_data[i].frame);
        }
        GFL_BGSysSetBGEnabled(sc_bgcnt_data[i].frame, sc_bgcnt_data[i].isVisible);
    }
}
