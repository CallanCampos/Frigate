// GTS Negotiations' display: the BGs and actors of its screens. The lower screen has the main menu, the search's
// settings with their arrows, or the list of the people of the Pal Pad, which scrolls, with their icons and a scroll
// bar; the top screen shows the search and the partner found. The name is the ROM's string, from GFL_HeapAllocate's
// call. Function names are ours.

#include "types.h"
#include "app/gtsnego_local.h"
#include "app/ui/touchbar.h"
#include "app/ui/ui_scene.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "nnsys/g2d.h"
#include "nnsys/gfd.h"
#include "save/player_info.h"
#include "system/app_taskmenu.h"
#include "system/blink_palanm.h"
#include "system/bmp_winframe.h"
#include "system/palanm.h"

// The archives of the screens' graphics and of the people's icons, which have no names yet
#define GTSNEGO_ARCID 0x90
#define GTSNEGO_ARCID_AVATAR 0x1f

#define GTSNEGO_AVATAR_COUNT 16
#define GTSNEGO_ARROW_COUNT 12

// The tiles of a row of the list of people, and what the list scrolls by a frame
#define GTSNEGO_LIST_ROW_HEIGHT 6
#define GTSNEGO_LIST_SCROLL_SPEED 8

// The scroll bar: where its knob starts, and how far it moves
#define GTSNEGO_KNOB_TOP 26
#define GTSNEGO_KNOB_RANGE 132

// The actors' resources
enum {
    GTSNEGO_RES_PLTT_SUB,
    GTSNEGO_RES_PLTT_MAIN,
    GTSNEGO_RES_PLTT_AVATAR,
    GTSNEGO_RES_PLTT_FOOTER,
    GTSNEGO_RES_CHAR_SUB,
    GTSNEGO_RES_CHAR_MAIN,
    GTSNEGO_RES_CHAR_AVATAR,
    GTSNEGO_RES_CHAR_FOOTER = GTSNEGO_RES_CHAR_AVATAR + GTSNEGO_AVATAR_COUNT,
    GTSNEGO_RES_ANIM_SUB,
    GTSNEGO_RES_ANIM_MAIN,
    GTSNEGO_RES_ANIM_AVATAR,
    GTSNEGO_RES_ANIM_FOOTER = GTSNEGO_RES_ANIM_AVATAR + GTSNEGO_AVATAR_COUNT,
    GTSNEGO_RES_UNUSED,
    GTSNEGO_RES_COUNT,
};

// The actors of the list of people: the scroll bar's knob and the two that are always there
enum {
    GTSNEGO_LIST_ACTOR_KNOB,
    GTSNEGO_LIST_ACTOR_BAR,
    GTSNEGO_LIST_ACTOR_HEADER,
    GTSNEGO_LIST_ACTOR_COUNT,
};

struct GtsNegoDisp {
    // The characters of BG 4, BG 0 and BG 7
    u32 subChars;
    u32 mainChars;
    u32 barChars;
    TouchBar *touchBar;
    ClActUnit *clUnit;
    TCB *vblankTask;
    u32 res[GTSNEGO_RES_COUNT];
    ClActor *arrows[GTSNEGO_ARROW_COUNT];
    HeapID heapId;
    BlinkPalAnm *blinkMain;
    BlinkPalAnm *blinkSub;
    // The list of people is scrolled in the V-blank: where it is, the way it goes and where it stops
    BOOL scrollPending;
    int listScroll;
    int listScrollDir;
    int listScrollTarget;
    // The top screen's BG 2 scrolls while it searches
    int topScroll;
    BOOL topScrolling;
    GameData *gameData;
    // The screen of a row of the list, in its file
    NNSG2dScreenData *listScreen;
    void *listScreenFile;
    // The lower screen's palettes, kept while they are dimmed
    void *objPalette;
    void *bgPalette;
    ClActor *searchActors[2];
    ClActor *cursor;
    ClActor *listFooter;
    u32 unk13C;
    ClActor *listActors[GTSNEGO_LIST_ACTOR_COUNT];
    ClActor *friendIcons[GTSNEGO_LIST_ROWS];
};

// Where the cursor goes for a GTSNEGO_CURSOR_*, with its animation there and that of a press
typedef struct {
    u8 anim;
    u8 x;
    u8 y;
    u8 pressAnim;
} GtsNegoCursorPos;

static void GtsNegoDisp_CreateBGs(GtsNegoDisp *disp);
static void GtsNegoDisp_DimPalettes(GtsNegoDisp *disp, u32 vram, int *starts, int *counts, int count);
static void GtsNegoDisp_DimPalettesVBlank(GtsNegoDisp *disp, u32 vram, int *starts, int *counts, int count);
static void GtsNegoDisp_VBlank(TCB *tcb, void *work);
static void GtsNegoDisp_LoadGraphics(GtsNegoDisp *disp);
static void GtsNegoDisp_CreateCursor(GtsNegoDisp *disp);
static void GtsNegoDisp_AddArrow(GtsNegoDisp *disp, int x, int y, BOOL right);
static void GtsNegoDisp_DrawListRow(GtsNegoDisp *disp, int row);
static void GtsNegoDisp_ClearListRow(GtsNegoDisp *disp, int row);
static void GtsNegoDisp_CreateListFooter(GtsNegoDisp *disp);
static void GtsNegoDisp_CreateListActors(GtsNegoDisp *disp, BOOL scrollable);
static int GtsNegoDisp_GetKnobY(GtsNegoDisp *disp, int pos, int count);
static int GtsNegoDisp_GetKnobPos(GtsNegoDisp *disp, int y, int count);
static void GtsNegoDisp_OnSearchAnimEnd(u32 param, fx32 frame);
static void GtsNegoDisp_CreateSearchActors(GtsNegoDisp *disp);
static void GtsNegoDisp_FreeSearchActors(GtsNegoDisp *disp);
static void GtsNegoDisp_UpdateTopScroll(GtsNegoDisp *disp);

static const ClActSysSetup sGtsNegoDispClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 100, 100, 100, 100, 16, 16 };

// The sub screen's OBJ extended palettes are in bank I (the SDK's GX_VRAM_SUB_OBJEXTPLTT_0_I)
static BGSysVRAMConfig sGtsNegoDispVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_I,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static BGSysLCDConfig sGtsNegoDispLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

GtsNegoDisp *GtsNegoDisp_Create(HeapID heapId, GameData *gameData) {
    GtsNegoDisp *disp = GFL_HeapAllocate(heapId, sizeof(GtsNegoDisp), TRUE, "gtsnego_disp.c", 258);

    disp->heapId = heapId;
    disp->gameData = gameData;
    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    GFL_BGSysSetDisplayLayout(1);
    GXS_DispOn();
    gfxEngineEnableA();
    GFL_BGSysCreate(disp->heapId);
    ClActSys_Create(&sGtsNegoDispClActSetup, &sGtsNegoDispVRAMConfig, disp->heapId);
    disp->clUnit = func_0204bf1c(64, 0, disp->heapId);
    GFL_BGSysSetVRAMBanks(&sGtsNegoDispVRAMConfig);
    GFL_BGSysSetLCDConfig(&sGtsNegoDispLCDConfig);
    GtsNegoDisp_CreateBGs(disp);
    GtsNegoDisp_LoadGraphics(disp);
    disp->vblankTask = GFL_VBlankTCBAdd(GtsNegoDisp_VBlank, disp, 0);
    disp->objPalette = GFL_HeapAllocate(disp->heapId, 0x200, FALSE, "gtsnego_disp.c", 280);
    disp->bgPalette = GFL_HeapAllocate(disp->heapId, 0x200, FALSE, "gtsnego_disp.c", 281);
    GtsNegoDisp_CreateCursor(disp);
    GtsNegoDisp_CreateTouchBar(disp, 0);
    GFL_BGSysSetEnabledBGsB(GX_PLANEMASK_ALL);
    GFL_BGSysSetEnabledBGsA(GX_PLANEMASK_ALL);
    return disp;
}

void GtsNegoDisp_Main(GtsNegoDisp *disp) {
    func_0204b794();
    if (disp->blinkMain != NULL) {
        BlinkPalAnm_Main(disp->blinkMain);
    }
    if (disp->blinkSub != NULL) {
        BlinkPalAnm_Main(disp->blinkSub);
    }
    GtsNegoDisp_UpdateTopScroll(disp);
}

void GtsNegoDisp_Free(GtsNegoDisp *disp) {
    int i;

    func_0204c108(disp->cursor);
    GtsNegoDisp_FreeTouchBar(disp);
    GtsNegoDisp_FreeListFooter(disp);
    GtsNegoDisp_FreeListScreen(disp);
    GFL_HeapFree(disp->objPalette);
    GFL_HeapFree(disp->bgPalette);
    GtsNegoDisp_FreeArrows(disp);
    func_0204bcd0(disp->res[GTSNEGO_RES_PLTT_SUB]);
    func_0204bcd0(disp->res[GTSNEGO_RES_PLTT_MAIN]);
    func_0204b98c(disp->res[GTSNEGO_RES_CHAR_SUB]);
    func_0204b98c(disp->res[GTSNEGO_RES_CHAR_MAIN]);
    func_0204be64(disp->res[GTSNEGO_RES_ANIM_SUB]);
    func_0204be64(disp->res[GTSNEGO_RES_ANIM_MAIN]);
    func_0204bcd0(disp->res[GTSNEGO_RES_PLTT_FOOTER]);
    func_0204b98c(disp->res[GTSNEGO_RES_CHAR_FOOTER]);
    func_0204be64(disp->res[GTSNEGO_RES_ANIM_FOOTER]);
    func_0204bcd0(disp->res[GTSNEGO_RES_PLTT_AVATAR]);
    for (i = 0; i < GTSNEGO_AVATAR_COUNT; i++) {
        func_0204b98c(disp->res[GTSNEGO_RES_CHAR_AVATAR + i]);
        func_0204be64(disp->res[GTSNEGO_RES_ANIM_AVATAR + i]);
    }
    GFL_TCBRemove(disp->vblankTask);
    func_0204bf98(disp->clUnit);
    func_0204b758();
    BlinkPalAnm_Free(disp->blinkMain);
    BlinkPalAnm_Free(disp->blinkSub);
    GFL_BGSysFreeCharMemory(7, CHAR_POS(disp->barChars), CHAR_SIZE(disp->barChars));
    GFL_BGSysFreeCharMemory(4, CHAR_POS(disp->subChars), CHAR_SIZE(disp->subChars));
    GFL_BGSysFreeCharMemory(0, CHAR_POS(disp->mainChars), CHAR_SIZE(disp->mainChars));
    GFL_BGSysFreeFilledChar(1, 1, 0);
    GFL_BGSysFreeFilledChar(5, 1, 0);
    GFL_BGSysFreeFilledChar(6, 1, 0);
    GFL_BGSysSetBGEnabled(5, FALSE);
    for (i = 0; i <= 7; i++) {
        GFL_BGSysReleaseBG(i);
    }
    GFL_BGSysFree();
    GFL_HeapFree(disp);
    GFL_OvlUnload(OVERLAY_APP_UI);
}

ClActUnit *GtsNegoDisp_GetClActUnit(GtsNegoDisp *disp) {
    return disp->clUnit;
}

static void GtsNegoDisp_CreateBGs(GtsNegoDisp *disp) {
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xe800),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            2,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(0, &setup, BGMODE_TEXT);
        GFL_BGSysClearBG(0);
        GFL_BGSysLoadScr(0);
        GFL_BGSysSetBGEnabled(0, TRUE);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf000),
            GX_BG_CHARBASE(0x08000),
            0x8000,
            GX_BG_EXTPLTT_01,
            0,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(1, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(1, TRUE);
        GFL_BGSysFillChar(1, 0, 1, 0);
        GFL_BGSysFillScrArea(1, 0, 0, 0, 32, 32, 17);
        GFL_BGSysLoadScr(1);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf800),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            3,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(2, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(2, TRUE);
        GFL_BGSysFillScrArea(2, 0, 0, 0, 32, 32, 17);
        GFL_BGSysLoadScr(2);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xe000),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            1,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(3, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(3, TRUE);
        GFL_BGSysFillScrArea(3, 0, 0, 0, 32, 32, 17);
        GFL_BGSysLoadScr(3);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x1000,
            0,
            BGRES_256x512,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xd000),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            2,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(4, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(4, TRUE);
        GFL_BGSysLoadScr(4);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf000),
            GX_BG_CHARBASE(0x08000),
            0x8000,
            GX_BG_EXTPLTT_01,
            0,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(5, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(5, TRUE);
        GFL_BGSysFillChar(5, 0, 1, 0);
        GFL_BGSysFillScrArea(5, 0, 0, 0, 32, 32, 17);
        GFL_BGSysLoadScr(5);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x1000,
            0,
            BGRES_256x512,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xe000),
            GX_BG_CHARBASE(0x10000),
            0x8000,
            GX_BG_EXTPLTT_01,
            1,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(6, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(6, TRUE);
        GFL_BGSysFillChar(6, 0, 1, 0);
        GFL_BGSysFillScrArea(6, 0, 0, 0, 32, 32, 17);
        GFL_BGSysLoadScr(6);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf800),
            GX_BG_CHARBASE(0x18000),
            0x8000,
            GX_BG_EXTPLTT_01,
            3,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(7, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(7, TRUE);
        GFL_BGSysLoadScr(7);
    }
}

// Dims count runs of palettes of the lower screen's OBJ or BG palettes (PALFADE_VRAM_*), each from a palette of
// starts for as many as in counts
static void GtsNegoDisp_DimPalettes(GtsNegoDisp *disp, u32 vram, int *starts, int *counts, int count) {
    int i;
    PaletteFade *fade = PaletteFade_Create(disp->heapId);
    u8 *colors;

    PaletteFade_AllocBuffer(fade, vram, 0x200, disp->heapId);
    PaletteFade_LoadFromVRAM(fade, vram, 0, 0x200);
    PaletteFade_BlendBuffer(fade, vram, 0, 0x100, 6, 0);
    colors = (u8 *)PaletteFade_GetFadedBuffer(fade, vram);
    for (i = 0; i < count; i++) {
        switch (vram) {
        case PALFADE_VRAM_SUB_OBJ:
            cp15_flushDC(colors + starts[i] * 32, counts[i] * 32);
            gfxUploadStdPaletteObjB(colors + starts[i] * 32, starts[i] * 32, counts[i] * 32);
            break;
        case PALFADE_VRAM_SUB_BG:
            cp15_flushDC(colors + starts[i] * 32, counts[i] * 32);
            gfxUploadStdPaletteBGB(colors + starts[i] * 32, starts[i] * 32, counts[i] * 32);
            break;
        }
    }
    PaletteFade_FreeBuffer(fade, vram);
    PaletteFade_Free(fade);
}

// The same, with the palettes copied in the V-blank
static void GtsNegoDisp_DimPalettesVBlank(GtsNegoDisp *disp, u32 vram, int *starts, int *counts, int count) {
    PaletteFade *fade = PaletteFade_Create(disp->heapId);
    u8 *colors;
    int i;

    PaletteFade_AllocBuffer(fade, vram, 0x200, disp->heapId);
    PaletteFade_LoadFromVRAM(fade, vram, 0, 0x200);
    PaletteFade_BlendBuffer(fade, vram, 0, 0x100, 6, 0);
    colors = (u8 *)PaletteFade_GetFadedBuffer(fade, vram);
    for (i = 0; i < count; i++) {
        switch (vram) {
        case PALFADE_VRAM_SUB_OBJ:
            NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_OBJ_PLTT_SUB, starts[i] * 32, colors + starts[i] * 32,
                                               counts[i] * 32);
            break;
        case PALFADE_VRAM_SUB_BG:
            NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_SUB, starts[i] * 32, colors + starts[i] * 32,
                                               counts[i] * 32);
            break;
        }
    }
    PaletteFade_FreeBuffer(fade, vram);
    PaletteFade_Free(fade);
}

void GtsNegoDisp_SetDim(GtsNegoDisp *disp, BOOL dim, u32 type) {
    if (dim) {
        sys_memcpy((void *)HW_DB_OBJ_PLTT, disp->objPalette, 0x200);
        sys_memcpy((void *)HW_DB_BG_PLTT, disp->bgPalette, 0x200);
        switch (type) {
        case GTSNEGO_DIM_SEARCH: {
            int starts[] = { 0 };
            int counts[] = { 16 };

            GtsNegoDisp_DimPalettes(disp, PALFADE_VRAM_SUB_OBJ, starts, counts, NELEMS(starts));
            break;
        }
        case GTSNEGO_DIM_ALL: {
            int starts[] = { 1 };
            int counts[] = { 15 };

            GtsNegoDisp_DimPalettes(disp, PALFADE_VRAM_SUB_OBJ, starts, counts, NELEMS(starts));
            break;
        }
        case GTSNEGO_DIM_LIST_MENU: {
            int starts[] = { 0, 11 };
            int counts[] = { 10, 5 };

            GtsNegoDisp_DimPalettes(disp, PALFADE_VRAM_SUB_OBJ, starts, counts, NELEMS(starts));
            break;
        }
        case GTSNEGO_DIM_MAIN_MENU: {
            int starts[] = { 1 };
            int counts[] = { 15 };

            GtsNegoDisp_DimPalettesVBlank(disp, PALFADE_VRAM_SUB_OBJ, starts, counts, NELEMS(starts));
            break;
        }
        }
        switch (type) {
        case GTSNEGO_DIM_ALL:
        case GTSNEGO_DIM_SEARCH: {
            int starts[] = { 0, 11 };
            int counts[] = { 9, 4 };

            GtsNegoDisp_DimPalettes(disp, PALFADE_VRAM_SUB_BG, starts, counts, NELEMS(starts));
            break;
        }
        case GTSNEGO_DIM_LIST_MENU: {
            int starts[] = { 0, 11 };
            int counts[] = { 9, 5 };

            GtsNegoDisp_DimPalettes(disp, PALFADE_VRAM_SUB_BG, starts, counts, NELEMS(starts));
            break;
        }
        case GTSNEGO_DIM_MAIN_MENU: {
            int starts[] = { 0, 11 };
            int counts[] = { 9, 4 };

            GtsNegoDisp_DimPalettesVBlank(disp, PALFADE_VRAM_SUB_BG, starts, counts, NELEMS(starts));
            break;
        }
        }
    } else {
        cp15_flushDC(disp->objPalette, 0x200);
        gfxUploadStdPaletteObjB(disp->objPalette, 0, 0x200);
        cp15_flushDC(disp->bgPalette, 0x200);
        gfxUploadStdPaletteBGB(disp->bgPalette, 0, 0x200);
    }
}

static void GtsNegoDisp_VBlank(TCB *tcb, void *work) {
    GtsNegoDisp *disp = work;

    func_0204b7c8();
    if (disp->scrollPending) {
        GFL_BGSysMoveBG(4, BG_MOVE_SET_Y, disp->listScroll + 72);
        GFL_BGSysMoveBG(6, BG_MOVE_SET_Y, disp->listScroll + 72);
        disp->scrollPending = FALSE;
    }
}

static void GtsNegoDisp_LoadGraphics(GtsNegoDisp *disp) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(GTSNEGO_ARCID, disp->heapId);
    int i = 0;

    GFL_G2DIOLoadArcNCLRDefault(arc, 3, PALTYPE_SUB_BG, 0, 0xc0, disp->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, PALTYPE_SUB_BG, 0, 0x40, disp->heapId);
    disp->barChars = GFL_BGSysLoadArcNCGRDynamic(arc, 6, 7, 0, FALSE, disp->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 17, 7, 0, CHAR_POS(disp->barChars), 0, FALSE, disp->heapId);
    disp->subChars = GFL_BGSysLoadArcNCGRDynamic(arc, 8, 4, 0, FALSE, disp->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 18, 4, 0, CHAR_POS(disp->subChars), 0, FALSE, disp->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, PALTYPE_MAIN_BG, 0, 0, disp->heapId);
    disp->mainChars = GFL_BGSysLoadArcNCGRDynamic(arc, 6, 0, 0, FALSE, disp->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 12, 0, 0, CHAR_POS(disp->mainChars), 0, FALSE, disp->heapId);
    disp->res[GTSNEGO_RES_CHAR_FOOTER] = func_0204b81c(arc, 5, FALSE, CLACT_VRAM_SUB, disp->heapId);
    disp->res[GTSNEGO_RES_PLTT_FOOTER] = func_0204bbb8(arc, 0, CLACT_VRAM_SUB, 0x140, 0, 1, disp->heapId);
    disp->res[GTSNEGO_RES_ANIM_FOOTER] = func_0204bde0(arc, 10, 22, disp->heapId);
    disp->res[GTSNEGO_RES_CHAR_SUB] = func_0204b81c(arc, 7, FALSE, CLACT_VRAM_SUB, disp->heapId);
    disp->res[GTSNEGO_RES_PLTT_SUB] = func_0204bbb8(arc, 2, CLACT_VRAM_SUB, 0x100, 0, 8, disp->heapId);
    disp->res[GTSNEGO_RES_ANIM_SUB] = func_0204bde0(arc, 11, 23, disp->heapId);
    disp->res[GTSNEGO_RES_CHAR_MAIN] = func_0204b81c(arc, 7, FALSE, CLACT_VRAM_MAIN, disp->heapId);
    disp->res[GTSNEGO_RES_PLTT_MAIN] = func_0204bbb8(arc, 2, CLACT_VRAM_MAIN, 0, 0, 8, disp->heapId);
    disp->res[GTSNEGO_RES_ANIM_MAIN] = func_0204bde0(arc, 11, 23, disp->heapId);
    disp->blinkMain = BlinkPalAnm_Create(0, 16, 0, disp->heapId);
    BlinkPalAnm_SetPalBufferArcTool(disp->blinkMain, arc, 1, 0, 16);
    disp->blinkSub = BlinkPalAnm_Create(0, 16, 7, disp->heapId);
    BlinkPalAnm_SetPalBufferArcTool(disp->blinkSub, arc, 1, 0, 16);
    GFL_ArcToolFree(arc);
    arc = GFL_ArcSysCreateFileHandle(GTSNEGO_ARCID_AVATAR, disp->heapId);
    for (; i < GTSNEGO_AVATAR_COUNT; i++) {
        disp->res[GTSNEGO_RES_CHAR_AVATAR + i] = func_0204b81c(arc, 1 + i, FALSE, CLACT_VRAM_SUB, disp->heapId);
        disp->res[GTSNEGO_RES_ANIM_AVATAR + i] = func_0204bde0(arc, 17 + i * 2, 18 + i * 2, disp->heapId);
    }
    disp->res[GTSNEGO_RES_PLTT_AVATAR] = func_0204bbb8(arc, 0, CLACT_VRAM_SUB, 0, 0, 8, disp->heapId);
    GFL_ArcToolFree(arc);
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2,
                        GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ, 15, 9);
}

void GtsNegoDisp_Dummy(GtsNegoDisp *disp, BOOL list) {
}

void GtsNegoDisp_ShowSearchMenu(GtsNegoDisp *disp) {
    ArcTool *arc;

    GtsNegoDisp_Dummy(disp, FALSE);
    arc = GFL_ArcSysCreateFileHandle(GTSNEGO_ARCID, disp->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 19, 4, 0, CHAR_POS(disp->subChars), 0, FALSE, disp->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 20, 4, 0, CHAR_POS(disp->subChars), 0xc0, FALSE, disp->heapId);
    GFL_ArcToolFree(arc);
    GtsNegoDisp_AddArrow(disp, 96, 56, FALSE);
    GtsNegoDisp_AddArrow(disp, 240, 56, TRUE);
    GtsNegoDisp_AddArrow(disp, 120, 96, FALSE);
    GtsNegoDisp_AddArrow(disp, 240, 96, TRUE);
    GtsNegoDisp_AddArrow(disp, 120, 136, FALSE);
    GtsNegoDisp_AddArrow(disp, 240, 136, TRUE);
}

void GtsNegoDisp_HideSearchMenu(GtsNegoDisp *disp) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(GTSNEGO_ARCID, disp->heapId);

    GtsNegoDisp_FreeArrows(disp);
    GFL_BGSysFillScrArea(6, 0, 0, 0, 32, 32, 17);
    GFL_BGSysQueueScrLoad(6);
    GFL_G2DIOLoadNSCRSync(arc, 18, 4, 0, CHAR_POS(disp->subChars), 0, FALSE, disp->heapId);
    GFL_BGSysQueueScrLoad(4);
    GFL_ArcToolFree(arc);
}

// The touch bar with its return button, its icons in an OBJ palette
void GtsNegoDisp_CreateTouchBar(GtsNegoDisp *disp, u32 objPalette) {
    TouchBarItem items[] = {
        { 1, { 232, 168 } },
    };
    TouchBarSetup setup;

    sys_memset(&setup, 0, sizeof(TouchBarSetup));
    setup.items = items;
    setup.count = NELEMS(items);
    setup.unit = disp->clUnit;
    setup.bg = 7;
    setup.bgPalette = 15;
    setup.vramType = CLACT_VRAM_BOTH;
    setup.unk1C = FALSE;
    setup.objPalette = objPalette;
    disp->touchBar = TouchBar_Create(&setup, disp->heapId);
}

TouchBar *GtsNegoDisp_GetTouchBar(GtsNegoDisp *disp) {
    return disp->touchBar;
}

void GtsNegoDisp_FreeTouchBar(GtsNegoDisp *disp) {
    if (disp->touchBar != NULL) {
        TouchBar_Free(disp->touchBar);
        disp->touchBar = NULL;
    }
}

static void GtsNegoDisp_CreateCursor(GtsNegoDisp *disp) {
    ClActorSetup setup;

    setup.x = 0;
    setup.y = 0;
    setup.sequence = 0;
    setup.priority = 0;
    setup.bgPriority = 2;
    disp->cursor = func_0204c040(disp->clUnit, disp->res[GTSNEGO_RES_CHAR_SUB], disp->res[GTSNEGO_RES_PLTT_SUB],
                                 disp->res[GTSNEGO_RES_ANIM_SUB], &setup, 1, disp->heapId);
    func_0204c520(disp->cursor, TRUE);
    func_0204c124(disp->cursor, FALSE);
    func_0204c318(disp->cursor, GX_OAM_MODE_XLU);
}

static GtsNegoCursorPos sGtsNegoDispCursors[] = {
    { 5, 128, 64, 29 },  // GTSNEGO_CURSOR_ANYONE
    { 5, 128, 112, 29 }, // GTSNEGO_CURSOR_RENDEZVOUS
    { 6, 168, 56, 30 },  // GTSNEGO_CURSOR_LEVEL
    { 7, 168, 96, 31 },  // GTSNEGO_CURSOR_MY_KIND
    { 7, 168, 136, 31 }, // GTSNEGO_CURSOR_WANT_KIND
    { 0, 0, 0, 0 },      // GTSNEGO_CURSOR_SEARCH
    { 8, 128, 48, 32 },  // GTSNEGO_CURSOR_FRIEND_1
    { 8, 128, 96, 32 },  // GTSNEGO_CURSOR_FRIEND_2
    { 8, 128, 144, 32 }, // GTSNEGO_CURSOR_FRIEND_3
    { 0, 0, 0, 0 },      // GTSNEGO_CURSOR_NONE
};

void GtsNegoDisp_SetCursor(GtsNegoDisp *disp, AppTaskMenuWin *button, int cursor) {
    ClActorPos pos;

    switch (cursor) {
    case GTSNEGO_CURSOR_NONE:
        func_0204c124(disp->cursor, FALSE);
        return;
    case GTSNEGO_CURSOR_SEARCH:
        func_0204c124(disp->cursor, FALSE);
        if (button != NULL) {
            AppTaskMenuWin_SetActive(button, TRUE);
        }
        return;
    case GTSNEGO_CURSOR_LEVEL:
    case GTSNEGO_CURSOR_MY_KIND:
    case GTSNEGO_CURSOR_WANT_KIND:
    case GTSNEGO_CURSOR_FRIEND_1:
    case GTSNEGO_CURSOR_FRIEND_2:
    case GTSNEGO_CURSOR_FRIEND_3:
        if (button != NULL) {
            AppTaskMenuWin_SetActive(button, FALSE);
        }
        break;
    }
    pos.x = sGtsNegoDispCursors[cursor].x;
    pos.y = sGtsNegoDispCursors[cursor].y;
    func_0204c140(disp->cursor, &pos, 1);
    func_0204c488(disp->cursor, sGtsNegoDispCursors[cursor].anim);
    func_0204c124(disp->cursor, TRUE);
}

BOOL GtsNegoDisp_PressCursor(GtsNegoDisp *disp, int cursor) {
    if (disp->cursor == NULL) {
        return FALSE;
    }
    func_0204c488(disp->cursor, sGtsNegoDispCursors[cursor].pressAnim);
    return TRUE;
}

BOOL GtsNegoDisp_IsCursorPressDone(GtsNegoDisp *disp) {
    if (disp->cursor == NULL) {
        return FALSE;
    }
    if (func_0204c4a0(disp->cursor) >= 29) {
        if (!func_0204c560(disp->cursor)) {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

// Adds an arrow of the search's settings, pointing left or right
static void GtsNegoDisp_AddArrow(GtsNegoDisp *disp, int x, int y, BOOL right) {
    int i;

    for (i = 0; i < GTSNEGO_ARROW_COUNT; i++) {
        if (disp->arrows[i] == NULL) {
            ClActorSetup setup;

            setup.x = x;
            setup.y = y;
            setup.sequence = right ? 2 : 0;
            setup.priority = 0;
            setup.bgPriority = 1;
            disp->arrows[i] =
                func_0204c040(disp->clUnit, disp->res[GTSNEGO_RES_CHAR_SUB], disp->res[GTSNEGO_RES_PLTT_SUB],
                              disp->res[GTSNEGO_RES_ANIM_SUB], &setup, 1, disp->heapId);
            func_0204c520(disp->arrows[i], TRUE);
            func_0204c124(disp->arrows[i], TRUE);
            return;
        }
    }
}

void GtsNegoDisp_FreeArrows(GtsNegoDisp *disp) {
    int i;

    for (i = 0; i < GTSNEGO_ARROW_COUNT; i++) {
        if (disp->arrows[i] != NULL) {
            func_0204c108(disp->arrows[i]);
            disp->arrows[i] = NULL;
        }
    }
}

// Plays an arrow's animation of a press: the one after its own
void GtsNegoDisp_PressArrow(GtsNegoDisp *disp, int arrow) {
    int sequence = func_0204c4a0(disp->arrows[arrow]);
    int pressed = 1;

    if (sequence >= 2) {
        pressed = 3;
    }
    func_0204c488(disp->arrows[arrow], pressed);
    func_0204c520(disp->arrows[arrow], TRUE);
}

void GtsNegoDisp_ShowFriendList(GtsNegoDisp *disp, BOOL scrollable) {
    ArcTool *arc;

    GtsNegoDisp_FreeTouchBar(disp);
    GtsNegoDisp_CreateTouchBar(disp, 8);
    GtsNegoDisp_Dummy(disp, TRUE);
    arc = GFL_ArcSysCreateFileHandle(GTSNEGO_ARCID, disp->heapId);
    disp->listScreenFile = GFL_G2DIOReadNSCRArc(arc, 21, FALSE, &disp->listScreen, disp->heapId);
    GFL_G2DIOLoadArcNCLR(arc, 0, PALTYPE_SUB_OBJ, 0, 0x140, 0x20, disp->heapId);
    GFL_ArcToolFree(arc);
    arc = GFL_ArcSysCreateFileHandle(GTSNEGO_ARCID_AVATAR, disp->heapId);
    GFL_G2DIOLoadArcNCLR(arc, 0, PALTYPE_SUB_OBJ, 0, 0, 0x100, disp->heapId);
    GFL_ArcToolFree(arc);
    GtsNegoDisp_CreateListFooter(disp);
    GtsNegoDisp_CreateListActors(disp, scrollable);
    disp->scrollPending = TRUE;
}

void GtsNegoDisp_FreeListScreen(GtsNegoDisp *disp) {
    if (disp->listScreenFile != NULL) {
        GFL_HeapFree(disp->listScreenFile);
        disp->listScreenFile = NULL;
    }
}

// Draws a row of the list of people on BG 4
static void GtsNegoDisp_DrawListRow(GtsNegoDisp *disp, int row) {
    u16 *tiles = (u16 *)disp->listScreen->rawData;
    u16 base = CHAR_POS(disp->subChars);
    int i = 0;
    int x;
    int y;

    for (y = 0; y < GTSNEGO_LIST_ROW_HEIGHT; y++) {
        for (x = 0; x < 32; x++) {
            GFL_BGSysSetScrTile(4, x, y + row * GTSNEGO_LIST_ROW_HEIGHT, base + tiles[i]);
            i++;
        }
    }
}

// Clears a row of the list, with the tile at the row's top right corner
static void GtsNegoDisp_ClearListRow(GtsNegoDisp *disp, int row) {
    u16 *tiles = (u16 *)disp->listScreen->rawData;
    u16 base = CHAR_POS(disp->subChars);
    int x;
    int y;

    for (y = 0; y < GTSNEGO_LIST_ROW_HEIGHT; y++) {
        for (x = 0; x < 32; x++) {
            GFL_BGSysSetScrTile(4, x, y + row * GTSNEGO_LIST_ROW_HEIGHT, tiles[31] + base);
        }
    }
}

// Draws the rows of the list that have a person, from the person at top
void GtsNegoDisp_DrawListRows(GtsNegoDisp *disp, GameData *gameData, int top) {
    int i;

    for (i = 0; i < GTSNEGO_LIST_ROWS; i++) {
        if (GtsNego_GetFriend(gameData, top + i) != NULL) {
            GtsNegoDisp_DrawListRow(disp, i);
        } else {
            GtsNegoDisp_ClearListRow(disp, i);
        }
    }
    GFL_BGSysQueueScrLoad(4);
}

void GtsNegoDisp_LeaveFriendList(GtsNegoDisp *disp) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(GTSNEGO_ARCID, disp->heapId);
    int i;

    for (i = 0; i < GTSNEGO_LIST_ROWS; i++) {
        if (disp->friendIcons[i] != NULL) {
            func_0204c108(disp->friendIcons[i]);
            disp->friendIcons[i] = NULL;
        }
    }
    GtsNegoDisp_FreeTouchBar(disp);
    GtsNegoDisp_CreateTouchBar(disp, 0);
    GtsNegoDisp_FreeListFooter(disp);
    GFL_ArcToolFree(arc);
}

void GtsNegoDisp_HideFriendList(GtsNegoDisp *disp) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(GTSNEGO_ARCID, disp->heapId);

    GtsNegoDisp_FreeListScreen(disp);
    GtsNegoDisp_FreeListActors(disp);
    GFL_BGSysFillScrArea(6, 0, 0, 0, 32, 32, 17);
    GFL_BGSysQueueScrLoad(6);
    GFL_G2DIOLoadArcNCLR(arc, 2, PALTYPE_SUB_OBJ, 0, 0x100, 0x100, disp->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 18, 4, 0, CHAR_POS(disp->subChars), 0, FALSE, disp->heapId);
    GFL_BGSysQueueScrLoad(4);
    GFL_ArcToolFree(arc);
}

int GtsNegoDisp_ScrollDown(GtsNegoDisp *disp, GtsNegoScroll *scroll) {
    int result = GTSNEGO_SCROLL_NONE;

    if (scroll->cursor < GTSNEGO_LIST_SHOWN - 1 && scroll->cursor < scroll->count - 1) {
        scroll->cursor++;
        return GTSNEGO_SCROLL_CURSOR;
    }
    if (scroll->cursor == GTSNEGO_LIST_SHOWN - 1 && scroll->top + GTSNEGO_LIST_SHOWN < scroll->count) {
        scroll->top++;
        result = GTSNEGO_SCROLL_LIST;
    }
    return result;
}

int GtsNegoDisp_ScrollUp(GtsNegoDisp *disp, GtsNegoScroll *scroll) {
    int result = GTSNEGO_SCROLL_NONE;

    if (scroll->cursor > 0) {
        scroll->cursor--;
        return GTSNEGO_SCROLL_CURSOR;
    }
    if (scroll->cursor == 0 && scroll->top != 0) {
        scroll->top--;
        result = GTSNEGO_SCROLL_LIST;
    }
    return result;
}

// Puts the cursor on a row of the list that has a person
void GtsNegoDisp_SetScrollCursor(GtsNegoDisp *disp, GtsNegoScroll *scroll, u32 row) {
    if (row < scroll->count) {
        scroll->cursor = row;
    }
    if (scroll->cursor > GTSNEGO_LIST_SHOWN - 1) {
        scroll->cursor = GTSNEGO_LIST_SHOWN - 1;
    }
}

static void GtsNegoDisp_CreateListFooter(GtsNegoDisp *disp) {
    ClActorSetup setup;

    setup.x = 128;
    setup.y = 200;
    setup.sequence = 0;
    setup.priority = 1;
    setup.bgPriority = 1;
    disp->listFooter = func_0204c040(disp->clUnit, disp->res[GTSNEGO_RES_CHAR_FOOTER],
                                     disp->res[GTSNEGO_RES_PLTT_FOOTER], disp->res[GTSNEGO_RES_ANIM_FOOTER], &setup, 1,
                                     disp->heapId);
    func_0204c520(disp->listFooter, TRUE);
    func_0204c124(disp->listFooter, TRUE);
}

void GtsNegoDisp_FreeListFooter(GtsNegoDisp *disp) {
    if (disp->listFooter != NULL) {
        func_0204c108(disp->listFooter);
    }
    disp->listFooter = NULL;
}

// The list's actors, without the knob when the list does not scroll
static void GtsNegoDisp_CreateListActors(GtsNegoDisp *disp, BOOL scrollable) {
    ClActorSetup setup;
    u16 xs[] = { 250, 250, 128 };
    u16 ys[] = { 96, 96, 16 };
    u16 sequences[] = { 4, 10, 9 };
    u16 bgPriorities[] = { 1, 1, 1 };
    int i;

    for (i = 0; i < GTSNEGO_LIST_ACTOR_COUNT; i++) {
        if (!scrollable && i == GTSNEGO_LIST_ACTOR_KNOB) {
            continue;
        }
        setup.x = xs[i];
        setup.y = ys[i];
        setup.sequence = sequences[i];
        setup.priority = i;
        setup.bgPriority = bgPriorities[i];
        disp->listActors[i] =
            func_0204c040(disp->clUnit, disp->res[GTSNEGO_RES_CHAR_SUB], disp->res[GTSNEGO_RES_PLTT_SUB],
                          disp->res[GTSNEGO_RES_ANIM_SUB], &setup, 1, disp->heapId);
        func_0204c520(disp->listActors[i], TRUE);
        func_0204c124(disp->listActors[i], TRUE);
    }
}

void GtsNegoDisp_FreeListActors(GtsNegoDisp *disp) {
    int i;

    for (i = 0; i < GTSNEGO_LIST_ACTOR_COUNT; i++) {
        if (disp->listActors[i] != NULL) {
            func_0204c108(disp->listActors[i]);
            disp->listActors[i] = NULL;
        }
    }
}

void GtsNegoDisp_SetKnobAnimating(GtsNegoDisp *disp, BOOL animating) {
    if (disp->listActors[GTSNEGO_LIST_ACTOR_KNOB] != NULL) {
        func_0204c56c(disp->listActors[GTSNEGO_LIST_ACTOR_KNOB]);
        func_0204c520(disp->listActors[GTSNEGO_LIST_ACTOR_KNOB], animating);
    }
}

void GtsNegoDisp_StartListScroll(GtsNegoDisp *disp, int dir) {
    disp->listScrollDir = dir;
    if (dir == GTSNEGO_LIST_UP) {
        disp->listScrollTarget = disp->listScroll + GTSNEGO_LIST_ROW_HEIGHT * 8;
    } else if (dir == GTSNEGO_LIST_DOWN) {
        disp->listScrollTarget = disp->listScroll - GTSNEGO_LIST_ROW_HEIGHT * 8;
    }
}

void GtsNegoDisp_StopListScroll(GtsNegoDisp *disp) {
    disp->listScrollDir = GTSNEGO_LIST_STILL;
}

// Once the list has scrolled a row, the scroll goes back, for the caller to move the rows instead
int GtsNegoDisp_UpdateListScroll(GtsNegoDisp *disp, int *done) {
    int i;

    if (disp->listScrollDir == GTSNEGO_LIST_UP) {
        disp->listScroll += GTSNEGO_LIST_SCROLL_SPEED;
        if (disp->listScrollTarget == disp->listScroll) {
            *done = disp->listScrollDir;
            disp->listScrollDir = GTSNEGO_LIST_STILL;
            disp->listScroll -= GTSNEGO_LIST_ROW_HEIGHT * 8;
        }
        for (i = 0; i < GTSNEGO_LIST_ROWS; i++) {
            GtsNegoDisp_MoveFriendIcon(disp, i, -GTSNEGO_LIST_SCROLL_SPEED);
        }
        disp->scrollPending = TRUE;
    }
    if (disp->listScrollDir == GTSNEGO_LIST_DOWN) {
        disp->listScroll -= GTSNEGO_LIST_SCROLL_SPEED;
        if (disp->listScrollTarget == disp->listScroll) {
            *done = disp->listScrollDir;
            disp->listScrollDir = GTSNEGO_LIST_STILL;
            disp->listScroll += GTSNEGO_LIST_ROW_HEIGHT * 8;
        }
        for (i = 0; i < GTSNEGO_LIST_ROWS; i++) {
            GtsNegoDisp_MoveFriendIcon(disp, i, GTSNEGO_LIST_SCROLL_SPEED);
        }
        disp->scrollPending = TRUE;
    }
    return disp->listScrollDir;
}

// Moves the people's icons a row up or down
void GtsNegoDisp_MoveFriendIcons(GtsNegoDisp *disp, BOOL up) {
    int dir = 1;
    int i;

    if (up) {
        dir = -1;
    }
    for (i = 0; i < GTSNEGO_LIST_ROWS; i++) {
        GtsNegoDisp_MoveFriendIcon(disp, i, dir * (GTSNEGO_LIST_ROW_HEIGHT * 8));
    }
}

void GtsNegoDisp_ResetList(GtsNegoDisp *disp) {
    GFL_BGSysMoveBG(4, BG_MOVE_SET_Y, 0);
    GFL_BGSysMoveBG(6, BG_MOVE_SET_Y, 0);
    GFL_BGSysFillScrArea(1, 0, 0, 0, 32, 32, 17);
    GFL_BGSysLoadScr(1);
}

// Creates the icon of a row of the list, hidden, with a trainer's look
void GtsNegoDisp_CreateFriendIcon(GtsNegoDisp *disp, int row, int avatar) {
    ClActorSetup setup;

    if (disp->friendIcons[row] != NULL) {
        func_0204c108(disp->friendIcons[row]);
    }
    setup.x = 0;
    setup.y = 0;
    setup.sequence = 1;
    setup.priority = 10;
    setup.bgPriority = 2;
    disp->friendIcons[row] =
        func_0204c040(disp->clUnit, disp->res[GTSNEGO_RES_CHAR_AVATAR + avatar], disp->res[GTSNEGO_RES_PLTT_AVATAR],
                      disp->res[GTSNEGO_RES_ANIM_AVATAR + avatar], &setup, 1, disp->heapId);
    func_0204c520(disp->friendIcons[row], TRUE);
    func_0204c124(disp->friendIcons[row], FALSE);
}

void GtsNegoDisp_ShowFriendIcon(GtsNegoDisp *disp, int row, int x, int y) {
    if (row < GTSNEGO_LIST_ROWS) {
        ClActorPos pos;

        pos.x = x;
        pos.y = y;
        func_0204c140(disp->friendIcons[row], &pos, 1);
        func_0204c124(disp->friendIcons[row], TRUE);
    }
}

// The icon of a person in a row of the list, the second row being the first on the screen
void GtsNegoDisp_SetFriendIcon(GtsNegoDisp *disp, PlayerInfo *friend, int row) {
    if (friend != NULL) {
        GtsNegoDisp_CreateFriendIcon(disp, row, func_02008bf4(friend));
        GtsNegoDisp_ShowFriendIcon(disp, row, 16, -(GTSNEGO_LIST_ROW_HEIGHT * 8) * (1 - row));
    }
}

void GtsNegoDisp_MoveFriendIcon(GtsNegoDisp *disp, int row, int y) {
    if (disp->friendIcons[row] != NULL) {
        ClActorPos pos;

        func_0204c178(disp->friendIcons[row], &pos, 1);
        pos.y += y;
        func_0204c140(disp->friendIcons[row], &pos, 1);
    }
}

void GtsNegoDisp_ShiftFriendIconsUp(GtsNegoDisp *disp, PlayerInfo *friend) {
    if (friend != NULL) {
        int i;

        if (disp->friendIcons[0] != NULL) {
            func_0204c108(disp->friendIcons[0]);
        }
        for (i = 0; i < GTSNEGO_LIST_ROWS - 1; i++) {
            disp->friendIcons[i] = disp->friendIcons[i + 1];
        }
        disp->friendIcons[GTSNEGO_LIST_ROWS - 1] = NULL;
        GtsNegoDisp_SetFriendIcon(disp, friend, GTSNEGO_LIST_ROWS - 1);
    }
}

void GtsNegoDisp_ShiftFriendIconsDown(GtsNegoDisp *disp, PlayerInfo *friend) {
    if (friend != NULL) {
        int i;

        if (disp->friendIcons[GTSNEGO_LIST_ROWS - 1] != NULL) {
            func_0204c108(disp->friendIcons[GTSNEGO_LIST_ROWS - 1]);
        }
        for (i = GTSNEGO_LIST_ROWS - 1; i > 0; i--) {
            disp->friendIcons[i] = disp->friendIcons[i - 1];
        }
        disp->friendIcons[0] = NULL;
        GtsNegoDisp_SetFriendIcon(disp, friend, 0);
    }
}

void GtsNegoDisp_SetFriendIcons(GtsNegoDisp *disp, GameData *gameData, int top) {
    int i;

    for (i = 0; i < GTSNEGO_LIST_ROWS; i++) {
        if (i + top >= 0) {
            PlayerInfo *friend = GtsNego_GetFriend(gameData, i + top);

            if (friend != NULL) {
                GtsNegoDisp_SetFriendIcon(disp, friend, i);
            }
        }
    }
}

// How far down the scroll bar the knob is for a position of count
static int GtsNegoDisp_GetKnobY(GtsNegoDisp *disp, int pos, int count) {
    if (pos == 0) {
        return 0;
    }
    if (pos == count - 1) {
        return GTSNEGO_KNOB_RANGE;
    }
    return pos * GTSNEGO_KNOB_RANGE / (count - 1);
}

// The first row of the list for the knob at y
static int GtsNegoDisp_GetKnobPos(GtsNegoDisp *disp, int y, int count) {
    float pos = y - GTSNEGO_KNOB_TOP;
    float range = GTSNEGO_KNOB_RANGE;

    if (count <= GTSNEGO_LIST_SHOWN) {
        return 0;
    }
    pos = pos / (range / (count - 2));
    if (pos >= count - 2) {
        pos = count - 3;
    }
    return pos;
}

void GtsNegoDisp_SetKnob(GtsNegoDisp *disp, int pos, int count) {
    if (disp->listActors[GTSNEGO_LIST_ACTOR_KNOB] != NULL) {
        ClActorPos knob;

        func_0204c178(disp->listActors[GTSNEGO_LIST_ACTOR_KNOB], &knob, 1);
        knob.y = GtsNegoDisp_GetKnobY(disp, pos, count) + GTSNEGO_KNOB_TOP;
        func_0204c140(disp->listActors[GTSNEGO_LIST_ACTOR_KNOB], &knob, 1);
    }
}

int GtsNegoDisp_DragKnob(GtsNegoDisp *disp, int y, int count) {
    int pos = 0;

    if (disp->listActors[GTSNEGO_LIST_ACTOR_KNOB] != NULL) {
        ClActorPos knob;

        func_0204c178(disp->listActors[GTSNEGO_LIST_ACTOR_KNOB], &knob, 1);
        knob.y = y;
        func_0204c140(disp->listActors[GTSNEGO_LIST_ACTOR_KNOB], &knob, 1);
        pos = GtsNegoDisp_GetKnobPos(disp, y, count);
    }
    return pos;
}

// The search's trainer cycles through the looks, a sequence each
static void GtsNegoDisp_OnSearchAnimEnd(u32 param, fx32 frame) {
    GtsNegoDisp *disp = (GtsNegoDisp *)param;
    u16 sequence = func_0204c4a0(disp->searchActors[1]) + 1;

    if (sequence > 26) {
        sequence = 11;
    }
    func_0204c488(disp->searchActors[1], sequence);
}

static void GtsNegoDisp_CreateSearchActors(GtsNegoDisp *disp) {
    ClActorSetup setup;
    ClActorCallback callback;

    setup.x = 128;
    setup.y = 52;
    setup.sequence = 27;
    setup.priority = 2;
    setup.bgPriority = 1;
    disp->searchActors[0] =
        func_0204c040(disp->clUnit, disp->res[GTSNEGO_RES_CHAR_MAIN], disp->res[GTSNEGO_RES_PLTT_MAIN],
                      disp->res[GTSNEGO_RES_ANIM_MAIN], &setup, 0, disp->heapId);
    func_0204c520(disp->searchActors[0], TRUE);
    func_0204c124(disp->searchActors[0], TRUE);
    setup.sequence = 11;
    setup.priority = 1;
    disp->searchActors[1] =
        func_0204c040(disp->clUnit, disp->res[GTSNEGO_RES_CHAR_MAIN], disp->res[GTSNEGO_RES_PLTT_MAIN],
                      disp->res[GTSNEGO_RES_ANIM_MAIN], &setup, 0, disp->heapId);
    func_0204c520(disp->searchActors[1], TRUE);
    func_0204c124(disp->searchActors[1], TRUE);
    callback.type = CLACT_CALLBACK_LAST_FRAME;
    callback.param = (u32)disp;
    callback.func = GtsNegoDisp_OnSearchAnimEnd;
    func_0204c5b0(disp->searchActors[1], &callback);
    GFL_BGSysSetEnabledBGsA(GX_PLANEMASK_ALL);
}

static void GtsNegoDisp_FreeSearchActors(GtsNegoDisp *disp) {
    func_0204c124(disp->searchActors[0], FALSE);
    func_0204c108(disp->searchActors[0]);
    func_0204c124(disp->searchActors[1], FALSE);
    func_0204c108(disp->searchActors[1]);
}

// Stops the search's trainer on the partner's look
void GtsNegoDisp_ShowPartner(GtsNegoDisp *disp, int avatar) {
    int sequences[GTSNEGO_AVATAR_COUNT] = { 0, 6, 8, 12, 14, 3, 4, 10, 11, 1, 13, 5, 7, 9, 15, 2 };
    ArcTool *arc = GFL_ArcSysCreateFileHandle(GTSNEGO_ARCID, disp->heapId);

    GFL_G2DIOLoadNSCRSync(arc, 15, 0, 0, CHAR_POS(disp->mainChars), 0, FALSE, disp->heapId);
    GFL_ArcToolFree(arc);
    func_0204c520(disp->searchActors[1], FALSE);
    func_0204c488(disp->searchActors[1], sequences[avatar] + 11);
    func_0204c504(disp->searchActors[1], 4);
    func_0204c488(disp->searchActors[0], 28);
    GFL_BGSysQueueScrLoad(0);
    disp->topScroll = 0;
    disp->topScrolling = FALSE;
}

void GtsNegoDisp_ShowSearching(GtsNegoDisp *disp) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(GTSNEGO_ARCID, disp->heapId);

    GFL_G2DIOLoadNSCRSync(arc, 14, 2, 0, CHAR_POS(disp->mainChars), 0, FALSE, disp->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 13, 0, 0, CHAR_POS(disp->mainChars), 0, FALSE, disp->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 16, 3, 0, CHAR_POS(disp->mainChars), 0, FALSE, disp->heapId);
    GFL_ArcToolFree(arc);
    GtsNegoDisp_CreateSearchActors(disp);
    GFL_BGSysQueueScrLoad(2);
    GFL_BGSysQueueScrLoad(0);
    GFL_BGSysQueueScrLoad(3);
    disp->topScroll = 0;
    disp->topScrolling = TRUE;
}

static void GtsNegoDisp_UpdateTopScroll(GtsNegoDisp *disp) {
    if (disp->topScrolling) {
        GFL_BGSysMoveBG(2, BG_MOVE_SET_Y, disp->topScroll);
        disp->topScroll++;
    }
}

void GtsNegoDisp_Reset(GtsNegoDisp *disp) {
    ArcTool *arc;
    int i;

    for (i = 0; i < GTSNEGO_LIST_ROWS; i++) {
        if (disp->friendIcons[i] != NULL) {
            func_0204c124(disp->friendIcons[i], FALSE);
            func_0204c108(disp->friendIcons[i]);
            disp->friendIcons[i] = NULL;
        }
    }
    GtsNegoDisp_FreeListFooter(disp);
    GtsNegoDisp_FreeListScreen(disp);
    GtsNegoDisp_FreeSearchActors(disp);
    GtsNegoDisp_FreeListActors(disp);
    GtsNegoDisp_FreeArrows(disp);
    arc = GFL_ArcSysCreateFileHandle(GTSNEGO_ARCID, disp->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 12, 0, 0, CHAR_POS(disp->mainChars), 0, FALSE, disp->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 18, 4, 0, CHAR_POS(disp->subChars), 0, FALSE, disp->heapId);
    GFL_G2DIOLoadArcNCLR(arc, 2, PALTYPE_SUB_OBJ, 0, 0x100, 0x100, disp->heapId);
    GFL_ArcToolFree(arc);
    GtsNegoDisp_FreeTouchBar(disp);
    GtsNegoDisp_CreateTouchBar(disp, 0);
    GFL_BGSysSetBGEnabled(2, FALSE);
    GFL_BGSysQueueScrLoad(0);
    GFL_BGSysSetBGEnabled(3, FALSE);
    disp->topScroll = 0;
    disp->topScrolling = FALSE;
}
