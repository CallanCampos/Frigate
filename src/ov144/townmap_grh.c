#include "types.h"
#include "app/townmap.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "system/gf_font.h"

// The BGs and cell actors of the town map. It starts at TownMapGraphic_Create, as its sibling graphic files start at
// theirs and as the first use of its name shows; the empty VBlank task before it is townmap.c's, which adds it with
// its own work

#define ARCID_TOWNMAP_GRA 84

// The resources of the map's actors
enum {
    TOWNMAP_RES_CHARS,
    TOWNMAP_RES_CELL_ANIMS,
    TOWNMAP_RES_PLTT,
    TOWNMAP_RES_COUNT
};

typedef struct {
    u32 unk0;
} TownMapBG;

typedef struct {
    ClActUnit *units[TOWNMAP_UNIT_COUNT];
    ClActor *actors[TOWNMAP_ACTOR_COUNT];
    u32 unk14;
    u32 res[TOWNMAP_RES_COUNT];
} TownMapClAct;

struct TownMapGraphic {
    TownMapBG bg;
    TownMapClAct clact;
    TCB *vblankTask;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
} TownMapBGSetup;

static void TownMapGraphic_InitDisplay(void);
static void TownMapGraphic_VBlank(TCB *tcb, void *data);
static void TownMapBG_Init(TownMapBG *bg, HeapID heapId);
static void TownMapBG_Free(TownMapBG *bg);
static void TownMapBG_VBlank(TownMapBG *bg);
static void TownMapClAct_Init(TownMapClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void TownMapClAct_Free(TownMapClAct *clact);
static void TownMapClAct_Update(TownMapClAct *clact);
static void TownMapClAct_VBlank(TownMapClAct *clact);
static ClActor *TownMapClAct_GetActor(TownMapClAct *clact, u32 index);
static ClActUnit *TownMapClAct_GetUnit(TownMapClAct *clact, u32 index);
static u32 TownMapClAct_GetRes(TownMapClAct *clact, u32 index);

static const BGSysLCDConfig sTownMapLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_5, GX_BGMODE_0, GX_BG0_AS_2D };

static const BGSysVRAMConfig sTownMapVRAMConfig = {
    GX_VRAM_A | GX_VRAM_D, GX_VRAM_E,               GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B,     GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,        GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,      GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

// In the order of TOWNMAP_BG_*
static const TownMapBGSetup sTownMapBGSetups[TOWNMAP_BG_COUNT] = {
    { 0,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x4000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT },
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x8000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT },
    { 2,
      { 0, 0, 0x1000, 0, BGRES_256x256, GX_BG_COLORMODE_256, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0xc000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_EXTENDED },
    { 3,
      { 0, 0, 0x1000, 0, BGRES_256x256, GX_BG_COLORMODE_256, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x18000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_EXTENDED },
    { 6,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT },
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x4000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT },
    { 5,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0xc000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT },
};

TownMapGraphic *TownMapGraphic_Create(HeapID heapId) {
    TownMapGraphic *graphic = GFL_HeapAllocate(heapId, sizeof(TownMapGraphic), FALSE, "townmap_grh.c", 236);

    sys_memset(graphic, 0, sizeof(TownMapGraphic));
    TownMapGraphic_InitDisplay();
    TownMapBG_Init(&graphic->bg, heapId);
    TownMapClAct_Init(&graphic->clact, &sTownMapVRAMConfig, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(TownMapGraphic_VBlank, graphic, 1);
    return graphic;
}

void TownMapGraphic_Free(TownMapGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    TownMapClAct_Free(&graphic->clact);
    TownMapBG_Free(&graphic->bg);
    GFL_HeapFree(graphic);
}

void TownMapGraphic_Update(TownMapGraphic *graphic) {
    TownMapClAct_Update(&graphic->clact);
}

u8 TownMapGraphic_GetBG(TownMapGraphic *graphic, u32 index) {
    return sTownMapBGSetups[index].bg;
}

ClActUnit *TownMapGraphic_GetUnit(TownMapGraphic *graphic, u32 index) {
    return TownMapClAct_GetUnit(&graphic->clact, index);
}

ClActor *TownMapGraphic_GetActor(TownMapGraphic *graphic, u32 index) {
    return TownMapClAct_GetActor(&graphic->clact, index);
}

void TownMapGraphic_GetRes(TownMapGraphic *graphic, u32 *chars, u32 *cellAnims, u32 *pltt) {
    *chars = TownMapClAct_GetRes(&graphic->clact, TOWNMAP_RES_CHARS);
    *cellAnims = TownMapClAct_GetRes(&graphic->clact, TOWNMAP_RES_CELL_ANIMS);
    *pltt = TownMapClAct_GetRes(&graphic->clact, TOWNMAP_RES_PLTT);
}

static void TownMapGraphic_InitDisplay(void) {
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sTownMapVRAMConfig);
    GFL_BGSysSetDisplayLayout(0);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    func_020232d0();
}

static void TownMapGraphic_VBlank(TCB *tcb, void *data) {
    TownMapGraphic *graphic = data;

    TownMapClAct_VBlank(&graphic->clact);
    TownMapBG_VBlank(&graphic->bg);
}

static void TownMapBG_Init(TownMapBG *bg, HeapID heapId) {
    int i;
    ArcTool *arc;

    sys_memset(bg, 0, sizeof(TownMapBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sTownMapLCDConfig);
    for (i = 0; i < TOWNMAP_BG_COUNT; i++) {
        GFL_BGSysCreateBG(sTownMapBGSetups[i].bg, &sTownMapBGSetups[i].setup, sTownMapBGSetups[i].mode);
        GFL_BGSysSetBGEnabled(sTownMapBGSetups[i].bg, TRUE);
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_TOWNMAP_GRA, heapId);
    // The map's 256-color palettes, in the slots of BG3 and BG2
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, 2, 0x6000, 0x2000, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, 2, 0x4000, 0x2000, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 5, 2, 0, 0x10000, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 7, 3, 0, 0x10000, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 17, 2, 0, 0x800, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 16, 3, 0, 0x800, FALSE, heapId);
    // The upper screen's background
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, 4, 0, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 6, 5, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 15, 5, 0, 0, FALSE, heapId);
    // The font's palette, as palette 14 of both screens
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 14 * 0x20, 0x20, heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 14 * 0x20, 0x20, heapId);
    GFL_BGSysFillChar(1, 0, 1, 0);
    GFL_ArcToolFree(arc);
}

static void TownMapBG_Free(TownMapBG *bg) {
    int i;

    GFL_BGSysFreeFilledChar(1, 1, 0);
    for (i = 0; i < TOWNMAP_BG_COUNT; i++) {
        GFL_BGSysClearBG(sTownMapBGSetups[i].bg);
        GFL_BGSysReleaseBG(sTownMapBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    sys_memset(bg, 0, sizeof(TownMapBG));
}

static void TownMapBG_VBlank(TownMapBG *bg) {
    GFL_BGSysUpdate();
}

static void TownMapClAct_Init(TownMapClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    int i;
    ArcTool *arc;

    sys_memset(clact, 0, sizeof(TownMapClAct));
    {
        ClActSysSetup setup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };

        ClActSys_Create(&setup, vramConfig, heapId);
    }
    clact->units[TOWNMAP_UNIT_MAIN] = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->units[TOWNMAP_UNIT_MAIN]);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    arc = GFL_ArcSysCreateFileHandle(ARCID_TOWNMAP_GRA, heapId);
    clact->res[TOWNMAP_RES_PLTT] = func_0204bba0(arc, 2, CLACT_VRAM_MAIN, 0, heapId);
    clact->res[TOWNMAP_RES_CHARS] = func_0204b81c(arc, 8, FALSE, CLACT_VRAM_MAIN, heapId);
    clact->res[TOWNMAP_RES_CELL_ANIMS] = func_0204bde0(arc, 12, 19, heapId);
    GFL_ArcToolFree(arc);
    {
        ClActorSetup setup;

        sys_memset(&setup, 0, sizeof(ClActorSetup));
        for (i = 0; i < TOWNMAP_ACTOR_COUNT; i++) {
            clact->actors[i] =
                func_0204c040(clact->units[TOWNMAP_UNIT_MAIN], clact->res[TOWNMAP_RES_CHARS],
                              clact->res[TOWNMAP_RES_PLTT], clact->res[TOWNMAP_RES_CELL_ANIMS], &setup, 1, heapId);
        }
    }
    func_0204c488(clact->actors[TOWNMAP_ACTOR_NAME_PLATE], 0);
    func_0204c488(clact->actors[TOWNMAP_ACTOR_CURSOR], 4);
    func_0204c488(clact->actors[TOWNMAP_ACTOR_RING], 5);
    func_0204c488(clact->actors[TOWNMAP_ACTOR_PLAYER], 6);
    for (i = 0; i < TOWNMAP_ACTOR_COUNT; i++) {
        func_0204c124(clact->actors[i], FALSE);
    }
}

static void TownMapClAct_Free(TownMapClAct *clact) {
    int i;

    for (i = 0; i < TOWNMAP_ACTOR_COUNT; i++) {
        if (clact->actors[i] != NULL) {
            func_0204c108(clact->actors[i]);
        }
    }
    func_0204be64(clact->res[TOWNMAP_RES_CELL_ANIMS]);
    func_0204b98c(clact->res[TOWNMAP_RES_CHARS]);
    func_0204bcd0(clact->res[TOWNMAP_RES_PLTT]);
    if (clact->units[TOWNMAP_UNIT_MAIN] != NULL) {
        func_0204bf98(clact->units[TOWNMAP_UNIT_MAIN]);
    }
    func_0204b758();
    sys_memset(clact, 0, sizeof(TownMapClAct));
}

static void TownMapClAct_Update(TownMapClAct *clact) {
    func_0204b794();
}

static void TownMapClAct_VBlank(TownMapClAct *clact) {
    func_0204b7c8();
}

static ClActor *TownMapClAct_GetActor(TownMapClAct *clact, u32 index) {
    return clact->actors[index];
}

static ClActUnit *TownMapClAct_GetUnit(TownMapClAct *clact, u32 index) {
    return clact->units[index];
}

static u32 TownMapClAct_GetRes(TownMapClAct *clact, u32 index) {
    return clact->res[index];
}
