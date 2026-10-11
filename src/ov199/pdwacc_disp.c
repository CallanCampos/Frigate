// The Dream World account screens' display: their BGs, a palette that cycles through blends of 15 palettes, and a
// scrolling BG 0. The name is the ROM's string, from GFL_HeapAllocate's call. Function names are ours.

#include "types.h"
#include "app/gsync/pdwacc_disp.h"
#include "app/ui/ui_scene.h"
#include "constants/arc.h"
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
#include "system/palanm.h"

#define PDWACC_DISP_ACTOR_COUNT 13
#define PDWACC_DISP_RES_COUNT 9

// The palettes the cycle blends between, and the 4 steps of each blend
#define PDWACC_PALETTE_COUNT 15
#define PDWACC_BLEND_STEPS 4

// Palette 1's colors 4 to 10 cycle through blends of the file's palettes, a step every other frame
typedef struct {
    TCB *task;
    BOOL active;
    u16 palettes[PDWACC_PALETTE_COUNT][16];
    u16 blended[PDWACC_PALETTE_COUNT * PDWACC_BLEND_STEPS][16];
    s16 frame;
    u8 unk96A;
    u8 skip;
    // The BG that scrolls with the cycle
    u8 scrollBg;
    u8 unk96D[3];
} PdwAccPaletteCycle;

struct PdwAccDisp {
    u8 unk0[8];
    PdwAccPaletteCycle cycle;
    u32 unk978;
    ClActUnit *clUnit;
    TCB *vblankTask;
    TCB *hblankTask;
    u32 res[PDWACC_DISP_RES_COUNT];
    u32 unk9AC;
    ClActor *actors[PDWACC_DISP_ACTOR_COUNT];
    HeapID heapId;
    u8 unk9E6[0x18a];
};

static void PdwAccDisp_CreateBGs(PdwAccDisp *disp);
static void PdwAccDisp_VBlank(TCB *task, void *work);
static void PdwAccDisp_LoadGraphics(PdwAccDisp *disp);
static void PdwAccDisp_CyclePalette(TCB *task, void *work);
static void PdwAccDisp_DeleteActors(PdwAccDisp *disp);

static BGSysLCDConfig sPdwAccLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

static BGSysVRAMConfig sPdwAccVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

PdwAccDisp *PdwAccDisp_Create(HeapID heapId) {
    PdwAccDisp *disp = GFL_HeapAllocate(heapId, sizeof(PdwAccDisp), TRUE, "pdwacc_disp.c", 201);

    disp->heapId = heapId;
    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    GFL_BGSysSetDisplayLayout(1);
    GXS_DispOn();
    gfxEngineEnableA();
    GFL_BGSysCreate(disp->heapId);
    ClActSys_Create(&data_02093f24, &sPdwAccVRAMConfig, disp->heapId);
    disp->clUnit = func_0204bf1c(40, 0, disp->heapId);
    GFL_BGSysSetVRAMBanks(&sPdwAccVRAMConfig);
    GFL_BGSysSetLCDConfig(&sPdwAccLCDConfig);
    PdwAccDisp_CreateBGs(disp);
    PdwAccDisp_LoadGraphics(disp);
    disp->vblankTask = GFL_VBlankTCBAdd(PdwAccDisp_VBlank, disp, 0);
    GFL_BGSysSetEnabledBGsB(GX_PLANEMASK_ALL);
    GFL_BGSysSetEnabledBGsA(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ);
    return disp;
}

void PdwAccDisp_Main(PdwAccDisp *disp) {
    func_0204b794();
}

void PdwAccDisp_Free(PdwAccDisp *disp) {
    int i;

    PdwAccDisp_DeleteActors(disp);
    if (disp->cycle.task != NULL) {
        GFL_TCBRemove(disp->cycle.task);
        disp->cycle.task = NULL;
        disp->cycle.active = FALSE;
    }
    for (i = 0; i < 3; i++) {
        if (disp->res[i] != 0) {
            func_0204bcd0(disp->res[i]);
        }
    }
    for (; i < 6; i++) {
        if (disp->res[i] != 0) {
            func_0204b98c(disp->res[i]);
        }
    }
    for (; i < PDWACC_DISP_RES_COUNT; i++) {
        if (disp->res[i] != 0) {
            func_0204be64(disp->res[i]);
        }
    }
    if (disp->hblankTask != NULL) {
        GFL_TCBRemove(disp->hblankTask);
    }
    GFL_TCBRemove(disp->vblankTask);
    func_0204bf98(disp->clUnit);
    func_0204b758();
    GFL_BGSysFreeFilledChar(1, 1, 0);
    GFL_BGSysFreeFilledChar(6, 1, 0);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysSetBGEnabled(5, FALSE);
    GFL_BGSysFree();
    GFL_HeapFree(disp);
    GFL_OvlUnload(OVERLAY_APP_UI);
}

static void PdwAccDisp_CreateBGs(PdwAccDisp *disp) {
    {
        BGSetup setup = {
            0,
            0,
            0x1000,
            0,
            BGRES_512x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf800),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            2,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(0, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(0, TRUE);
        GFL_BGSysLoadScr(0);
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
            1,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(1, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(1, TRUE);
        GFL_BGSysFillChar(1, 0, 1, 0);
        GFL_BGSysFillScrArea(1, 0, 0, 0, 32, 32, BGSYS_FILL_TILE_PALETTE);
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
            GX_BG_SCRBASE(0xe000),
            GX_BG_CHARBASE(0x10000),
            0x8000,
            GX_BG_EXTPLTT_01,
            0,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(2, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(2, TRUE);
        GFL_BGSysFillScrArea(2, 0, 0, 0, 32, 32, BGSYS_FILL_TILE_PALETTE);
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
            GX_BG_SCRBASE(0xe800),
            GX_BG_CHARBASE(0x18000),
            0x8000,
            GX_BG_EXTPLTT_01,
            3,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(3, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(3, TRUE);
        GFL_BGSysFillChar(3, 0, 1, 0);
        GFL_BGSysFillScrArea(3, 0, 0, 0, 32, 32, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysLoadScr(3);
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
            1,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(5, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(5, TRUE);
        GFL_BGSysFillChar(5, 0, 1, 0);
        GFL_BGSysFillScrArea(5, 0, 0, 0, 32, 32, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysLoadScr(5);
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
            GX_BG_CHARBASE(0x10000),
            0x8000,
            GX_BG_EXTPLTT_01,
            0,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(6, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(6, TRUE);
        GFL_BGSysFillChar(6, 0, 1, 0);
        GFL_BGSysFillScrArea(6, 0, 0, 0, 32, 32, BGSYS_FILL_TILE_PALETTE);
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
            GX_BG_SCRBASE(0xe800),
            GX_BG_CHARBASE(0x18000),
            0x8000,
            GX_BG_EXTPLTT_01,
            3,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(7, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(7, TRUE);
        GFL_BGSysFillChar(7, 0, 1, 0);
        GFL_BGSysFillScrArea(7, 0, 0, 0, 32, 32, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysLoadScr(7);
    }
}

static void PdwAccDisp_VBlank(TCB *task, void *work) {
    func_0204b7c8();
}

// Loads the palettes the cycle blends between, makes the blends and starts the cycle
static inline void PdwAccDisp_InitCycle(PdwAccPaletteCycle *cycle, ArcTool *arc, HeapID heapId) {
    int i;
    int count = 0;
    NNSG2dPaletteData *palette;
    void *file;

    sys_memset(cycle, 0, sizeof(*cycle));
    file = GFL_G2DIOReadNCLRArc(arc, 2, &palette, heapId);
    sys_memcpy(palette->rawData, cycle->palettes, sizeof(cycle->palettes));
    sys_memcpy(palette->rawData, cycle->blended, sizeof(cycle->palettes));
    GFL_HeapFree(file);

    // Colors 4 to 10 of each palette, blended into the next in 4 steps
    for (i = 0; i < PDWACC_PALETTE_COUNT; i++) {
        int next = i + 1;
        fx32 fraction;
        BOOL done;

        if (next >= PDWACC_PALETTE_COUNT) {
            next -= PDWACC_PALETTE_COUNT;
        }
        fraction = 0;
        done = FALSE;
        while (TRUE) {
            int j;

            for (j = 4; j < 11; j++) {
                BlendColors(&cycle->palettes[i][j], &cycle->blended[count][j], 1, fraction >> 8,
                            cycle->palettes[next][j]);
            }
            count++;
            if (done == TRUE) {
                break;
            }
            fraction += FX32_ONE * 6 / 16;
            if (fraction >= FX32_ONE) {
                fraction = FX32_ONE;
                done = TRUE;
            }
        }
    }
    cp15_flushDC(cycle->blended, sizeof(cycle->blended));
    cycle->active = TRUE;
    cycle->task = GFL_VBlankTCBAdd(PdwAccDisp_CyclePalette, cycle, 20);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG0, GX_BLEND_PLANEMASK_BG3, 8, 16);
    cycle->scrollBg = 0;
}

static void PdwAccDisp_LoadGraphics(PdwAccDisp *disp) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_PDWACC, disp->heapId);

    GFL_G2DIOLoadArcNCLRDefault(arc, 3, PALTYPE_SUB_BG, 0, 0, disp->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 4, 4, 0, 0, FALSE, disp->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 6, 4, 0, 0, FALSE, disp->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 3, PALTYPE_MAIN_BG, 0, 0, disp->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 4, 0, 0, 0, FALSE, disp->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 5, 0, 0, 0, FALSE, disp->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 4, 3, 0, 0, FALSE, disp->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 7, 3, 0, 0, FALSE, disp->heapId);
    PdwAccDisp_InitCycle(&disp->cycle, arc, disp->heapId);
    GFL_ArcToolFree(arc);
}

static void PdwAccDisp_CyclePalette(TCB *task, void *work) {
    PdwAccPaletteCycle *cycle = work;

    if (cycle->active) {
        cycle->skip ^= 1;
        if (!(cycle->skip & 1)) {
            gfxUploadStdPaletteBGA(&cycle->blended[cycle->frame][4], 0x20 + 4 * 2, 7 * 2);
            gfxUploadStdPaletteBGB(&cycle->blended[cycle->frame][4], 0x20 + 4 * 2, 7 * 2);
            cycle->frame++;
            if (cycle->frame >= PDWACC_PALETTE_COUNT * PDWACC_BLEND_STEPS) {
                cycle->frame = 0;
            }
            GFL_BGSysMoveBG(cycle->scrollBg, BG_MOVE_LEFT, 7);
        }
    }
}

static void PdwAccDisp_DeleteActors(PdwAccDisp *disp) {
    int i;

    for (i = 0; i < PDWACC_DISP_ACTOR_COUNT; i++) {
        if (disp->actors[i] != NULL) {
            func_0204c108(disp->actors[i]);
            disp->actors[i] = NULL;
        }
    }
}
