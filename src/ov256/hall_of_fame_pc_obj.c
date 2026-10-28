#include "types.h"
#include "app/hall_of_fame_pc_main.h"
#include "app/hall_of_fame_pc_obj.h"
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

#define HALL_OF_FAME_PC_OBJ_RES_NONE 0xffffffff

// The common cells for the screen's OBJ VRAM mapping, 128 KB
#define HALL_OF_FAME_PC_OBJ_MAPPING_128K 2

// The ring's center and radii, in pixels
#define HALL_OF_FAME_PC_OBJ_RING_X 128
#define HALL_OF_FAME_PC_OBJ_RING_Y 88
#define HALL_OF_FAME_PC_OBJ_RING_RADIUS_X 100
#define HALL_OF_FAME_PC_OBJ_RING_RADIUS_Y 44

// An actor and the resources it uses
typedef struct {
    ClActorSetup setup;
    u32 chars;
    u32 palette;
    u32 cellAnims;
    u16 unk14;
    u16 surface;
} HallOfFamePCObjSetup;

// Where a Pokémon is on the ring, in degrees, and how dark it is there
typedef struct {
    u16 angle;
    u16 fade;
} HallOfFamePCObjPlace;

static void HallOfFamePCObj_InitClAct(void);
static void HallOfFamePCObj_InitResources(HallOfFamePCWork *wk);
static void HallOfFamePCObj_FreeChars(HallOfFamePCWork *wk, u32 id);
static void HallOfFamePCObj_FreePalette(HallOfFamePCWork *wk, u32 id);
static void HallOfFamePCObj_FreeCellAnims(HallOfFamePCWork *wk, u32 id);
static void HallOfFamePCObj_FreeResources(HallOfFamePCWork *wk);
static ClActor *HallOfFamePCObj_AddActor(HallOfFamePCWork *wk, const HallOfFamePCObjSetup *setup);
static void HallOfFamePCObj_AddButtons(HallOfFamePCWork *wk);
static void HallOfFamePCObj_DeleteActor(HallOfFamePCWork *wk, u32 id);
static void HallOfFamePCObj_DeleteActors(HallOfFamePCWork *wk);
static void HallOfFamePCObj_InitPageOam(HallOfFamePCWork *wk);
static void HallOfFamePCObj_ExitPageOam(HallOfFamePCWork *wk);

static const HallOfFamePCObjSetup sHallOfFamePCObjButtonSetups[] = {
    { { 8, 168, 4, 0, 0 },
      HALL_OF_FAME_PC_OBJ_RES_BUTTON,
      HALL_OF_FAME_PC_OBJ_RES_BUTTON,
      HALL_OF_FAME_PC_OBJ_RES_BUTTON,
      0,
      CLACT_SURFACE_MAIN },
    { { 88, 168, 5, 0, 0 },
      HALL_OF_FAME_PC_OBJ_RES_BUTTON,
      HALL_OF_FAME_PC_OBJ_RES_BUTTON,
      HALL_OF_FAME_PC_OBJ_RES_BUTTON,
      0,
      CLACT_SURFACE_MAIN },
    { { 200, 168, 0, 0, 0 },
      HALL_OF_FAME_PC_OBJ_RES_BUTTON,
      HALL_OF_FAME_PC_OBJ_RES_BUTTON,
      HALL_OF_FAME_PC_OBJ_RES_BUTTON,
      0,
      CLACT_SURFACE_MAIN },
    { { 232, 168, 1, 0, 0 },
      HALL_OF_FAME_PC_OBJ_RES_BUTTON,
      HALL_OF_FAME_PC_OBJ_RES_BUTTON,
      HALL_OF_FAME_PC_OBJ_RES_BUTTON,
      0,
      CLACT_SURFACE_MAIN },
};

// The Pokémon; HallOfFamePCObj_SetPokePos moves them onto the ring
static const HallOfFamePCObjSetup sHallOfFamePCObjPokeSetups[HALL_OF_FAME_TEAM_SIZE] = {
    { { 128, 132, 0, 0, 2 }, 0, 0, 0, 0, CLACT_SURFACE_MAIN }, { { 40, 124, 0, 1, 2 }, 1, 1, 1, 0, CLACT_SURFACE_MAIN },
    { { 224, 124, 0, 2, 2 }, 2, 2, 2, 0, CLACT_SURFACE_MAIN }, { { 128, 44, 0, 3, 2 }, 3, 3, 3, 0, CLACT_SURFACE_MAIN },
    { { 40, 52, 0, 4, 2 }, 4, 4, 4, 0, CLACT_SURFACE_MAIN },   { { 224, 52, 0, 5, 2 }, 5, 5, 5, 0, CLACT_SURFACE_MAIN },
};

// The places of a team's Pokémon by their number, the first at the front
static const HallOfFamePCObjPlace sHallOfFamePCObjPlaces[HALL_OF_FAME_TEAM_SIZE][HALL_OF_FAME_TEAM_SIZE] = {
    { { 90, 0 } },
    { { 90, 0 }, { 270, 12 } },
    { { 90, 0 }, { 210, 10 }, { 330, 10 } },
    { { 90, 0 }, { 180, 7 }, { 270, 12 }, { 0, 7 } },
    { { 90, 0 }, { 162, 8 }, { 234, 10 }, { 306, 10 }, { 18, 8 } },
    { { 90, 0 }, { 150, 8 }, { 210, 10 }, { 270, 12 }, { 330, 10 }, { 30, 8 } },
};

void HallOfFamePCObj_Init(HallOfFamePCWork *wk) {
    HallOfFamePCObj_InitClAct();
    HallOfFamePCObj_InitResources(wk);
    HallOfFamePCObj_AddButtons(wk);
    HallOfFamePCObj_InitPageOam(wk);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

void HallOfFamePCObj_Exit(HallOfFamePCWork *wk) {
    HallOfFamePCObj_ExitPageOam(wk);
    HallOfFamePCObj_DeleteActors(wk);
    HallOfFamePCObj_FreeResources(wk);
    ClActSys_Delete();
}

void HallOfFamePCObj_Animate(HallOfFamePCWork *wk) {
    u32 i;

    for (i = 0; i < HALL_OF_FAME_PC_OBJ_MAX; i++) {
        if (wk->actors[i] != NULL && ClActor_IsAutoAnim(wk->actors[i]) != TRUE) {
            ClActor_SetAnimSpeed(wk->actors[i], FX32_ONE);
        }
    }
    ClActSys_Draw();
}

void HallOfFamePCObj_SetVisible(HallOfFamePCWork *wk, u32 id, BOOL visible) {
    if (wk->actors[id] != NULL) {
        ClActor_SetVisible(wk->actors[id], visible);
    }
}

void HallOfFamePCObj_SetAnime(HallOfFamePCWork *wk, u32 id, u32 sequence) {
    ClActor_SetAnimTime(wk->actors[id], 0);
    ClActor_SetSequence(wk->actors[id], sequence);
    ClActor_SetAutoAnim(wk->actors[id], TRUE);
}

BOOL HallOfFamePCObj_IsAnimating(HallOfFamePCWork *wk, u32 id) {
    return ClActor_IsAnimActive(wk->actors[id]);
}

void HallOfFamePCObj_SetPos(HallOfFamePCWork *wk, u32 id, s16 x, s16 y) {
    ClActorPos pos;

    pos.x = x;
    pos.y = y;
    ClActor_SetSurfacePos(wk->actors[id], &pos, CLACT_SURFACE_MAIN);
}

void HallOfFamePCObj_GetPos(HallOfFamePCWork *wk, u32 id, s16 *x, s16 *y) {
    ClActorPos pos;

    ClActor_GetSurfacePos(wk->actors[id], &pos, CLACT_SURFACE_MAIN);
    *x = pos.x;
    *y = pos.y;
}

static void HallOfFamePCObj_InitClAct(void) {
    ClActSysSetup setup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 28, HALL_OF_FAME_PC_OBJ_RES_MAX, 28, 0, 16, 16 };

    ClActSys_Create(&setup, HallOfFamePCMain_GetVramBanks(), HEAPID_HALL_OF_FAME_PC);
}

static void HallOfFamePCObj_InitResources(HallOfFamePCWork *wk) {
    ArcTool *arc;
    u32 i;

    for (i = 0; i < HALL_OF_FAME_PC_OBJ_RES_MAX; i++) {
        wk->chars[i] = HALL_OF_FAME_PC_OBJ_RES_NONE;
    }
    for (i = 0; i < HALL_OF_FAME_PC_OBJ_RES_MAX; i++) {
        wk->palettes[i] = HALL_OF_FAME_PC_OBJ_RES_NONE;
    }
    for (i = 0; i < HALL_OF_FAME_PC_OBJ_RES_MAX; i++) {
        wk->cellAnims[i] = HALL_OF_FAME_PC_OBJ_RES_NONE;
    }

    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_TAIL(HEAPID_HALL_OF_FAME_PC));
    wk->chars[HALL_OF_FAME_PC_OBJ_RES_BUTTON] =
        ClActSys_LoadChars(arc, AppMenuCommon_GetTouchBarCharFile(), FALSE, CLACT_VRAM_MAIN, HEAPID_HALL_OF_FAME_PC);
    wk->palettes[HALL_OF_FAME_PC_OBJ_RES_BUTTON] =
        ClActSys_LoadPltt(arc, AppMenuCommon_GetTouchBarPaletteFile(), CLACT_VRAM_MAIN,
                          HALL_OF_FAME_PC_OBJ_RES_BUTTON * 0x20, HEAPID_HALL_OF_FAME_PC);
    wk->cellAnims[HALL_OF_FAME_PC_OBJ_RES_BUTTON] = ClActSys_LoadCellAnims(
        arc, AppMenuCommon_GetTouchBarCellFile(HALL_OF_FAME_PC_OBJ_MAPPING_128K),
        AppMenuCommon_GetTouchBarAnimFile(HALL_OF_FAME_PC_OBJ_MAPPING_128K), HEAPID_HALL_OF_FAME_PC);
    GFL_ArcToolFree(arc);
}

static void HallOfFamePCObj_FreeChars(HallOfFamePCWork *wk, u32 id) {
    if (wk->chars[id] != HALL_OF_FAME_PC_OBJ_RES_NONE) {
        ClActSys_FreeChars(wk->chars[id]);
        wk->chars[id] = HALL_OF_FAME_PC_OBJ_RES_NONE;
    }
}

static void HallOfFamePCObj_FreePalette(HallOfFamePCWork *wk, u32 id) {
    if (wk->palettes[id] != HALL_OF_FAME_PC_OBJ_RES_NONE) {
        ClActSys_FreePltt(wk->palettes[id]);
        wk->palettes[id] = HALL_OF_FAME_PC_OBJ_RES_NONE;
    }
}

static void HallOfFamePCObj_FreeCellAnims(HallOfFamePCWork *wk, u32 id) {
    if (wk->cellAnims[id] != HALL_OF_FAME_PC_OBJ_RES_NONE) {
        ClActSys_FreeCellAnims(wk->cellAnims[id]);
        wk->cellAnims[id] = HALL_OF_FAME_PC_OBJ_RES_NONE;
    }
}

static void HallOfFamePCObj_FreeResources(HallOfFamePCWork *wk) {
    u32 i;

    for (i = 0; i < HALL_OF_FAME_PC_OBJ_RES_MAX; i++) {
        HallOfFamePCObj_FreeChars(wk, i);
    }
    for (i = 0; i < HALL_OF_FAME_PC_OBJ_RES_MAX; i++) {
        HallOfFamePCObj_FreePalette(wk, i);
    }
    for (i = 0; i < HALL_OF_FAME_PC_OBJ_RES_MAX; i++) {
        HallOfFamePCObj_FreeCellAnims(wk, i);
    }
}

static ClActor *HallOfFamePCObj_AddActor(HallOfFamePCWork *wk, const HallOfFamePCObjSetup *setup) {
    return ClActor_Create(wk->unit, wk->chars[setup->chars], wk->palettes[setup->palette],
                          wk->cellAnims[setup->cellAnims], &setup->setup, setup->surface, HEAPID_HALL_OF_FAME_PC);
}

static void HallOfFamePCObj_AddButtons(HallOfFamePCWork *wk) {
    u32 i;

    wk->unit = ClActUnit_Create(31, 0, HEAPID_HALL_OF_FAME_PC);
    for (i = 0; i < HALL_OF_FAME_PC_OBJ_MAX; i++) {
        wk->actors[i] = NULL;
    }
    for (i = HALL_OF_FAME_PC_OBJ_ARROW_LEFT; i <= HALL_OF_FAME_PC_OBJ_RETURN; i++) {
        wk->actors[i] = HallOfFamePCObj_AddActor(wk, &sHallOfFamePCObjButtonSetups[i - HALL_OF_FAME_PC_OBJ_ARROW_LEFT]);
    }

    // A single page has no arrows
    if (wk->pageMax == 1) {
        HallOfFamePCObj_SetVisible(wk, HALL_OF_FAME_PC_OBJ_ARROW_LEFT, FALSE);
        HallOfFamePCObj_SetVisible(wk, HALL_OF_FAME_PC_OBJ_ARROW_RIGHT, FALSE);
    }
}

static void HallOfFamePCObj_DeleteActor(HallOfFamePCWork *wk, u32 id) {
    if (wk->actors[id] != NULL) {
        ClActor_Delete(wk->actors[id]);
        wk->actors[id] = NULL;
    }
}

static void HallOfFamePCObj_DeleteActors(HallOfFamePCWork *wk) {
    u32 i;

    for (i = 0; i < HALL_OF_FAME_PC_OBJ_MAX; i++) {
        HallOfFamePCObj_DeleteActor(wk, i);
    }
    ClActUnit_Delete(wk->unit);
}

void HallOfFamePCObj_LoadPokemon(HallOfFamePCWork *wk) {
    ArcTool *arc;
    HallOfFamePCTeam *team;
    HallOfFamePokemon *poke;
    HallOfFamePCObjSetup setup;
    ClActor **actor;
    BOOL rare;
    u32 i;
    u16 id;

    for (i = HALL_OF_FAME_PC_OBJ_POKE_A; i <= HALL_OF_FAME_PC_OBJ_POKE_B + HALL_OF_FAME_TEAM_SIZE - 1; i++) {
        HallOfFamePCObj_SetVisible(wk, i, FALSE);
    }

    team = &wk->teams[wk->page];
    arc = MakePokeGraArcHandle(HEAPID_TAIL(HEAPID_HALL_OF_FAME_PC));

    for (i = 0; i < team->count; i++) {
        setup = sHallOfFamePCObjPokeSetups[i];
        poke = &team->pokemon[i];
        rare = PML_UtilPIDIsRare(poke->trainerId, poke->personality);

        if (wk->objSwap == 1) {
            setup.chars += HALL_OF_FAME_TEAM_SIZE;
            setup.palette += HALL_OF_FAME_TEAM_SIZE;
            setup.cellAnims += HALL_OF_FAME_TEAM_SIZE;
            id = i + HALL_OF_FAME_PC_OBJ_POKE_B;
            actor = &wk->actors[id];
        } else {
            id = i;
            actor = &wk->actors[id];
        }

        wk->chars[setup.chars] = PokeGra_LoadClActChars(arc, poke->species, poke->form, poke->sex, rare, 0, FALSE,
                                                        poke->personality, CLACT_VRAM_MAIN, HEAPID_HALL_OF_FAME_PC);
        wk->palettes[setup.palette] =
            PokeGra_LoadClActPalette(arc, poke->species, poke->form, poke->sex, rare, 0, FALSE, CLACT_VRAM_MAIN,
                                     id * 0x20, HEAPID_HALL_OF_FAME_PC);
        wk->cellAnims[setup.cellAnims] =
            PokeGra_LoadClActCellAnims(poke->species, poke->form, poke->sex, rare, 0, FALSE,
                                       HALL_OF_FAME_PC_OBJ_MAPPING_128K, CLACT_VRAM_MAIN, HEAPID_HALL_OF_FAME_PC);
        *actor = HallOfFamePCObj_AddActor(wk, &setup);

        HallOfFamePCObj_SetPokePos(wk, id, sHallOfFamePCObjPlaces[team->count - 1][i].angle);
        wk->angle[i] = sHallOfFamePCObjPlaces[team->count - 1][i].angle;
    }

    PaletteFade_LoadFromVRAM(wk->palFade, PALFADE_VRAM_MAIN_OBJ, 0, 0x200);
    GFL_ArcToolFree(arc);

    // Free the set that the last page used
    if (wk->objSwap == 1) {
        for (i = HALL_OF_FAME_PC_OBJ_POKE_A; i <= HALL_OF_FAME_PC_OBJ_POKE_A + HALL_OF_FAME_TEAM_SIZE - 1; i++) {
            HallOfFamePCObj_DeleteActor(wk, i);
        }
        for (i = HALL_OF_FAME_PC_OBJ_POKE_A; i <= HALL_OF_FAME_PC_OBJ_POKE_A + HALL_OF_FAME_TEAM_SIZE - 1; i++) {
            HallOfFamePCObj_FreeChars(wk, i);
        }
        for (i = HALL_OF_FAME_PC_OBJ_POKE_A; i <= HALL_OF_FAME_PC_OBJ_POKE_A + HALL_OF_FAME_TEAM_SIZE - 1; i++) {
            HallOfFamePCObj_FreePalette(wk, i);
        }
        for (i = HALL_OF_FAME_PC_OBJ_POKE_A; i <= HALL_OF_FAME_PC_OBJ_POKE_A + HALL_OF_FAME_TEAM_SIZE - 1; i++) {
            HallOfFamePCObj_FreeCellAnims(wk, i);
        }
    } else {
        for (i = HALL_OF_FAME_PC_OBJ_POKE_B; i <= HALL_OF_FAME_PC_OBJ_POKE_B + HALL_OF_FAME_TEAM_SIZE - 1; i++) {
            HallOfFamePCObj_DeleteActor(wk, i);
        }
        for (i = HALL_OF_FAME_PC_OBJ_POKE_B; i <= HALL_OF_FAME_PC_OBJ_POKE_B + HALL_OF_FAME_TEAM_SIZE - 1; i++) {
            HallOfFamePCObj_FreeChars(wk, i);
        }
        for (i = HALL_OF_FAME_PC_OBJ_POKE_B; i <= HALL_OF_FAME_PC_OBJ_POKE_B + HALL_OF_FAME_TEAM_SIZE - 1; i++) {
            HallOfFamePCObj_FreePalette(wk, i);
        }
        for (i = HALL_OF_FAME_PC_OBJ_POKE_B; i <= HALL_OF_FAME_PC_OBJ_POKE_B + HALL_OF_FAME_TEAM_SIZE - 1; i++) {
            HallOfFamePCObj_FreeCellAnims(wk, i);
        }
    }
    wk->objSwap ^= 1;

    HallOfFamePCObj_SortPriority(wk);
}

u32 HallOfFamePCObj_GetPokeBase(HallOfFamePCWork *wk) {
    if (wk->objSwap == 0) {
        return HALL_OF_FAME_PC_OBJ_POKE_B;
    }
    return HALL_OF_FAME_PC_OBJ_POKE_A;
}

void HallOfFamePCObj_SetPokePos(HallOfFamePCWork *wk, u32 id, s32 angle) {
    fx32 x = FX_MUL(GFL_MathCosDegWrap(angle), HALL_OF_FAME_PC_OBJ_RING_RADIUS_X * FX32_ONE);
    fx32 y = FX_MUL(GFL_MathSinDegWrap(angle), HALL_OF_FAME_PC_OBJ_RING_RADIUS_Y * FX32_ONE);

    HallOfFamePCObj_SetPos(wk, id, FX_Whole((x + HALL_OF_FAME_PC_OBJ_RING_X * FX32_ONE) & 0x7ffff000),
                           FX_Whole((y + HALL_OF_FAME_PC_OBJ_RING_Y * FX32_ONE) & 0x7ffff000));
}

void HallOfFamePCObj_SortPriority(HallOfFamePCWork *wk) {
    u8 ids[HALL_OF_FAME_TEAM_SIZE];
    s16 x, y;
    s16 otherX, otherY;
    u32 base;
    u8 i, j;
    u8 tmp;

    base = HallOfFamePCObj_GetPokeBase(wk);
    for (i = 0; i < HALL_OF_FAME_TEAM_SIZE; i++) {
        ids[i] = base + i;
    }

    // The lower on the screen, the nearer
    for (i = 0; i < wk->teams[wk->page].count - 1; i++) {
        HallOfFamePCObj_GetPos(wk, ids[i], &x, &y);
        for (j = i + 1; j < wk->teams[wk->page].count; j++) {
            HallOfFamePCObj_GetPos(wk, ids[j], &otherX, &otherY);
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
            ClActor_SetPriority(wk->actors[ids[i]], i);
        }
    }
}

void HallOfFamePCObj_SetPokeFade(HallOfFamePCWork *wk, BOOL on) {
    u16 pos = wk->pokePos;
    u16 i;

    for (i = 0; i < wk->teams[wk->page].count; i++) {
        if (on == TRUE) {
            wk->fade[pos] = sHallOfFamePCObjPlaces[wk->teams[wk->page].count - 1][i].fade;
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

static void HallOfFamePCObj_InitPageOam(HallOfFamePCWork *wk) {
    BmpOamActorSetup setup;

    wk->bmpOam = BmpOam_Init(HEAPID_HALL_OF_FAME_PC, wk->unit);
    wk->pageBitmap = GFL_BitmapCreate(5, 3, 0x20, HEAPID_HALL_OF_FAME_PC);

    setup.bitmap = wk->pageBitmap;
    setup.x = 40;
    setup.y = 168;
    setup.palette = wk->palettes[HALL_OF_FAME_PC_OBJ_RES_BUTTON];
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

static void HallOfFamePCObj_ExitPageOam(HallOfFamePCWork *wk) {
    BmpOam_ActorDel(wk->pageActor);
    GFL_BitmapFree(wk->pageBitmap);
    BmpOam_Exit(wk->bmpOam);
}
