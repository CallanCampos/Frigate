#include "types.h"
#include "app/dpc_obj.h"
#include "app/dpc_main.h"
#include "gfl/arc.h"
#include "gfl/bmp.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/math_util.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "pml/poke_graphic.h"
#include "pml/poke_party.h"
#include "system/app_menu_common.h"
#include "system/bmp_oam.h"
#include "system/palanm.h"

// The Hall of Fame PC's actors: the Pokémon on their ring, the buttons and the page number. The ROM doesn't name
// this file; the name is a guess, after box2_obj.c

#define DPCOBJ_RES_NONE 0xffffffff

// The common cells for the screen's OBJ VRAM mapping, 128 KB
#define DPCOBJ_MAPPING_128K 2

// The ring's center and radii, in pixels
#define DPCOBJ_RING_X 128
#define DPCOBJ_RING_Y 88
#define DPCOBJ_RING_RADIUS_X 100
#define DPCOBJ_RING_RADIUS_Y 44

// An actor and the resources it uses
typedef struct {
    ClActorSetup setup;
    u32 chars;
    u32 palette;
    u32 cellAnims;
    u16 unk14;
    u16 surface;
} DpcObjSetup;

// Where a Pokémon is on the ring, in degrees, and how dark it is there
typedef struct {
    u16 angle;
    u16 fade;
} DpcObjPlace;

static void DpcObj_InitClAct(void);
static void DpcObj_InitResources(DendouPCWork *wk);
static void DpcObj_FreeChars(DendouPCWork *wk, u32 id);
static void DpcObj_FreePalette(DendouPCWork *wk, u32 id);
static void DpcObj_FreeCellAnims(DendouPCWork *wk, u32 id);
static void DpcObj_FreeResources(DendouPCWork *wk);
static ClActor *DpcObj_AddActor(DendouPCWork *wk, const DpcObjSetup *setup);
static void DpcObj_AddButtons(DendouPCWork *wk);
static void DpcObj_DeleteActor(DendouPCWork *wk, u32 id);
static void DpcObj_DeleteActors(DendouPCWork *wk);
static void DpcObj_InitPageOam(DendouPCWork *wk);
static void DpcObj_ExitPageOam(DendouPCWork *wk);

static const DpcObjSetup sDpcObjButtonSetups[] = {
    { { 8, 168, 4, 0, 0 }, DPCOBJ_RES_BUTTON, DPCOBJ_RES_BUTTON, DPCOBJ_RES_BUTTON, 0, CLACT_SURFACE_MAIN },
    { { 88, 168, 5, 0, 0 }, DPCOBJ_RES_BUTTON, DPCOBJ_RES_BUTTON, DPCOBJ_RES_BUTTON, 0, CLACT_SURFACE_MAIN },
    { { 200, 168, 0, 0, 0 }, DPCOBJ_RES_BUTTON, DPCOBJ_RES_BUTTON, DPCOBJ_RES_BUTTON, 0, CLACT_SURFACE_MAIN },
    { { 232, 168, 1, 0, 0 }, DPCOBJ_RES_BUTTON, DPCOBJ_RES_BUTTON, DPCOBJ_RES_BUTTON, 0, CLACT_SURFACE_MAIN },
};

// The Pokémon; DpcObj_SetPokePos moves them onto the ring
static const DpcObjSetup sDpcObjPokeSetups[HALL_OF_FAME_TEAM_SIZE] = {
    { { 128, 132, 0, 0, 2 }, 0, 0, 0, 0, CLACT_SURFACE_MAIN }, { { 40, 124, 0, 1, 2 }, 1, 1, 1, 0, CLACT_SURFACE_MAIN },
    { { 224, 124, 0, 2, 2 }, 2, 2, 2, 0, CLACT_SURFACE_MAIN }, { { 128, 44, 0, 3, 2 }, 3, 3, 3, 0, CLACT_SURFACE_MAIN },
    { { 40, 52, 0, 4, 2 }, 4, 4, 4, 0, CLACT_SURFACE_MAIN },   { { 224, 52, 0, 5, 2 }, 5, 5, 5, 0, CLACT_SURFACE_MAIN },
};

// The places of a team's Pokémon by their number, the first at the front
static const DpcObjPlace sDpcObjPlaces[HALL_OF_FAME_TEAM_SIZE][HALL_OF_FAME_TEAM_SIZE] = {
    { { 90, 0 } },
    { { 90, 0 }, { 270, 12 } },
    { { 90, 0 }, { 210, 10 }, { 330, 10 } },
    { { 90, 0 }, { 180, 7 }, { 270, 12 }, { 0, 7 } },
    { { 90, 0 }, { 162, 8 }, { 234, 10 }, { 306, 10 }, { 18, 8 } },
    { { 90, 0 }, { 150, 8 }, { 210, 10 }, { 270, 12 }, { 330, 10 }, { 30, 8 } },
};

void DpcObj_Init(DendouPCWork *wk) {
    DpcObj_InitClAct();
    DpcObj_InitResources(wk);
    DpcObj_AddButtons(wk);
    DpcObj_InitPageOam(wk);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

void DpcObj_Exit(DendouPCWork *wk) {
    DpcObj_ExitPageOam(wk);
    DpcObj_DeleteActors(wk);
    DpcObj_FreeResources(wk);
    func_0204b758();
}

void DpcObj_Animate(DendouPCWork *wk) {
    u32 i;

    for (i = 0; i < DPCOBJ_MAX; i++) {
        if (wk->actors[i] != NULL && func_0204c534(wk->actors[i]) != TRUE) {
            func_0204c4e0(wk->actors[i], FX32_ONE);
        }
    }
    func_0204b794();
}

void DpcObj_SetVisible(DendouPCWork *wk, u32 id, BOOL visible) {
    if (wk->actors[id] != NULL) {
        func_0204c124(wk->actors[id], visible);
    }
}

void DpcObj_SetAnime(DendouPCWork *wk, u32 id, u32 sequence) {
    func_0204c4d4(wk->actors[id], 0);
    func_0204c488(wk->actors[id], sequence);
    func_0204c520(wk->actors[id], TRUE);
}

BOOL DpcObj_IsAnimating(DendouPCWork *wk, u32 id) {
    return func_0204c560(wk->actors[id]);
}

void DpcObj_SetPos(DendouPCWork *wk, u32 id, s16 x, s16 y) {
    ClActorPos pos;

    pos.x = x;
    pos.y = y;
    func_0204c140(wk->actors[id], &pos, CLACT_SURFACE_MAIN);
}

void DpcObj_GetPos(DendouPCWork *wk, u32 id, s16 *x, s16 *y) {
    ClActorPos pos;

    func_0204c178(wk->actors[id], &pos, CLACT_SURFACE_MAIN);
    *x = pos.x;
    *y = pos.y;
}

static void DpcObj_InitClAct(void) {
    ClActSysSetup setup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 28, DPCOBJ_RES_MAX, 28, 0, 16, 16 };

    ClActSys_Create(&setup, DpcMain_GetVramBanks(), HEAPID_DENDOU_PC);
}

static void DpcObj_InitResources(DendouPCWork *wk) {
    ArcTool *arc;
    u32 i;

    for (i = 0; i < DPCOBJ_RES_MAX; i++) {
        wk->chars[i] = DPCOBJ_RES_NONE;
    }
    for (i = 0; i < DPCOBJ_RES_MAX; i++) {
        wk->palettes[i] = DPCOBJ_RES_NONE;
    }
    for (i = 0; i < DPCOBJ_RES_MAX; i++) {
        wk->cellAnims[i] = DPCOBJ_RES_NONE;
    }

    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_TAIL(HEAPID_DENDOU_PC));
    wk->chars[DPCOBJ_RES_BUTTON] = func_0204b81c(arc, func_0202d814(), FALSE, CLACT_VRAM_MAIN, HEAPID_DENDOU_PC);
    wk->palettes[DPCOBJ_RES_BUTTON] =
        func_0204bba0(arc, func_0202d810(), CLACT_VRAM_MAIN, DPCOBJ_RES_BUTTON * 0x20, HEAPID_DENDOU_PC);
    wk->cellAnims[DPCOBJ_RES_BUTTON] =
        func_0204bde0(arc, func_0202d818(DPCOBJ_MAPPING_128K), func_0202d81c(DPCOBJ_MAPPING_128K), HEAPID_DENDOU_PC);
    GFL_ArcToolFree(arc);
}

static void DpcObj_FreeChars(DendouPCWork *wk, u32 id) {
    if (wk->chars[id] != DPCOBJ_RES_NONE) {
        func_0204b98c(wk->chars[id]);
        wk->chars[id] = DPCOBJ_RES_NONE;
    }
}

static void DpcObj_FreePalette(DendouPCWork *wk, u32 id) {
    if (wk->palettes[id] != DPCOBJ_RES_NONE) {
        func_0204bcd0(wk->palettes[id]);
        wk->palettes[id] = DPCOBJ_RES_NONE;
    }
}

static void DpcObj_FreeCellAnims(DendouPCWork *wk, u32 id) {
    if (wk->cellAnims[id] != DPCOBJ_RES_NONE) {
        func_0204be64(wk->cellAnims[id]);
        wk->cellAnims[id] = DPCOBJ_RES_NONE;
    }
}

static void DpcObj_FreeResources(DendouPCWork *wk) {
    u32 i;

    for (i = 0; i < DPCOBJ_RES_MAX; i++) {
        DpcObj_FreeChars(wk, i);
    }
    for (i = 0; i < DPCOBJ_RES_MAX; i++) {
        DpcObj_FreePalette(wk, i);
    }
    for (i = 0; i < DPCOBJ_RES_MAX; i++) {
        DpcObj_FreeCellAnims(wk, i);
    }
}

static ClActor *DpcObj_AddActor(DendouPCWork *wk, const DpcObjSetup *setup) {
    return func_0204c040(wk->unit, wk->chars[setup->chars], wk->palettes[setup->palette],
                         wk->cellAnims[setup->cellAnims], &setup->setup, setup->surface, HEAPID_DENDOU_PC);
}

static void DpcObj_AddButtons(DendouPCWork *wk) {
    // The SDK's u32 is unsigned long, which MWCC indexes with differently than unsigned int
    unsigned long i;

    wk->unit = func_0204bf1c(31, 0, HEAPID_DENDOU_PC);
    for (i = 0; i < DPCOBJ_MAX; i++) {
        wk->actors[i] = NULL;
    }
    for (i = DPCOBJ_ARROW_LEFT; i <= DPCOBJ_RETURN; i++) {
        wk->actors[i] = DpcObj_AddActor(wk, &sDpcObjButtonSetups[i - DPCOBJ_ARROW_LEFT]);
    }

    // A single page has no arrows
    if (wk->pageMax == 1) {
        DpcObj_SetVisible(wk, DPCOBJ_ARROW_LEFT, FALSE);
        DpcObj_SetVisible(wk, DPCOBJ_ARROW_RIGHT, FALSE);
    }
}

static void DpcObj_DeleteActor(DendouPCWork *wk, u32 id) {
    if (wk->actors[id] != NULL) {
        func_0204c108(wk->actors[id]);
        wk->actors[id] = NULL;
    }
}

static void DpcObj_DeleteActors(DendouPCWork *wk) {
    u32 i;

    for (i = 0; i < DPCOBJ_MAX; i++) {
        DpcObj_DeleteActor(wk, i);
    }
    func_0204bf98(wk->unit);
}

void DpcObj_LoadPokemon(DendouPCWork *wk) {
    ArcTool *arc;
    DendouPCTeam *team;
    HallOfFamePokemon *poke;
    DpcObjSetup setup;
    ClActor **actor;
    BOOL rare;
    u32 i;
    u16 id;

    for (i = DPCOBJ_POKE_A; i <= DPCOBJ_POKE_B + HALL_OF_FAME_TEAM_SIZE - 1; i++) {
        DpcObj_SetVisible(wk, i, FALSE);
    }

    team = &wk->teams[wk->page];
    arc = MakePokeGraArcHandle(HEAPID_TAIL(HEAPID_DENDOU_PC));

    for (i = 0; i < team->count; i++) {
        setup = sDpcObjPokeSetups[i];
        poke = &team->pokemon[i];
        rare = PML_UtilPIDIsRare(poke->trainerId, poke->personality);

        if (wk->objSwap == 1) {
            setup.chars += HALL_OF_FAME_TEAM_SIZE;
            setup.palette += HALL_OF_FAME_TEAM_SIZE;
            setup.cellAnims += HALL_OF_FAME_TEAM_SIZE;
            id = i + DPCOBJ_POKE_B;
            actor = &wk->actors[id];
        } else {
            id = i;
            actor = &wk->actors[id];
        }

        wk->chars[setup.chars] = PokeGra_LoadClActChars(arc, poke->species, poke->form, poke->sex, rare, 0, FALSE,
                                                        poke->personality, CLACT_VRAM_MAIN, HEAPID_DENDOU_PC);
        wk->palettes[setup.palette] = PokeGra_LoadClActPalette(arc, poke->species, poke->form, poke->sex, rare, 0,
                                                               FALSE, CLACT_VRAM_MAIN, id * 0x20, HEAPID_DENDOU_PC);
        wk->cellAnims[setup.cellAnims] =
            PokeGra_LoadClActCellAnims(poke->species, poke->form, poke->sex, rare, 0, FALSE,
                                       DPCOBJ_MAPPING_128K, CLACT_VRAM_MAIN, HEAPID_DENDOU_PC);
        *actor = DpcObj_AddActor(wk, &setup);

        DpcObj_SetPokePos(wk, id, sDpcObjPlaces[team->count - 1][i].angle);
        wk->angle[i] = sDpcObjPlaces[team->count - 1][i].angle;
    }

    PaletteFade_LoadFromVRAM(wk->palFade, PALFADE_VRAM_MAIN_OBJ, 0, 0x200);
    GFL_ArcToolFree(arc);

    // Free the set that the last page used
    if (wk->objSwap == 1) {
        for (i = DPCOBJ_POKE_A; i <= DPCOBJ_POKE_A + HALL_OF_FAME_TEAM_SIZE - 1; i++) {
            DpcObj_DeleteActor(wk, i);
        }
        for (i = DPCOBJ_POKE_A; i <= DPCOBJ_POKE_A + HALL_OF_FAME_TEAM_SIZE - 1; i++) {
            DpcObj_FreeChars(wk, i);
        }
        for (i = DPCOBJ_POKE_A; i <= DPCOBJ_POKE_A + HALL_OF_FAME_TEAM_SIZE - 1; i++) {
            DpcObj_FreePalette(wk, i);
        }
        for (i = DPCOBJ_POKE_A; i <= DPCOBJ_POKE_A + HALL_OF_FAME_TEAM_SIZE - 1; i++) {
            DpcObj_FreeCellAnims(wk, i);
        }
    } else {
        for (i = DPCOBJ_POKE_B; i <= DPCOBJ_POKE_B + HALL_OF_FAME_TEAM_SIZE - 1; i++) {
            DpcObj_DeleteActor(wk, i);
        }
        for (i = DPCOBJ_POKE_B; i <= DPCOBJ_POKE_B + HALL_OF_FAME_TEAM_SIZE - 1; i++) {
            DpcObj_FreeChars(wk, i);
        }
        for (i = DPCOBJ_POKE_B; i <= DPCOBJ_POKE_B + HALL_OF_FAME_TEAM_SIZE - 1; i++) {
            DpcObj_FreePalette(wk, i);
        }
        for (i = DPCOBJ_POKE_B; i <= DPCOBJ_POKE_B + HALL_OF_FAME_TEAM_SIZE - 1; i++) {
            DpcObj_FreeCellAnims(wk, i);
        }
    }
    wk->objSwap ^= 1;

    DpcObj_SortPriority(wk);
}

u32 DpcObj_GetPokeBase(DendouPCWork *wk) {
    if (wk->objSwap == 0) {
        return DPCOBJ_POKE_B;
    }
    return DPCOBJ_POKE_A;
}

void DpcObj_SetPokePos(DendouPCWork *wk, u32 id, s32 angle) {
    fx32 x = FX_MUL(func_02044388(angle), DPCOBJ_RING_RADIUS_X * FX32_ONE);
    fx32 y = FX_MUL(func_02044360(angle), DPCOBJ_RING_RADIUS_Y * FX32_ONE);

    DpcObj_SetPos(wk, id, FX_Whole((x + DPCOBJ_RING_X * FX32_ONE) & 0x7ffff000),
                  FX_Whole((y + DPCOBJ_RING_Y * FX32_ONE) & 0x7ffff000));
}

void DpcObj_SortPriority(DendouPCWork *wk) {
    u8 ids[HALL_OF_FAME_TEAM_SIZE];
    s16 x, y;
    s16 otherX, otherY;
    u32 base;
    u8 i, j;
    u8 tmp;

    base = DpcObj_GetPokeBase(wk);
    for (i = 0; i < HALL_OF_FAME_TEAM_SIZE; i++) {
        ids[i] = base + i;
    }

    // The lower on the screen, the nearer
    for (i = 0; i < wk->teams[wk->page].count - 1; i++) {
        DpcObj_GetPos(wk, ids[i], &x, &y);
        for (j = i + 1; j < wk->teams[wk->page].count; j++) {
            DpcObj_GetPos(wk, ids[j], &otherX, &otherY);
            if (y < otherY) {
                tmp = ids[i];
                ids[i] = ids[j];
                ids[j] = tmp;
                y = otherY;
            }
        }
    }

    for (i = 0; i < wk->teams[wk->page].count; i++) {
        if (wk->actors[ids[i]] != NULL) {
            func_0204c438(wk->actors[ids[i]], i);
        }
    }
}

void DpcObj_SetPokeFade(DendouPCWork *wk, BOOL on) {
    u16 pos = wk->pokePos;
    u16 i;

    for (i = 0; i < wk->teams[wk->page].count; i++) {
        if (on == TRUE) {
            wk->fade[pos] = sDpcObjPlaces[wk->teams[wk->page].count - 1][i].fade;
            wk->fadeNow[pos] = wk->fade[pos];
            pos++;
            if (pos >= wk->teams[wk->page].count) {
                pos = 0;
            }
        } else {
            wk->fade[i] = 0;
            wk->fadeNow[i] = 0;
        }
    }
}

static void DpcObj_InitPageOam(DendouPCWork *wk) {
    BmpOamActorSetup setup;

    wk->bmpOam = BmpOam_Init(HEAPID_DENDOU_PC, wk->unit);
    wk->pageBitmap = GFL_BitmapCreate(5, 3, 0x20, HEAPID_DENDOU_PC);

    setup.bitmap = wk->pageBitmap;
    setup.x = 40;
    setup.y = 168;
    setup.palette = wk->palettes[DPCOBJ_RES_BUTTON];
    setup.paletteOffset = 1;
    setup.priority = 0;
    setup.bgPriority = 0;
    setup.surface = 0xffff;
    setup.vramType = CLACT_VRAM_MAIN;
    wk->pageActor = BmpOam_ActorAdd(wk->bmpOam, &setup);

    if (wk->pageMax == 1) {
        BmpOam_ActorSetDrawEnable(wk->pageActor, FALSE);
    }
}

static void DpcObj_ExitPageOam(DendouPCWork *wk) {
    BmpOam_ActorDel(wk->pageActor);
    GFL_BitmapFree(wk->pageBitmap);
    BmpOam_Exit(wk->bmpOam);
}
