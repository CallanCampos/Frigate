// The ROM names this file zukan_toroku_graphic.c.
#include "types.h"
#include "app/pokedex_registration_graphic.h"
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
} PokedexRegistrationBG;

typedef struct {
    ClActUnit *unit;
} PokedexRegistrationClAct;

struct PokedexRegistrationGraphic {
    PokedexRegistrationBG bg;
    PokedexRegistrationClAct clact;
    // The 3D system of the template that this is copied from, which this does not use
    u32 unused8;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} PokedexRegistrationBGSetup;

static void PokedexRegistrationGraphic_VBlank(TCB *tcb, void *data);
static void PokedexRegistrationBG_Init(PokedexRegistrationBG *bg, HeapID heapId);
static void PokedexRegistrationBG_Free(PokedexRegistrationBG *bg);
static void PokedexRegistrationBG_Update(PokedexRegistrationBG *bg);
static void PokedexRegistrationBG_VBlank(PokedexRegistrationBG *bg);
static void PokedexRegistrationClAct_Init(PokedexRegistrationClAct *clact, const BGSysVRAMConfig *vramConfig,
                                          HeapID heapId);
static void PokedexRegistrationClAct_Free(PokedexRegistrationClAct *clact);
static void PokedexRegistrationClAct_Update(PokedexRegistrationClAct *clact);
static void PokedexRegistrationClAct_VBlank(PokedexRegistrationClAct *clact);
static ClActUnit *PokedexRegistrationClAct_GetUnit(PokedexRegistrationClAct *clact);

static const BGSysLCDConfig sPokedexRegistrationLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0,
                                                              GX_BG0_AS_2D };
static const ClActSysSetup sPokedexRegistrationClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

static const BGSysVRAMConfig sPokedexRegistrationVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,        GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_D,   GX_VRAM_TEXPLTT_0_F,     GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const PokedexRegistrationBGSetup sPokedexRegistrationBGSetups[BG_COUNT] = {
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

PokedexRegistrationGraphic *PokedexRegistrationGraphic_Create(u32 layout, HeapID heapId) {
    PokedexRegistrationGraphic *graphic =
        GFL_HeapAllocate(heapId, sizeof(PokedexRegistrationGraphic), FALSE, "zukan_toroku_graphic.c", 455);

    MI_CpuFill8(graphic, 0, sizeof(PokedexRegistrationGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sPokedexRegistrationVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GFL_TextRndInitColorIndexLUT();
    PokedexRegistrationBG_Init(&graphic->bg, heapId);
    PokedexRegistrationClAct_Init(&graphic->clact, &sPokedexRegistrationVRAMConfig, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(PokedexRegistrationGraphic_VBlank, graphic, 0);
    return graphic;
}

void PokedexRegistrationGraphic_Free(PokedexRegistrationGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    PokedexRegistrationClAct_Free(&graphic->clact);
    PokedexRegistrationBG_Free(&graphic->bg);
    GFL_TextRndResetColorIndexLUT();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    MI_CpuFill8(graphic, 0, sizeof(PokedexRegistrationGraphic));
    GFL_HeapFree(graphic);
}

void PokedexRegistrationGraphic_Update(PokedexRegistrationGraphic *graphic) {
    PokedexRegistrationClAct_Update(&graphic->clact);
    PokedexRegistrationBG_Update(&graphic->bg);
}

void PokedexRegistrationGraphic_Begin3D(PokedexRegistrationGraphic *graphic) {
}

void PokedexRegistrationGraphic_End3D(PokedexRegistrationGraphic *graphic) {
}

ClActUnit *PokedexRegistrationGraphic_GetClActUnit(PokedexRegistrationGraphic *graphic) {
    return PokedexRegistrationClAct_GetUnit(&graphic->clact);
}

static void PokedexRegistrationGraphic_VBlank(TCB *tcb, void *data) {
    PokedexRegistrationGraphic *graphic = data;

    PokedexRegistrationBG_VBlank(&graphic->bg);
    PokedexRegistrationClAct_VBlank(&graphic->clact);
}

static void PokedexRegistrationBG_Init(PokedexRegistrationBG *bg, HeapID heapId) {
    u32 i;

    MI_CpuFill8(bg, 0, sizeof(PokedexRegistrationBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sPokedexRegistrationLCDConfig);
    for (i = 0; i < NELEMS(sPokedexRegistrationBGSetups); i++) {
        GFL_BGSysCreateBG(sPokedexRegistrationBGSetups[i].bg, &sPokedexRegistrationBGSetups[i].setup,
                          sPokedexRegistrationBGSetups[i].mode);
        GFL_BGSysClearBG(sPokedexRegistrationBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sPokedexRegistrationBGSetups[i].bg, sPokedexRegistrationBGSetups[i].enabled);
    }
}

static void PokedexRegistrationBG_Free(PokedexRegistrationBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sPokedexRegistrationBGSetups); i++) {
        GFL_BGSysReleaseBG(sPokedexRegistrationBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    MI_CpuFill8(bg, 0, sizeof(PokedexRegistrationBG));
}

static void PokedexRegistrationBG_Update(PokedexRegistrationBG *bg) {
}

static void PokedexRegistrationBG_VBlank(PokedexRegistrationBG *bg) {
    GFL_BGSysUpdate();
}

static void PokedexRegistrationClAct_Init(PokedexRegistrationClAct *clact, const BGSysVRAMConfig *vramConfig,
                                          HeapID heapId) {
    MI_CpuFill8(clact, 0, sizeof(PokedexRegistrationClAct));
    ClActSys_Create(&sPokedexRegistrationClActSetup, vramConfig, heapId);
    clact->unit = ClActUnit_Create(128, 0, heapId);
    ClActUnit_UseSystemRenderer(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void PokedexRegistrationClAct_Free(PokedexRegistrationClAct *clact) {
    ClActUnit_Delete(clact->unit);
    ClActSys_Delete();
    MI_CpuFill8(clact, 0, sizeof(PokedexRegistrationClAct));
}

static void PokedexRegistrationClAct_Update(PokedexRegistrationClAct *clact) {
    ClActSys_Draw();
}

static void PokedexRegistrationClAct_VBlank(PokedexRegistrationClAct *clact) {
    ClActSys_VBlank();
}

static ClActUnit *PokedexRegistrationClAct_GetUnit(PokedexRegistrationClAct *clact) {
    return clact->unit;
}
