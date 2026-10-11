// Game Sync's screens: the BGs, the actors, the Pokémon sent's icon, the top screen's wave and progress gauge, and the
// items and Pokémon of the Dream World that drift down it. The name is the ROM's string, from GFL_HeapAllocate's call.
// Function names are ours.

#include "types.h"
#include "app/gsync/gsync_disp.h"
#include "app/ui/ui_scene.h"
#include "constants/arc.h"
#include "constants/species.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "nnsys/g2d.h"
#include "pml/item.h"
#include "pml/poke_party.h"
#include "system/bmp_winframe.h"
#include "system/poke_icon.h"
#include "system/sin_wave_table.h"
#include "system/tpoke_data.h"

#define GSYNC_DISP_ACTOR_COUNT 17
#define GSYNC_DISP_FLOAT_COUNT (GSYNC_DISP_FLOAT_ITEM_COUNT + GSYNC_DISP_FLOAT_POKEMON_COUNT)

// The actors that aren't one of the top screen's sequences
#define GSYNC_DISP_ACTOR_POKE_ICON 14
#define GSYNC_DISP_ACTOR_SUB 15

// The OBJ resources: the palettes, then the characters, then the cell animations
enum {
    GSYNC_DISP_RES_PLTT_ACTORS,
    GSYNC_DISP_RES_PLTT_SUB,
    GSYNC_DISP_RES_PLTT_POKE_ICON,
    GSYNC_DISP_RES_PLTT_3,
    GSYNC_DISP_RES_PLTT_4,
    GSYNC_DISP_RES_PLTT_ITEM,
    GSYNC_DISP_RES_CHAR_ACTORS,
    GSYNC_DISP_RES_CHAR_SUB,
    GSYNC_DISP_RES_CHAR_POKE_ICON,
    GSYNC_DISP_RES_CHAR_FLOAT_POKEMON,
    GSYNC_DISP_RES_CHAR_10,
    GSYNC_DISP_RES_CHAR_11,
    GSYNC_DISP_RES_CHAR_ITEM,
    GSYNC_DISP_RES_CELL_ACTORS,
    GSYNC_DISP_RES_CELL_SUB,
    GSYNC_DISP_RES_CELL_POKE_ICON,
    GSYNC_DISP_RES_CELL_16,
    GSYNC_DISP_RES_CELL_17,
    GSYNC_DISP_RES_CELL_18,
    GSYNC_DISP_RES_CELL_ITEM,
    GSYNC_DISP_RES_COUNT,
};

// An item or a Pokémon floating up the top screen
typedef struct {
    // The shared actor it is drawn with while it floats
    ClActor *actor;
    u16 x;
    u16 y;
    // The angles of its sway
    u16 angleX;
    u16 angleY;
    u16 timer;
    u16 active;
    // The item, or the Pokémon's species, form and sex
    u16 id;
    u16 form;
    u16 sex;
    u16 done;
} GSyncDispFloat;

// What a floating item or Pokémon is drawn with
typedef struct {
    u16 palette[16];
    u8 chars[0x200];
    u32 paletteNum;
} GSyncDispFloatGraphics;

// Where each top screen actor is
typedef struct {
    s32 x;
    s32 y;
    s32 priority;
} GSyncDispActorPos;

struct GSyncDisp {
    u32 subBgChars;
    u32 mainBgChars;
    u32 unk8;
    ClActUnit *clUnit;
    TCB *vblankTask;
    TCB *hblankTask;
    NNSG2dPaletteData *bgPalette;
    void *bgPaletteFile;
    u32 res[GSYNC_DISP_RES_COUNT];
    u32 unk70;
    ClActor *actors[GSYNC_DISP_ACTOR_COUNT];
    ClActor *floatPokemonActor;
    ClActor *floatItemActor;
    GSyncDispFloatGraphics floatGraphics[GSYNC_DISP_FLOAT_COUNT];
    GSyncDispFloat floats[GSYNC_DISP_FLOAT_COUNT];
    HeapID heapId;
    // The line of the wave's table the screen's first line takes
    s16 waveLine;
    s16 waveTable[192];
    u8 unk454C[0xc];
    fx32 fade;
    int fadeStep;
    // The line the progress gauge is filled up to
    int progressLine;
    u32 unk4564;
    int paletteTimer;
};

static void GSyncDisp_CreateBGs(GSyncDisp *disp);
static void GSyncDisp_VBlank(TCB *task, void *work);
static void GSyncDisp_LoadGraphics(GSyncDisp *disp);
static void GSyncDisp_CreateSubActorAt(GSyncDisp *disp, s16 x, s16 y);
static void GSyncDisp_InitActor(GSyncDisp *disp, int index);
static void GSyncDisp_DeleteActors(GSyncDisp *disp);
static void GSyncDisp_LoadPokeIconRes(GSyncDisp *disp);
static void GSyncDisp_HBlank(TCB *task, void *work);
static void GSyncDisp_UpdateFade(GSyncDisp *disp);
static void GSyncDisp_StartPokemonFloat(GSyncDisp *disp, int index);
static void GSyncDisp_HideFloat(GSyncDispFloat *fl);
static void GSyncDisp_StartItemFloat(GSyncDisp *disp, int index);
static void GSyncDisp_UpdateFloat(GSyncDispFloat *fl);
static void GSyncDisp_UploadPokemonFloat(GSyncDisp *disp, int index, ClActor *actor);
static void GSyncDisp_UploadItemFloat(GSyncDisp *disp, int index, ClActor *actor);
static void GSyncDisp_CreatePokemonFloatActor(GSyncDisp *disp);
static void GSyncDisp_CreateItemFloatActor(GSyncDisp *disp);

static BGSysVRAMConfig sGSyncDispVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static GSyncDispActorPos sActorPositions[] = {
    { 128, 150, 4 }, { 128, 150, 4 }, { 132, 155, 1 }, { 122, 155, 1 }, { 128, 145, 2 },
    { 128, 145, 2 }, { 128, 145, 2 }, { 128, 145, 2 }, { 128, 150, 0 }, { 100, 125, 0 },
    { 128, 160, 1 }, { 128, 160, 5 }, { 128, 160, 5 }, { 128, 30, 0 },
};

static BGSysLCDConfig sGSyncDispLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

GSyncDisp *GSyncDisp_Create(HeapID heapId) {
    GSyncDisp *disp = GFL_HeapAllocate(heapId, sizeof(GSyncDisp), TRUE, "gsync_disp.c", 257);

    disp->heapId = heapId;
    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    GFL_BGSysSetDisplayLayout(1);
    GXS_DispOn();
    gfxEngineEnableA();
    GFL_BGSysCreate(disp->heapId);
    ClActSys_Create(&data_02093f24, &sGSyncDispVRAMConfig, disp->heapId);
    disp->clUnit = func_0204bf1c(40, 0, disp->heapId);
    GFL_BGSysSetVRAMBanks(&sGSyncDispVRAMConfig);
    GFL_BGSysSetLCDConfig(&sGSyncDispLCDConfig);
    GSyncDisp_CreateBGs(disp);
    GSyncDisp_LoadGraphics(disp);
    GSyncDisp_CreatePokemonFloatActor(disp);
    GSyncDisp_CreateItemFloatActor(disp);
    disp->vblankTask = GFL_VBlankTCBAdd(GSyncDisp_VBlank, disp, 0);
    disp->progressLine = 200;
    GFL_BGSysSetEnabledBGsB(GX_PLANEMASK_ALL);
    GFL_BGSysSetEnabledBGsA(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ);
    return disp;
}

void GSyncDisp_Main(GSyncDisp *disp) {
    disp->waveLine++;
    if (disp->waveLine >= 192) {
        disp->waveLine = 0;
    }
    func_0204b794();
    GSyncDisp_UpdateFade(disp);
    GSyncDisp_UpdateFloats(disp);
}

void GSyncDisp_Free(GSyncDisp *disp) {
    int i;

    GSyncDisp_DeleteActors(disp);
    GFL_HeapFree(disp->bgPaletteFile);
    func_0204c108(disp->floatPokemonActor);
    func_0204c108(disp->floatItemActor);
    for (i = 0; i < GSYNC_DISP_RES_CHAR_ACTORS; i++) {
        if (disp->res[i] != 0) {
            func_0204bcd0(disp->res[i]);
        }
    }
    for (; i < GSYNC_DISP_RES_CELL_ACTORS; i++) {
        if (disp->res[i] != 0) {
            func_0204b98c(disp->res[i]);
        }
    }
    for (; i < GSYNC_DISP_RES_COUNT; i++) {
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
    // BUG: BG 5's filled character is freed from BG 1, which has none, so BG 5's stays allocated until
    // GFL_BGSysFree
#ifdef BUGFIX
    GFL_BGSysFreeFilledChar(5, 1, 0);
#else
    GFL_BGSysFreeFilledChar(1, 1, 0);
#endif
    GFL_BGSysFreeFilledChar(6, 1, 0);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysSetBGEnabled(5, FALSE);
    GFL_BGSysFree();
    GFL_HeapFree(disp);
    GFL_OvlUnload(OVERLAY_APP_UI);
}

static void GSyncDisp_CreateBGs(GSyncDisp *disp) {
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
            GX_BG_SCRBASE(0xe800),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            3,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(1, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(1, TRUE);
        GFL_BGSysLoadScr(1);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x1000,
            0,
            BGRES_512x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf000),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            1,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(3, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(3, TRUE);
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
            GX_BG_CHARBASE(0x10000),
            0x8000,
            GX_BG_EXTPLTT_01,
            0,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(5, &setup, BGMODE_TEXT);
        GFL_BGSysFillChar(5, 0, 1, 0);
        GFL_BGSysFillScrArea(5, 0, 0, 0, 32, 32, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysSetBGEnabled(5, TRUE);
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
            GX_BG_CHARBASE(0x18000),
            0x8000,
            GX_BG_EXTPLTT_01,
            1,
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
            GX_BG_CHARBASE(0x00000),
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

// Cycles palette 2 of both screens' BGs through the palette file's next 8, each for 10 frames
static void GSyncDisp_VBlank(TCB *task, void *work) {
    GSyncDisp *disp = work;
    int timer;

    func_0204b7c8();
    timer = ++disp->paletteTimer;
    if (timer % 10 == 1) {
        u8 *palette = disp->bgPalette->rawData;

        gfxUploadStdPaletteBGA(palette + (timer / 10 + 2) * 0x20, 2 * 0x20, 0x20);
        gfxUploadStdPaletteBGB(palette + (timer / 10 + 2) * 0x20, 2 * 0x20, 0x20);
    }
    if (disp->paletteTimer >= 80) {
        disp->paletteTimer = 0;
    }
}

static void GSyncDisp_LoadGraphics(GSyncDisp *disp) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_GSYNC, disp->heapId);

    GFL_G2DIOLoadArcNCLRDefault(arc, 2, PALTYPE_SUB_BG, 0, 0, disp->heapId);
    disp->subBgChars = GFL_BGSysLoadArcNCGRDynamic(arc, 8, 4, 0, FALSE, disp->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 21, 4, 0, CHAR_POS(disp->subBgChars), 0, FALSE, disp->heapId);
    // BUG: BG 7 is on the sub screen and shares BG 4's characters, but its screen is offset by the main BGs'
    // characters, which aren't loaded yet. Both offsets are 0, so it works
#ifdef BUGFIX
    GFL_G2DIOLoadNSCRSync(arc, 24, 7, 0, CHAR_POS(disp->subBgChars), 0, FALSE, disp->heapId);
#else
    GFL_G2DIOLoadNSCRSync(arc, 24, 7, 0, CHAR_POS(disp->mainBgChars), 0, FALSE, disp->heapId);
#endif
    disp->bgPaletteFile = GFL_G2DIOReadNCLRArc(arc, 2, &disp->bgPalette, disp->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 2, PALTYPE_MAIN_BG, 0, 0, disp->heapId);
    disp->mainBgChars = GFL_BGSysLoadArcNCGRDynamic(arc, 8, 0, 0, FALSE, disp->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 23, 0, 0, CHAR_POS(disp->mainBgChars), 0, FALSE, disp->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 24, 1, 0, CHAR_POS(disp->mainBgChars), 0, FALSE, disp->heapId);
    disp->res[GSYNC_DISP_RES_CHAR_ACTORS] = func_0204b81c(arc, 9, FALSE, CLACT_VRAM_BOTH, disp->heapId);
    disp->res[GSYNC_DISP_RES_PLTT_ACTORS] = func_0204bbb8(arc, 3, CLACT_VRAM_BOTH, 3 * 0x20, 0, 4, disp->heapId);
    disp->res[GSYNC_DISP_RES_CELL_ACTORS] = func_0204bde0(arc, 12, 25, disp->heapId);
    disp->res[GSYNC_DISP_RES_CHAR_SUB] = func_0204b81c(arc, 10, FALSE, CLACT_VRAM_BOTH, disp->heapId);
    disp->res[GSYNC_DISP_RES_PLTT_SUB] = func_0204bbb8(arc, 4, CLACT_VRAM_BOTH, 7 * 0x20, 0, 0, disp->heapId);
    disp->res[GSYNC_DISP_RES_CELL_SUB] = func_0204bde0(arc, 13, 26, disp->heapId);
    GSyncDisp_LoadPokeIconRes(disp);
    GFL_ArcToolFree(arc);
}

void GSyncDisp_CreateSubActor(GSyncDisp *disp) {
    GSyncDisp_CreateSubActorAt(disp, 133, 128);
}

void GSyncDisp_CreateActor(GSyncDisp *disp, int index) {
    GSyncDisp_InitActor(disp, index);
}

void GSyncDisp_SetActorSequence(GSyncDisp *disp, int index, int sequence) {
    func_0204c488(disp->actors[index], sequence);
}

void GSyncDisp_SetActorCallback(GSyncDisp *disp, int index, const ClActorCallback *callback) {
    func_0204c5b0(disp->actors[index], callback);
}

void GSyncDisp_DeleteActor(GSyncDisp *disp, int index) {
    if (disp->actors[index] != NULL) {
        func_0204c108(disp->actors[index]);
        disp->actors[index] = NULL;
    }
}

static void GSyncDisp_CreateSubActorAt(GSyncDisp *disp, s16 x, s16 y) {
    if (disp->actors[GSYNC_DISP_ACTOR_SUB] == NULL) {
        ClActorSetup setup;

        setup.x = x;
        setup.y = y;
        setup.sequence = 0;
        setup.priority = 0;
        setup.bgPriority = 1;
        disp->actors[GSYNC_DISP_ACTOR_SUB] =
            func_0204c040(disp->clUnit, disp->res[GSYNC_DISP_RES_CHAR_SUB], disp->res[GSYNC_DISP_RES_PLTT_SUB],
                          disp->res[GSYNC_DISP_RES_CELL_SUB], &setup, CLACT_SURFACE_SUB, disp->heapId);
        func_0204c520(disp->actors[GSYNC_DISP_ACTOR_SUB], TRUE);
        func_0204c124(disp->actors[GSYNC_DISP_ACTOR_SUB], TRUE);
    }
}

static void GSyncDisp_InitActor(GSyncDisp *disp, int index) {
    ClActorSetup setup;

    if (disp->actors[index] != NULL) {
        func_0204c108(disp->actors[index]);
    }
    setup.x = sActorPositions[index].x;
    setup.y = sActorPositions[index].y;
    setup.sequence = index;
    setup.priority = sActorPositions[index].priority;
    setup.bgPriority = 1;
    disp->actors[index] =
        func_0204c040(disp->clUnit, disp->res[GSYNC_DISP_RES_CHAR_ACTORS], disp->res[GSYNC_DISP_RES_PLTT_ACTORS],
                      disp->res[GSYNC_DISP_RES_CELL_ACTORS], &setup, CLACT_SURFACE_MAIN, disp->heapId);
    func_0204c4d4(disp->actors[index], 0);
    func_0204c520(disp->actors[index], TRUE);
    func_0204c124(disp->actors[index], TRUE);
    func_0204c244(disp->actors[index], 2);
}

void GSyncDisp_PokeIconSequence2(GSyncDisp *disp) {
    func_0204c488(disp->actors[GSYNC_DISP_ACTOR_POKE_ICON], 2);
}

static void GSyncDisp_DeleteActors(GSyncDisp *disp) {
    int i;

    for (i = 0; i < GSYNC_DISP_ACTOR_COUNT; i++) {
        if (disp->actors[i] != NULL) {
            func_0204c108(disp->actors[i]);
            disp->actors[i] = NULL;
        }
    }
}

static void GSyncDisp_LoadPokeIconRes(GSyncDisp *disp) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, disp->heapId);

    disp->res[GSYNC_DISP_RES_PLTT_POKE_ICON] = func_0204bc48(arc, func_02021114(), CLACT_VRAM_BOTH, 0, disp->heapId);
    GFL_ArcToolFree(arc);
    arc = GFL_ArcSysCreateFileHandle(ARCID_GSYNC, disp->heapId);
    disp->res[GSYNC_DISP_RES_CELL_POKE_ICON] = func_0204bde0(arc, 14, 27, disp->heapId);
    GFL_ArcToolFree(arc);
}

void GSyncDisp_CreatePokeIcon(GSyncDisp *disp, BoxPkm *pkm, u32 surface) {
    const s32 x[] = { 133, 129 };
    const s32 y[] = { 128, 132 };
    ClActorSetup setup;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, disp->heapId);

    disp->res[GSYNC_DISP_RES_CHAR_POKE_ICON] =
        func_0204b81c(arc, func_02020f40(pkm), FALSE, CLACT_VRAM_BOTH, disp->heapId);
    if (surface == CLACT_SURFACE_MAIN) {
        setup.x = x[1];
        setup.y = y[1];
    } else {
        setup.x = x[0];
        setup.y = y[0];
    }
    setup.sequence = 0;
    setup.priority = 3;
    setup.bgPriority = 1;
    disp->actors[GSYNC_DISP_ACTOR_POKE_ICON] =
        func_0204c040(disp->clUnit, disp->res[GSYNC_DISP_RES_CHAR_POKE_ICON], disp->res[GSYNC_DISP_RES_PLTT_POKE_ICON],
                      disp->res[GSYNC_DISP_RES_CELL_POKE_ICON], &setup, surface, disp->heapId);
    func_0204c520(disp->actors[GSYNC_DISP_ACTOR_POKE_ICON], FALSE);
    func_0204c124(disp->actors[GSYNC_DISP_ACTOR_POKE_ICON], TRUE);
    func_0204c378(disp->actors[GSYNC_DISP_ACTOR_POKE_ICON], func_020210c0(pkm), 1);
    GFL_ArcToolFree(arc);
}

void GSyncDisp_StartPokeIcon(GSyncDisp *disp) {
    func_0204c520(disp->actors[GSYNC_DISP_ACTOR_POKE_ICON], TRUE);
}

void GSyncDisp_PokeIconSequence1(GSyncDisp *disp) {
    func_0204c488(disp->actors[GSYNC_DISP_ACTOR_POKE_ICON], 1);
    func_0204c520(disp->actors[GSYNC_DISP_ACTOR_POKE_ICON], TRUE);
}

// The progress gauge's colors, the line where it is filled and the lines below it, then white
static u16 sGaugeColors[10] = {
    GX_RGB(22, 21, 22), GX_RGB(23, 23, 23), GX_RGB(24, 24, 24), GX_RGB(25, 25, 25), GX_RGB(26, 26, 26),
    GX_RGB(27, 27, 27), GX_RGB(28, 28, 28), GX_RGB(29, 29, 29), GX_RGB(30, 30, 30), GX_RGB(31, 31, 31),
};

// Sways BG 3 line by line, and colors the progress gauge's lines
static void GSyncDisp_HBlank(TCB *task, void *work) {
    GSyncDisp *disp = work;
    s32 vcount = GX_GetVCount();
    int line = (vcount + (disp->waveLine + 1)) % 192;

    if (GX_IsHBlank()) {
        G2_SetBG3Offset(disp->waveTable[line], 0);
        if (disp->progressLine >= 192) {
            gfxUploadStdPaletteBGA(sGaugeColors, 10 * 0x20 + 2, 2);
        } else if (disp->progressLine == 0) {
            gfxUploadStdPaletteBGA(&sGaugeColors[9], 10 * 0x20 + 2, 2);
        } else if (vcount > 200) {
            gfxUploadStdPaletteBGA(sGaugeColors, 10 * 0x20 + 2, 2);
        } else if (disp->progressLine > vcount) {
            gfxUploadStdPaletteBGA(sGaugeColors, 10 * 0x20 + 2, 2);
        } else {
            int diff = vcount - disp->progressLine;

            if (diff < NELEMS(sGaugeColors) && diff >= 0) {
                gfxUploadStdPaletteBGA(&sGaugeColors[diff], 10 * 0x20 + 2, 2);
            } else {
                gfxUploadStdPaletteBGA(&sGaugeColors[9], 10 * 0x20 + 2, 2);
            }
        }
    }
}

void GSyncDisp_StartWave(GSyncDisp *disp) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_GSYNC, disp->heapId);

    GFL_G2DIOLoadNSCRSync(arc, 22, 3, 0, CHAR_POS(disp->mainBgChars), 0, FALSE, disp->heapId);
    GFL_ArcToolFree(arc);
    disp->hblankTask = GFL_HBlankTCBAdd(GSyncDisp_HBlank, disp, 0);
    SinWaveTable_Make(disp->waveTable, 192, 0x5b0, 0x1800);
}

void GSyncDisp_StartFade(GSyncDisp *disp, BOOL fadeIn) {
    if (fadeIn) {
        disp->fade = 0;
        disp->fadeStep = 1;
    } else {
        // BUG: a fade out starts past the 10 that ends a fade, so it stops at once and leaves BG 3 shown
#ifdef BUGFIX
        disp->fade = FX32_CONST(10);
#else
        disp->fade = FX32_CONST(16);
#endif
        disp->fadeStep = -1;
    }
}

// Blends BG 3 in or out over the others
static void GSyncDisp_UpdateFade(GSyncDisp *disp) {
    int alpha;

    if (disp->fadeStep != 0) {
        disp->fade += disp->fadeStep << 10;
        alpha = disp->fade >> FX32_SHIFT;
        GFL_BGSysSetBGEnabled(3, TRUE);
        if (alpha > 10) {
            disp->fadeStep = 0;
        } else if (alpha < 0) {
            alpha = 0;
            GFL_BGSysSetBGEnabled(3, FALSE);
            // The fade ends, with the clamped alpha's 0
            disp->fadeStep = alpha;
        } else {
            gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG3,
                                GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 |
                                    GX_BLEND_PLANEMASK_OBJ | GX_BLEND_PLANEMASK_BD,
                                alpha, 8);
        }
    }
}

void GSyncDisp_SetProgress(GSyncDisp *disp, int percent) {
    f32 height = (f32)percent * 1.2;
    int line;

    if (percent > 100) {
        line = 0;
    } else {
        line = 120.0f - height;
    }
    if (disp->progressLine > line) {
        disp->progressLine = line;
    }
}

static void GSyncDisp_StartPokemonFloat(GSyncDisp *disp, int index) {
    GSyncDisp_UploadPokemonFloat(disp, index, disp->floatPokemonActor);
    {
        ClActor *actor = disp->floatPokemonActor;
        ClActorPos pos = { 128, 0 };
        ClActorScale scale = { FX32_ONE, FX32_ONE };

        func_0204c140(actor, &pos, CLACT_SURFACE_MAIN);
        func_0204c270(actor, &scale);
        func_0204c520(actor, FALSE);
        func_0204c124(actor, FALSE);
        func_0204c318(actor, 1);
        func_0204c244(actor, 2);
        disp->floats[index].actor = actor;
    }
}

static void GSyncDisp_HideFloat(GSyncDispFloat *fl) {
    if (fl->actor != NULL) {
        func_0204c124(fl->actor, FALSE);
    }
    fl->actor = NULL;
}

static void GSyncDisp_StartItemFloat(GSyncDisp *disp, int index) {
    GSyncDisp_UploadItemFloat(disp, index, disp->floatItemActor);
    {
        ClActor *actor = disp->floatItemActor;
        ClActorPos pos = { 156, 28 };
        ClActorScale scale = { FX32_ONE, FX32_ONE };

        func_0204c140(actor, &pos, CLACT_SURFACE_MAIN);
        func_0204c270(actor, &scale);
        func_0204c520(actor, FALSE);
        func_0204c124(actor, FALSE);
        func_0204c318(actor, 1);
        func_0204c244(actor, 2);
        disp->floats[index].actor = actor;
    }
}

// Moves a float down from the top of the screen, swaying, and shrinks it until it vanishes
static void GSyncDisp_UpdateFloat(GSyncDispFloat *fl) {
    ClActorPos pos;
    ClActorScale scale;
    s16 swayX;
    s16 swayY;

    if (fl == NULL) {
        return;
    }
    swayX = (s16)(FX_SinIdx(fl->angleX) * 4) >> FX32_SHIFT;
    swayY = (s16)(FX_SinIdx(fl->angleY) * 3) >> FX32_SHIFT;
    if (fl->actor != NULL) {
        func_0204c178(fl->actor, &pos, CLACT_SURFACE_MAIN);
        u16 x = fl->x;

        pos.x = x + swayX + swayY;
        pos.y = (fl->y + swayX + swayY + fl->timer) >> 2;
        if (fl->timer % 8 == 0) {
            if (x > 128) {
                fl->x = x - 1;
            } else {
                fl->x = x + 1;
            }
        }
        func_0204c140(fl->actor, &pos, CLACT_SURFACE_MAIN);
        func_0204c124(fl->actor, TRUE);
        fl->timer++;
        if (pos.y > 25) {
            func_0204c27c(fl->actor, &scale);
            scale.x -= 16;
            scale.y -= 16;
            if (scale.x < 0 || scale.y < 0) {
                GSyncDisp_HideFloat(fl);
            } else {
                func_0204c270(fl->actor, &scale);
            }
        }
        if (pos.y > 50) {
            GSyncDisp_HideFloat(fl);
        }
    } else {
        fl->done = TRUE;
    }
    if (!fl->done) {
        fl->angleX += (u16)(GFL_RandomLC(12) + 200);
        fl->angleY -= (u16)(GFL_RandomLC(12) + 666);
    }
}

// Moves the floats. Another item starts when no item is floating, and another Pokémon when nothing is, since
// `floating` isn't cleared between the two
void GSyncDisp_UpdateFloats(GSyncDisp *disp) {
    BOOL floating = FALSE;
    int i;

    for (i = 0; i < GSYNC_DISP_FLOAT_ITEM_COUNT; i++) {
        if (disp->floats[i].active && !disp->floats[i].done) {
            GSyncDisp_UpdateFloat(&disp->floats[i]);
            floating = TRUE;
        }
    }
    if (!floating) {
        for (i = 0; i < GSYNC_DISP_FLOAT_ITEM_COUNT; i++) {
            int index = GFL_RandomLC(GSYNC_DISP_FLOAT_ITEM_COUNT);

            if (!disp->floats[index].active && disp->floats[index].id != 0) {
                disp->floats[index].active = TRUE;
                disp->floats[index].x = GFL_RandomLC(128) + 64;
                disp->floats[index].y = 0;
                GSyncDisp_StartItemFloat(disp, index);
                break;
            }
        }
    }
    for (i = 0; i < GSYNC_DISP_FLOAT_POKEMON_COUNT; i++) {
        int index = i + GSYNC_DISP_FLOAT_ITEM_COUNT;

        if (disp->floats[index].active && !disp->floats[index].done) {
            GSyncDisp_UpdateFloat(&disp->floats[index]);
            floating = TRUE;
        }
    }
    if (!floating) {
        for (i = 0; i < GSYNC_DISP_FLOAT_POKEMON_COUNT; i++) {
            int index = GFL_RandomLC(GSYNC_DISP_FLOAT_POKEMON_COUNT) + GSYNC_DISP_FLOAT_ITEM_COUNT;

            if (!disp->floats[index].active && disp->floats[index].id != 0) {
                disp->floats[index].active = TRUE;
                disp->floats[index].x = GFL_RandomLC(128) + 64;
                disp->floats[index].y = 0;
                GSyncDisp_StartPokemonFloat(disp, index);
                return;
            }
        }
    }
}

// Sets a float's Pokémon, if it is one with an overworld model
void GSyncDisp_SetFloatPokemon(GSyncDisp *disp, int index, int species, int form, int sex) {
    TPokeData *data = LoadTPokeData(disp->heapId);

    if (species != SPECIES_NONE && species <= SPECIES_GENESECT) {
        if (GetFieldPokemonMMdlLUTIndex_(data, species, sex, form) != 0xffff) {
            disp->floats[index].id = species;
            disp->floats[index].form = form;
            if (sex > 2) {
                sex = 0;
            }
            disp->floats[index].sex = sex;
        }
    }
    FreeTPokeData(data);
}

void GSyncDisp_SetFloatItem(GSyncDisp *disp, int index, u16 item) {
    disp->floats[index].id = item;
}

void GSyncDisp_LoadFloatGraphics(GSyncDisp *disp) {
    NNSG2dCharacterData *chars;
    NNSG2dPaletteData *palette;
    ArcTool *itemArc;
    ArcTool *iconArc;
    void *file;
    int i;

    itemArc = GFL_ArcSysCreateFileHandle(ARCID_ITEMGRA, disp->heapId);
    for (i = 0; i < GSYNC_DISP_FLOAT_ITEM_COUNT; i++) {
        int item = disp->floats[i].id;

        if (item != ITEM_NONE) {
            file = GFL_ArcToolReadHeapNew(itemArc, GetItemGraphicsDatID(item, ITEM_FILE_ICON_PLTT), disp->heapId);
            NNS_G2dGetUnpackedPaletteData(file, &palette);
            sys_memcpy(palette->rawData, disp->floatGraphics[i].palette, sizeof(disp->floatGraphics[i].palette));
            GFL_HeapFree(file);
            file = GFL_ArcToolReadHeapNew(itemArc, GetItemGraphicsDatID(item, ITEM_FILE_ICON_CHAR), disp->heapId);
            NNS_G2dGetUnpackedBGCharacterData(file, &chars);
            sys_memcpy(chars->rawData, disp->floatGraphics[i].chars, sizeof(disp->floatGraphics[i].chars));
            GFL_HeapFree(file);
        }
    }
    GFL_ArcToolFree(itemArc);

    iconArc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, disp->heapId);
    for (i = 0; i < GSYNC_DISP_FLOAT_POKEMON_COUNT; i++) {
        int index = i + GSYNC_DISP_FLOAT_ITEM_COUNT;
        int species = disp->floats[index].id;

        if (species != SPECIES_NONE) {
            int sex = disp->floats[index].sex;
            u32 form = PML_PkmSanitizeForme(species, disp->floats[index].form);
            u32 paletteNum = func_02021034(species, form, sex, FALSE);
            u16 icon = PokeParty_GetIconIndex(species, form, sex, FALSE);

            file = GFL_ArcToolReadHeapNew(iconArc, icon, disp->heapId);
            NNS_G2dGetUnpackedBGCharacterData(file, &chars);
            sys_memcpy(chars->rawData, disp->floatGraphics[index].chars, sizeof(disp->floatGraphics[index].chars));
            GFL_HeapFree(file);
            disp->floatGraphics[index].paletteNum = paletteNum;
        }
    }
    GFL_ArcToolFree(iconArc);
}

static void GSyncDisp_UploadPokemonFloat(GSyncDisp *disp, int index, ClActor *actor) {
    NNSG2dImageProxy proxy;

    func_0204c40c(actor, &proxy);
    sys_memcpy(disp->floatGraphics[index].chars,
               (void *)(HW_OBJ_VRAM + proxy.vramLocation.baseAddrOfVram[NNS_G2D_VRAM_TYPE_2DMAIN]),
               sizeof(disp->floatGraphics[index].chars));
    func_0204c378(actor, (u8)disp->floatGraphics[index].paletteNum, 1);
}

static void GSyncDisp_UploadItemFloat(GSyncDisp *disp, int index, ClActor *actor) {
    NNSG2dImageProxy proxy;

    func_0204c40c(actor, &proxy);
    sys_memcpy(disp->floatGraphics[index].chars,
               (void *)(HW_OBJ_VRAM + proxy.vramLocation.baseAddrOfVram[NNS_G2D_VRAM_TYPE_2DMAIN]),
               sizeof(disp->floatGraphics[index].chars));
    cp15_flushDC(disp->floatGraphics[index].palette, sizeof(disp->floatGraphics[index].palette));
    gfxUploadStdPaletteObjA(disp->floatGraphics[index].palette, 8 * 0x20, sizeof(disp->floatGraphics[index].palette));
}

// The actor the floating Pokémon are drawn with, by turns
static void GSyncDisp_CreatePokemonFloatActor(GSyncDisp *disp) {
    ClActorSetup setup;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, disp->heapId);
    u16 icon = PokeParty_GetIconIndex(SPECIES_BULBASAUR, 0, 0, FALSE);

    disp->res[GSYNC_DISP_RES_CHAR_FLOAT_POKEMON] = func_0204b81c(arc, icon, FALSE, CLACT_VRAM_MAIN, disp->heapId);
    setup.x = 133;
    setup.y = 128;
    setup.sequence = 0;
    setup.priority = 3;
    setup.bgPriority = 2;
    disp->floatPokemonActor = func_0204c040(
        disp->clUnit, disp->res[GSYNC_DISP_RES_CHAR_FLOAT_POKEMON], disp->res[GSYNC_DISP_RES_PLTT_POKE_ICON],
        disp->res[GSYNC_DISP_RES_CELL_POKE_ICON], &setup, CLACT_SURFACE_MAIN, disp->heapId);
    func_0204c520(disp->floatPokemonActor, FALSE);
    func_0204c124(disp->floatPokemonActor, FALSE);
    GFL_ArcToolFree(arc);
}

// The actor the floating items are drawn with, by turns
static void GSyncDisp_CreateItemFloatActor(GSyncDisp *disp) {
    ClActorSetup setup;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ITEMGRA, disp->heapId);

    disp->res[GSYNC_DISP_RES_PLTT_ITEM] =
        func_0204bbb8(arc, GetItemGraphicsDatID(ITEM_MASTER_BALL, ITEM_FILE_ICON_PLTT), CLACT_VRAM_MAIN, 8 * 0x20, 0, 1,
                      disp->heapId);
    disp->res[GSYNC_DISP_RES_CHAR_ITEM] = func_0204b81c(
        arc, GetItemGraphicsDatID(ITEM_MASTER_BALL, ITEM_FILE_ICON_CHAR), FALSE, CLACT_VRAM_MAIN, disp->heapId);
    disp->res[GSYNC_DISP_RES_CELL_ITEM] = func_0204bde0(arc, 1, 0, disp->heapId);
    GFL_ArcToolFree(arc);
    setup.x = 156;
    setup.y = 28;
    setup.sequence = 0;
    setup.priority = 0;
    setup.bgPriority = 2;
    disp->floatItemActor =
        func_0204c040(disp->clUnit, disp->res[GSYNC_DISP_RES_CHAR_ITEM], disp->res[GSYNC_DISP_RES_PLTT_ITEM],
                      disp->res[GSYNC_DISP_RES_CELL_ITEM], &setup, CLACT_SURFACE_MAIN, disp->heapId);
    func_0204c520(disp->floatItemActor, FALSE);
    func_0204c124(disp->floatItemActor, FALSE);
}
