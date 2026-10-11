#include "types.h"
#include "app/th_award_graphic.h"
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

// The BGs and cell actors of the Trial House's results screen, named after the ROM's "th_award_graphic.c"

typedef struct {
    u32 unk0;
} ThAwardBG;

typedef struct {
    ClActUnit *unit;
} ThAwardClAct;

struct ThAwardGraphic {
    ThAwardBG bg;
    ThAwardClAct clact;
    u32 unk8;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} ThAwardBGSetup;

static void ThAwardGraphic_VBlank(TCB *tcb, void *data);
static void ThAwardBG_Init(ThAwardBG *bg, HeapID heapId);
static void ThAwardBG_Free(ThAwardBG *bg);
static void ThAwardBG_Update(ThAwardBG *bg);
static void ThAwardBG_VBlank(ThAwardBG *bg);
static void ThAwardClAct_Init(ThAwardClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void ThAwardClAct_Free(ThAwardClAct *clact);
static void ThAwardClAct_Update(ThAwardClAct *clact);
static void ThAwardClAct_VBlank(ThAwardClAct *clact);
static ClActUnit *ThAwardClAct_GetUnit(ThAwardClAct *clact);

static const BGSysLCDConfig sThAwardLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };
static const ClActSysSetup sThAwardClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

static const BGSysVRAMConfig sThAwardVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_D,   GX_VRAM_TEXPLTT_0_F,     GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const ThAwardBGSetup sThAwardBGSetups[3] = {
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x8000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 2,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x8000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 3,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x14000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

ThAwardGraphic *ThAwardGraphic_Create(u32 layout, HeapID heapId) {
    ThAwardGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(ThAwardGraphic), FALSE, "th_award_graphic.c", 458);

    sys_memset(graphic, 0, sizeof(ThAwardGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sThAwardVRAMConfig);
    GFL_BGSysSetDisplayLayout(layout);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    func_020232d0();
    ThAwardBG_Init(&graphic->bg, heapId);
    ThAwardClAct_Init(&graphic->clact, &sThAwardVRAMConfig, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(ThAwardGraphic_VBlank, graphic, 0);
    return graphic;
}

void ThAwardGraphic_Free(ThAwardGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    ThAwardClAct_Free(&graphic->clact);
    ThAwardBG_Free(&graphic->bg);
    func_020232d8();
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    sys_memset(graphic, 0, sizeof(ThAwardGraphic));
    GFL_HeapFree(graphic);
}

void ThAwardGraphic_Update(ThAwardGraphic *graphic) {
    ThAwardClAct_Update(&graphic->clact);
    ThAwardBG_Update(&graphic->bg);
}

ClActUnit *ThAwardGraphic_GetClActUnit(ThAwardGraphic *graphic) {
    return ThAwardClAct_GetUnit(&graphic->clact);
}

static void ThAwardGraphic_VBlank(TCB *tcb, void *data) {
    ThAwardGraphic *graphic = data;

    ThAwardBG_VBlank(&graphic->bg);
    ThAwardClAct_VBlank(&graphic->clact);
}

static void ThAwardBG_Init(ThAwardBG *bg, HeapID heapId) {
    u32 i;

    sys_memset(bg, 0, sizeof(ThAwardBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sThAwardLCDConfig);
    for (i = 0; i < NELEMS(sThAwardBGSetups); i++) {
        GFL_BGSysCreateBG(sThAwardBGSetups[i].bg, &sThAwardBGSetups[i].setup, sThAwardBGSetups[i].mode);
        GFL_BGSysClearBG(sThAwardBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sThAwardBGSetups[i].bg, sThAwardBGSetups[i].enabled);
    }
}

static void ThAwardBG_Free(ThAwardBG *bg) {
    u32 i;

    for (i = 0; i < NELEMS(sThAwardBGSetups); i++) {
        GFL_BGSysReleaseBG(sThAwardBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    sys_memset(bg, 0, sizeof(ThAwardBG));
}

static void ThAwardBG_Update(ThAwardBG *bg) {
}

static void ThAwardBG_VBlank(ThAwardBG *bg) {
    GFL_BGSysUpdate();
}

static void ThAwardClAct_Init(ThAwardClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    sys_memset(clact, 0, sizeof(ThAwardClAct));
    ClActSys_Create(&sThAwardClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void ThAwardClAct_Free(ThAwardClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    sys_memset(clact, 0, sizeof(ThAwardClAct));
}

static void ThAwardClAct_Update(ThAwardClAct *clact) {
    func_0204b794();
}

static void ThAwardClAct_VBlank(ThAwardClAct *clact) {
    func_0204b7c8();
}

static ClActUnit *ThAwardClAct_GetUnit(ThAwardClAct *clact) {
    return clact->unit;
}
