#include "types.h"
#include "app/pokedex_common.h"
#include "app/pokedex_list_main.h"
#include "app/pokedex_list_obj.h"
#include "app/ui/frame_list.h"
#include "constants/arc.h"
#include "constants/species.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nnsys/g2d.h"
#include "pml/personal.h"
#include "pml/poke_graphic.h"
#include "save/pokedex_savedata.h"
#include "system/app_menu_common.h"
#include "system/blink_palanm.h"
#include "system/game_data.h"
#include "system/palanm.h"
#include "system/poke_icon.h"

// The actors of the Pokédex's list: the touch bar's buttons, the scroll bar, the buttons of the Pokédexes, the icons
// of the rows and the picture of the species under the cursor. The ROM doesn't name this file; pokedex_list_obj.c is a
// guess after the search screen's pokedex_search_obj.c

// The resources: the icons of each screen have their own characters, and the two pictures their own of each kind
enum {
    CHAR_RES_ICON,
    CHAR_RES_ICON_SUB = CHAR_RES_ICON + POKEDEX_LIST_ICON_COUNT,
    CHAR_RES_MON = CHAR_RES_ICON_SUB + POKEDEX_LIST_ICON_COUNT,
    CHAR_RES_TOUCHBAR = CHAR_RES_MON + 2,
    CHAR_RES_LIST,
};

enum {
    RES_ICON,
    RES_ICON_SUB,
    RES_MON,
    RES_TOUCHBAR = RES_MON + 2,
    RES_LIST,
};

// The window that an actor's shape cuts out of the blending, which lib/nitro's GX_OAM_MODE_* doesn't have yet
#define OAM_MODE_OBJWND 2

// Where the icons are: their x, the rows' height and the height of the top screen's list
#define ICON_X 112
#define ROW_HEIGHT 24
#define TOP_LIST_HEIGHT 192

// The palettes of the two pictures
#define MON_PALETTE_OFFSET (11 * 0x20)

static void PokedexListObj_InitSystem(void);
static void PokedexListObj_LoadResources(PokedexListWork *wk);
static void PokedexListObj_InitBlinkAnm(PokedexListWork *wk);
static void PokedexListObj_ExitBlinkAnm(PokedexListWork *wk);
static void PokedexListObj_FreeResources(PokedexListWork *wk);
static void PokedexListObj_CreateActors(PokedexListWork *wk);
static void PokedexListObj_DeleteActor(PokedexListWork *wk, u32 id);
static void PokedexListObj_DeleteActors(PokedexListWork *wk);
static void PokedexListObj_CreateIcons(PokedexListWork *wk);
static void PokedexListObj_LoadIcon(PokedexListWork *wk, u32 id, u16 species, u16 form, u16 sex, BOOL touchScreen);
static u32 PokedexListObj_FindFreeIcon(PokedexListWork *wk, BOOL touchScreen);

static const PokedexActorTemplate sPokedexListActorTemplates[] = {
    { { 252, 12, 2, 0, 1 }, CHAR_RES_LIST, RES_LIST, RES_LIST, 0, CLACT_SURFACE_MAIN },
    { { 252, 84, 3, 10, 1 }, CHAR_RES_LIST, RES_LIST, RES_LIST, 0, CLACT_SURFACE_MAIN },
    { { 48, 48, 6, 10, 1 }, CHAR_RES_LIST, RES_LIST, RES_LIST, 0, CLACT_SURFACE_MAIN },
    { { 32, 110, 13, 10, 1 }, CHAR_RES_LIST, RES_LIST, RES_LIST, 0, CLACT_SURFACE_MAIN },
    { { 32, 133, 13, 10, 1 }, CHAR_RES_LIST, RES_LIST, RES_LIST, 0, CLACT_SURFACE_MAIN },
    { { 32, 156, 13, 10, 1 }, CHAR_RES_LIST, RES_LIST, RES_LIST, 0, CLACT_SURFACE_MAIN },
    { { 232, 168, 1, 0, 1 }, CHAR_RES_TOUCHBAR, RES_TOUCHBAR, RES_TOUCHBAR, 0, CLACT_SURFACE_MAIN },
    { { 208, 168, 0, 0, 1 }, CHAR_RES_TOUCHBAR, RES_TOUCHBAR, RES_TOUCHBAR, 0, CLACT_SURFACE_MAIN },
    { { 188, 172, 6, 0, 1 }, CHAR_RES_TOUCHBAR, RES_TOUCHBAR, RES_TOUCHBAR, 0, CLACT_SURFACE_MAIN },
    { { 160, 168, 5, 0, 1 }, CHAR_RES_TOUCHBAR, RES_TOUCHBAR, RES_TOUCHBAR, 0, CLACT_SURFACE_MAIN },
    { { 136, 168, 4, 0, 1 }, CHAR_RES_TOUCHBAR, RES_TOUCHBAR, RES_TOUCHBAR, 0, CLACT_SURFACE_MAIN },
    { { 36, 180, 1, 0, 1 }, CHAR_RES_LIST, RES_LIST, RES_LIST, 0, CLACT_SURFACE_MAIN },
    { { 104, 180, 0, 0, 1 }, CHAR_RES_LIST, RES_LIST, RES_LIST, 0, CLACT_SURFACE_MAIN },
};

void PokedexListObj_Init(PokedexListWork *wk) {
    PokedexListObj_InitSystem();
    PokedexListObj_LoadResources(wk);
    PokedexListObj_InitBlinkAnm(wk);
    PokedexListObj_CreateActors(wk);
    PokedexListObj_CreateIcons(wk);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

void PokedexListObj_Exit(PokedexListWork *wk) {
    PokedexListObj_ExitBlinkAnm(wk);
    PokedexListObj_DeleteActors(wk);
    PokedexListObj_FreeResources(wk);
    ClActSys_Delete();
}

void PokedexListObj_Update(PokedexListWork *wk) {
    u32 i;

    if (wk->blinkStopped == FALSE) {
        BlinkPalAnm_Main(wk->blinkAnm);
    }
    for (i = 0; i < POKEDEX_LIST_OBJ_COUNT; i++) {
        if (wk->actors[i] != NULL && ClActor_IsAutoAnim(wk->actors[i]) != TRUE) {
            ClActor_SetAnimSpeed(wk->actors[i], FX32_ONE);
        }
    }
    ClActSys_Draw();
}

void PokedexListObj_SetAnmSeq(PokedexListWork *wk, u32 id, u32 anm) {
    ClActor_SetAnimTime(wk->actors[id], 0);
    ClActor_SetSequence(wk->actors[id], anm);
    ClActor_StartAnim(wk->actors[id]);
}

void PokedexListObj_EndAnm(PokedexListWork *wk, u32 id) {
    ClActor_StopAnim(wk->actors[id]);
}

void PokedexListObj_SetAnm(PokedexListWork *wk, u32 id, u32 anm) {
    PokedexListObj_SetAnmSeq(wk, id, anm);
    ClActor_SetAutoAnim(wk->actors[id], TRUE);
}

u32 PokedexListObj_GetAnm(PokedexListWork *wk, u32 id) {
    return ClActor_GetSequence(wk->actors[id]);
}

BOOL PokedexListObj_IsAnmPlaying(PokedexListWork *wk, u32 id) {
    return ClActor_IsAnimActive(wk->actors[id]);
}

void PokedexListObj_SetVisible(PokedexListWork *wk, u32 id, BOOL visible) {
    if (wk->actors[id] != NULL) {
        ClActor_SetVisible(wk->actors[id], visible);
    }
}

BOOL PokedexListObj_IsVisible(PokedexListWork *wk, u32 id) {
    return ClActor_IsVisible(wk->actors[id]);
}

void PokedexListObj_SetBlend(PokedexListWork *wk, u32 id, BOOL blend) {
    if (blend == TRUE) {
        ClActor_SetObjMode(wk->actors[id], GX_OAM_MODE_XLU);
    } else {
        ClActor_SetObjMode(wk->actors[id], GX_OAM_MODE_NORMAL);
    }
}

void PokedexListObj_SetPos(PokedexListWork *wk, u32 id, s16 x, s16 y, u16 surface) {
    ClActorPos pos;

    pos.x = x;
    pos.y = y;
    ClActor_SetSurfacePos(wk->actors[id], &pos, surface);
}

void PokedexListObj_GetPos(PokedexListWork *wk, u32 id, s16 *x, s16 *y, u16 surface) {
    ClActorPos pos;

    ClActor_GetSurfacePos(wk->actors[id], &pos, surface);
    *x = pos.x;
    *y = pos.y;
}

static void PokedexListObj_InitSystem(void) {
    ClActSysSetup setup = { 0,
                            0,
                            0,
                            512,
                            4,
                            124,
                            4,
                            124,
                            0,
                            POKEDEX_LIST_CHAR_RES_COUNT,
                            POKEDEX_LIST_PALETTE_RES_COUNT,
                            POKEDEX_LIST_CELL_RES_COUNT,
                            0,
                            16,
                            16 };

    ClActSys_Create(&setup, PokedexListMain_GetVRAMConfig(), HEAPID_POKEDEX_LIST);
}

static void PokedexListObj_LoadResources(PokedexListWork *wk) {
    ArcTool *arc;
    u32 i;
    u32 *chars, *palette, *cells;

    for (i = 0; i < POKEDEX_LIST_CHAR_RES_COUNT; i++) {
        wk->charRes[i] = -1;
    }
    for (i = 0; i < POKEDEX_LIST_PALETTE_RES_COUNT; i++) {
        wk->paletteRes[i] = -1;
    }
    for (i = 0; i < POKEDEX_LIST_CELL_RES_COUNT; i++) {
        wk->cellRes[i] = -1;
    }

    arc = GFL_ArcSysCreateFileHandle(ARCID_POKEDEX_GRA, HEAPID_POKEDEX_LIST | HEAPID_TAIL_BIT);
    wk->charRes[CHAR_RES_LIST] = ClActSys_LoadChars(arc, 57, FALSE, CLACT_VRAM_MAIN, HEAPID_POKEDEX_LIST);
    wk->paletteRes[RES_LIST] = ClActSys_LoadPltt(arc, 53, CLACT_VRAM_MAIN, 0 * 0x20, HEAPID_POKEDEX_LIST);
    wk->cellRes[RES_LIST] = ClActSys_LoadCellAnims(arc, 63, 73, HEAPID_POKEDEX_LIST);
    GFL_ArcToolFree(arc);

    // The icons of each screen; their characters are loaded over when a row is drawn
    arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, HEAPID_POKEDEX_LIST | HEAPID_TAIL_BIT);
    chars = &wk->charRes[CHAR_RES_ICON];
    palette = &wk->paletteRes[RES_ICON];
    cells = &wk->cellRes[RES_ICON];
    for (i = 0; i < POKEDEX_LIST_ICON_COUNT; i++) {
        chars[i] = ClActSys_LoadChars(arc, PokeParty_GetIconIndex(0, 0, 0, FALSE), FALSE, CLACT_VRAM_MAIN,
                                      HEAPID_POKEDEX_LIST);
    }
    *palette = ClActSys_LoadPlttEx(arc, PokeIcon_GetPaletteFile(), CLACT_VRAM_MAIN, 4 * 0x20, HEAPID_POKEDEX_LIST);
    *cells = ClActSys_LoadCellAnims(arc, PokeIcon_GetCellFile_Main(), getOBJTileMapping_MainEng(), HEAPID_POKEDEX_LIST);
    chars = &wk->charRes[CHAR_RES_ICON_SUB];
    palette = &wk->paletteRes[RES_ICON_SUB];
    cells = &wk->cellRes[RES_ICON_SUB];
    for (i = 0; i < POKEDEX_LIST_ICON_COUNT; i++) {
        chars[i] =
            ClActSys_LoadChars(arc, PokeParty_GetIconIndex(0, 0, 0, FALSE), FALSE, CLACT_VRAM_SUB, HEAPID_POKEDEX_LIST);
    }
    *palette = ClActSys_LoadPlttEx(arc, PokeIcon_GetPaletteFile(), CLACT_VRAM_SUB, 0 * 0x20, HEAPID_POKEDEX_LIST);
    *cells = ClActSys_LoadCellAnims(arc, PokeIcon_GetCellFile_Sub(), getOBJTileMapping_SubEng(), HEAPID_POKEDEX_LIST);
    GFL_ArcToolFree(arc);

    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_POKEDEX_LIST | HEAPID_TAIL_BIT);
    wk->charRes[CHAR_RES_TOUCHBAR] =
        ClActSys_LoadChars(arc, AppMenuCommon_GetTouchBarCharFile(), FALSE, CLACT_VRAM_MAIN, HEAPID_POKEDEX_LIST);
    wk->paletteRes[RES_TOUCHBAR] =
        ClActSys_LoadPltt(arc, AppMenuCommon_GetTouchBarPaletteFile(), CLACT_VRAM_MAIN, 7 * 0x20, HEAPID_POKEDEX_LIST);
    wk->cellRes[RES_TOUCHBAR] = ClActSys_LoadCellAnims(arc, AppMenuCommon_GetTouchBarCellFile(2),
                                                       AppMenuCommon_GetTouchBarAnimFile(2), HEAPID_POKEDEX_LIST);
    GFL_ArcToolFree(arc);
}

// The cursor's colors, in the palette of the list's actors
static void PokedexListObj_InitBlinkAnm(PokedexListWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEDEX_GRA, HEAPID_POKEDEX_LIST);

    wk->blinkAnm = BlinkPalAnm_Create(1 * 0x10, 0x10, BLINK_PALANM_OBJ_MAIN, HEAPID_POKEDEX_LIST);
    BlinkPalAnm_SetPalBufferArcTool(wk->blinkAnm, arc, 53, 1 * 0x10, 2 * 0x10);
    GFL_ArcToolFree(arc);
}

static void PokedexListObj_ExitBlinkAnm(PokedexListWork *wk) {
    BlinkPalAnm_Free(wk->blinkAnm);
}

void PokedexListObj_StopBlink(PokedexListWork *wk, BOOL stopped) {
    wk->blinkStopped = stopped;
}

static void PokedexListObj_FreeResources(PokedexListWork *wk) {
    u32 i;

    for (i = 0; i < POKEDEX_LIST_CHAR_RES_COUNT; i++) {
        if (wk->charRes[i] != -1) {
            ClActSys_FreeChars(wk->charRes[i]);
        }
    }
    for (i = 0; i < POKEDEX_LIST_PALETTE_RES_COUNT; i++) {
        if (wk->paletteRes[i] != -1) {
            ClActSys_FreePltt(wk->paletteRes[i]);
        }
    }
    for (i = 0; i < POKEDEX_LIST_CELL_RES_COUNT; i++) {
        if (wk->cellRes[i] != -1) {
            ClActSys_FreeCellAnims(wk->cellRes[i]);
        }
    }
}

static void PokedexListObj_CreateActors(PokedexListWork *wk) {
    BOOL national = PokeDex_IsNationalObtained(wk->param->pokedex);
    BOOL habitat = PokeDex_IsHabitatListEnabled(wk->param->pokedex);
    u32 i;
    s16 x, y;

    wk->actorUnit = ClActUnit_Create(POKEDEX_LIST_OBJ_COUNT, 0, HEAPID_POKEDEX_LIST);
    for (i = 0; i < POKEDEX_LIST_OBJ_COUNT; i++) {
        wk->actors[i] = NULL;
    }
    for (i = POKEDEX_LIST_OBJ_SCROLL_BAR; i <= POKEDEX_LIST_OBJ_START; i++) {
        PokedexActorTemplate template = sPokedexListActorTemplates[i];

        template.chars = wk->charRes[template.chars];
        template.palette = wk->paletteRes[template.palette];
        template.cells = wk->cellRes[template.cells];
        wk->actors[i] = PokedexCommon_CreateActor(wk->actorUnit, &template, HEAPID_POKEDEX_LIST);
        ClActor_SetDrawAnim(wk->actors[i], FALSE);
    }
    if (wk->param->count <= POKEDEX_LIST_ROW_COUNT) {
        PokedexListObj_SetVisible(wk, POKEDEX_LIST_OBJ_SCROLL_BAR, FALSE);
        PokedexListObj_SetVisible(wk, POKEDEX_LIST_OBJ_SCROLL_RAIL, FALSE);
    }
    if (PokeDex_IsGiven(wk->param->pokedex) == 0) {
        PokedexListObj_SetVisible(wk, POKEDEX_LIST_OBJ_START, FALSE);
    }
    if (habitat == FALSE) {
        PokedexListObj_SetVisible(wk, POKEDEX_LIST_OBJ_SELECT, FALSE);
        PokedexListObj_GetPos(wk, POKEDEX_LIST_OBJ_START, &x, &y, CLACT_SURFACE_MAIN);
        PokedexListObj_SetPos(wk, POKEDEX_LIST_OBJ_START, x - 32, y, CLACT_SURFACE_MAIN);
    }
    // The button of the Pokédex that the list is of stays pressed
    if (national == FALSE && habitat == FALSE) {
        PokedexListObj_SetVisible(wk, POKEDEX_LIST_OBJ_UNOVA, FALSE);
        PokedexListObj_SetVisible(wk, POKEDEX_LIST_OBJ_NATIONAL, FALSE);
        PokedexListObj_SetVisible(wk, POKEDEX_LIST_OBJ_HABITAT, FALSE);
    } else if (national == FALSE && habitat == TRUE) {
        PokedexListObj_SetVisible(wk, POKEDEX_LIST_OBJ_UNOVA, FALSE);
        PokedexListObj_SetAnmSeq(wk, POKEDEX_LIST_OBJ_NATIONAL, 14);
        PokedexListObj_EndAnm(wk, POKEDEX_LIST_OBJ_NATIONAL);
    } else if (national == TRUE && habitat == TRUE) {
        if (PokeDex_GetMode(wk->param->pokedex) != 1) {
            PokedexListObj_SetAnmSeq(wk, POKEDEX_LIST_OBJ_UNOVA, 14);
            PokedexListObj_EndAnm(wk, POKEDEX_LIST_OBJ_UNOVA);
        } else {
            PokedexListObj_SetAnmSeq(wk, POKEDEX_LIST_OBJ_NATIONAL, 14);
            PokedexListObj_EndAnm(wk, POKEDEX_LIST_OBJ_NATIONAL);
        }
    }
    if (wk->param->searched == TRUE) {
        PokedexListObj_SetVisible(wk, POKEDEX_LIST_OBJ_SELECT, FALSE);
        PokedexListObj_SetVisible(wk, POKEDEX_LIST_OBJ_SHORTCUT, FALSE);
        PokedexListObj_SetVisible(wk, POKEDEX_LIST_OBJ_UNOVA, FALSE);
        PokedexListObj_SetVisible(wk, POKEDEX_LIST_OBJ_NATIONAL, FALSE);
        PokedexListObj_SetVisible(wk, POKEDEX_LIST_OBJ_HABITAT, FALSE);
    } else if (GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_POKEDEX_LIST) == TRUE) {
        PokedexListObj_SetAnm(wk, POKEDEX_LIST_OBJ_SHORTCUT, 7);
    }
}

static void PokedexListObj_DeleteActor(PokedexListWork *wk, u32 id) {
    if (wk->actors[id] != NULL) {
        ClActor_Delete(wk->actors[id]);
        wk->actors[id] = NULL;
    }
}

static void PokedexListObj_DeleteActors(PokedexListWork *wk) {
    u32 i;

    for (i = 0; i < POKEDEX_LIST_OBJ_COUNT; i++) {
        PokedexListObj_DeleteActor(wk, i);
    }
    ClActUnit_Delete(wk->actorUnit);
}

// Loads the picture into the actor that isn't shown, then hides the other
void PokedexListObj_PutMon(PokedexListWork *wk, u16 species) {
    PokedexActorTemplate template = { { 48, 48, 0, 0, 1 }, CHAR_RES_MON, RES_MON, RES_MON, 0, CLACT_SURFACE_MAIN };
    u32 sex, form, rare;
    ArcTool *arc;
    PersonalData *personal;
    u32 id, prevId;
    u32 paletteOffset;

    if (wk->monIndex == 0) {
        prevId = POKEDEX_LIST_OBJ_MON + 1;
        id = POKEDEX_LIST_OBJ_MON;
        paletteOffset = MON_PALETTE_OFFSET;
    } else {
        prevId = POKEDEX_LIST_OBJ_MON;
        template.chars = CHAR_RES_MON + 1;
        id = POKEDEX_LIST_OBJ_MON + 1;
        template.palette = RES_MON + 1;
        template.cells = RES_MON + 1;
        paletteOffset = MON_PALETTE_OFFSET + 0x20;
    }
    if (wk->actors[id] != NULL) {
        PokedexListObj_DeleteActor(wk, id);
        if (wk->charRes[template.chars] != -1) {
            ClActSys_FreeChars(wk->charRes[template.chars]);
        }
        if (wk->paletteRes[template.palette] != -1) {
            ClActSys_FreePltt(wk->paletteRes[template.palette]);
        }
        if (wk->cellRes[template.cells] != -1) {
            ClActSys_FreeCellAnims(wk->cellRes[template.cells]);
        }
    }
    if (species == SPECIES_NONE) {
        arc = GFL_ArcSysCreateFileHandle(ARCID_POKEDEX_GRA, HEAPID_POKEDEX_LIST | HEAPID_TAIL_BIT);
        wk->charRes[template.chars] = ClActSys_LoadChars(arc, 56, FALSE, CLACT_VRAM_MAIN, HEAPID_POKEDEX_LIST);
        wk->paletteRes[template.palette] =
            ClActSys_LoadPlttRange(arc, 52, CLACT_VRAM_MAIN, paletteOffset, 0, 1, HEAPID_POKEDEX_LIST);
        wk->cellRes[template.cells] = ClActSys_LoadCellAnims(arc, 62, 72, HEAPID_POKEDEX_LIST);
        GFL_ArcToolFree(arc);
        PokedexListObj_SetVisible(wk, POKEDEX_LIST_OBJ_MON_BASE, FALSE);
    } else {
        u32 personality;

        PokeDex_GetShownSexRareForm(wk->param->pokedex, species, &sex, &rare, &form,
                                    HEAPID_POKEDEX_LIST | HEAPID_TAIL_BIT);
        arc = MakePokeGraArcHandle(HEAPID_POKEDEX_LIST | HEAPID_TAIL_BIT);
        if (species == SPECIES_SPINDA) {
            personality = PokeDex_GetSpindaPersonality(wk->param->pokedex, 0);
        } else {
            personality = 0;
        }
        wk->charRes[template.chars] = PokeGra_LoadClActChars(arc, species, form, sex, rare, 0, FALSE, personality,
                                                             CLACT_VRAM_MAIN, HEAPID_POKEDEX_LIST);
        wk->paletteRes[template.palette] = PokeGra_LoadClActPalette(
            arc, species, form, sex, rare, 0, FALSE, CLACT_VRAM_MAIN, paletteOffset, HEAPID_POKEDEX_LIST);
        wk->cellRes[template.cells] =
            PokeGra_LoadClActCellAnims(species, form, sex, rare, 0, FALSE, 2, CLACT_VRAM_MAIN, HEAPID_POKEDEX_LIST);
        GFL_ArcToolFree(arc);
        PokedexListObj_SetVisible(wk, POKEDEX_LIST_OBJ_MON_BASE, TRUE);
    }
    template.chars = wk->charRes[template.chars];
    template.palette = wk->paletteRes[template.palette];
    template.cells = wk->cellRes[template.cells];
    wk->actors[id] = PokedexCommon_CreateActor(wk->actorUnit, &template, HEAPID_POKEDEX_LIST);
    ClActor_SetDrawAnim(wk->actors[id], FALSE);

    // BUG: for the unknown picture the form is whatever the stack holds, since nothing read it from the Pokédex
#if defined(PLATFORM_PC) || defined(BUGFIX)
    if (species == SPECIES_NONE) {
        form = 0;
    }
#endif
    personal = PML_PersonalLoad(species, form, HEAPID_POKEDEX_LIST | HEAPID_TAIL_BIT);
    if (species != SPECIES_NONE && PML_PersonalGetParam(personal, PERSONAL_NO_FLIP) == 0) {
        ClActor_SetFlip(wk->actors[id], 1, TRUE);
    }
    PML_PersonalFree(personal);
    PokedexListObj_SetVisible(wk, prevId, FALSE);
    PaletteFade_LoadFromVRAM(wk->paletteFade, PALFADE_BUFFER_MAIN_OBJ, paletteOffset / 0x20 * 0x10, 0x20);
    wk->monIndex ^= 1;
}

static void PokedexListObj_CreateIcons(PokedexListWork *wk) {
    PokedexActorTemplate template;
    PokedexActorTemplate icon = { { ICON_X, 8, 0, 0, 2 }, CHAR_RES_ICON, RES_ICON, RES_ICON, 0, CLACT_SURFACE_MAIN };
    u32 i;

    template = icon;
    template.palette = wk->paletteRes[RES_ICON];
    template.cells = wk->cellRes[RES_ICON];
    for (i = 0; i < POKEDEX_LIST_ICON_COUNT; i++) {
        template.setup.y = i * ROW_HEIGHT + 8;
        template.chars = wk->charRes[CHAR_RES_ICON + i];
        wk->actors[POKEDEX_LIST_OBJ_ICON + i] =
            PokedexCommon_CreateActor(wk->actorUnit, &template, HEAPID_POKEDEX_LIST);
        ClActor_SetDrawAnim(wk->actors[POKEDEX_LIST_OBJ_ICON + i], FALSE);
    }

    template = icon;
    template.palette = wk->paletteRes[RES_ICON_SUB];
    template.cells = wk->cellRes[RES_ICON_SUB];
    for (i = 0; i < POKEDEX_LIST_ICON_COUNT; i++) {
        template.setup.y = i * ROW_HEIGHT + 32;
        template.chars = wk->charRes[CHAR_RES_ICON_SUB + i];
        template.surface = CLACT_SURFACE_SUB;
        wk->actors[POKEDEX_LIST_OBJ_ICON_SUB + i] =
            PokedexCommon_CreateActor(wk->actorUnit, &template, HEAPID_POKEDEX_LIST);
        ClActor_SetDrawAnim(wk->actors[POKEDEX_LIST_OBJ_ICON_SUB + i], FALSE);
        PokedexListObj_SetBlend(wk, POKEDEX_LIST_OBJ_ICON_SUB + i, TRUE);

        wk->actors[POKEDEX_LIST_OBJ_ICON_WINDOW + i] =
            PokedexCommon_CreateActor(wk->actorUnit, &template, HEAPID_POKEDEX_LIST);
        ClActor_SetDrawAnim(wk->actors[POKEDEX_LIST_OBJ_ICON_WINDOW + i], FALSE);
        ClActor_SetObjMode(wk->actors[POKEDEX_LIST_OBJ_ICON_WINDOW + i], OAM_MODE_OBJWND);
    }
    for (i = 0; i < POKEDEX_LIST_ICON_COUNT; i++) {
        PokedexListObj_SetVisible(wk, POKEDEX_LIST_OBJ_ICON + i, FALSE);
    }
    for (i = 0; i < POKEDEX_LIST_ICON_COUNT; i++) {
        PokedexListObj_SetVisible(wk, POKEDEX_LIST_OBJ_ICON_SUB + i, FALSE);
        PokedexListObj_SetVisible(wk, POKEDEX_LIST_OBJ_ICON_WINDOW + i, FALSE);
    }
}

// Loads a species' icon over an icon actor's characters
static void PokedexListObj_LoadIcon(PokedexListWork *wk, u32 id, u16 species, u16 form, u16 sex, BOOL touchScreen) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, HEAPID_POKEDEX_LIST | HEAPID_TAIL_BIT);
    NNSG2dCharacterData *chars;
    void *file = GFL_G2DIOReadOBJNCGRArc(arc, PokeParty_GetIconIndex(species, form, sex, FALSE), FALSE, &chars,
                                         HEAPID_POKEDEX_LIST | HEAPID_TAIL_BIT);

    if (touchScreen == TRUE) {
        ClActSys_WriteCharData(ClActor_GetCharIdx(wk->actors[id]), chars->pRawData, 0x400, 0, CLACT_VRAM_MAIN);
    } else {
        ClActSys_WriteCharData(ClActor_GetCharIdx(wk->actors[id]), chars->pRawData, 0x400, 0, CLACT_VRAM_SUB);
    }
    GFL_HeapFree(file);
    GFL_ArcToolFree(arc);
    ClActor_SetPalette(wk->actors[id], PokeIcon_GetPalette(species, form, sex, FALSE), 1);
}

// An icon actor of a screen that isn't shown
static u32 PokedexListObj_FindFreeIcon(PokedexListWork *wk, BOOL touchScreen) {
    u32 i;

    if (touchScreen == TRUE) {
        for (i = POKEDEX_LIST_OBJ_ICON; i < POKEDEX_LIST_OBJ_ICON + POKEDEX_LIST_ICON_COUNT; i++) {
            if (PokedexListObj_IsVisible(wk, i) == FALSE) {
                return i;
            }
        }
    } else {
        for (i = POKEDEX_LIST_OBJ_ICON_SUB; i < POKEDEX_LIST_OBJ_ICON_SUB + POKEDEX_LIST_ICON_COUNT; i++) {
            if (PokedexListObj_IsVisible(wk, i) == FALSE) {
                return i;
            }
        }
    }
    return 0;
}

void PokedexListObj_PutIcon(PokedexListWork *wk, u16 species, s16 y, BOOL touchScreen) {
    u32 sex, form, rare;
    u32 id;

    PokeDex_GetShownSexRareForm(wk->param->pokedex, species, &sex, &rare, &form, HEAPID_POKEDEX_LIST | HEAPID_TAIL_BIT);
    id = PokedexListObj_FindFreeIcon(wk, touchScreen);
    PokedexListObj_LoadIcon(wk, id, species, form, sex, touchScreen);
    PokedexListObj_SetVisible(wk, id, TRUE);
    PokedexListObj_SetAnm(wk, id, 0);
    if (touchScreen == TRUE) {
        PokedexListObj_SetPos(wk, id, ICON_X, y + 8, CLACT_SURFACE_MAIN);
    } else {
        PokedexListObj_SetPos(wk, id, ICON_X, y + 8, CLACT_SURFACE_SUB);
        PokedexListObj_SetPos(wk, id + POKEDEX_LIST_ICON_COUNT, ICON_X, y + 8, CLACT_SURFACE_SUB);
        PokedexListObj_SetVisible(wk, id + POKEDEX_LIST_ICON_COUNT, TRUE);
    }
}

void PokedexListObj_ScrollIcons(PokedexListWork *wk, s8 delta) {
    u32 i;
    s16 x, y;

    for (i = POKEDEX_LIST_OBJ_ICON; i < POKEDEX_LIST_OBJ_ICON + POKEDEX_LIST_ICON_COUNT; i++) {
        if (PokedexListObj_IsVisible(wk, i)) {
            PokedexListObj_GetPos(wk, i, &x, &y, CLACT_SURFACE_MAIN);
            PokedexListObj_SetPos(wk, i, x, y + delta, CLACT_SURFACE_MAIN);
            if (y + delta <= -16 || y + delta >= TOP_LIST_HEIGHT + 8) {
                PokedexListObj_SetVisible(wk, i, FALSE);
            }
        }
    }
    for (i = POKEDEX_LIST_OBJ_ICON_SUB; i < POKEDEX_LIST_OBJ_ICON_SUB + POKEDEX_LIST_ICON_COUNT; i++) {
        if (PokedexListObj_IsVisible(wk, i)) {
            PokedexListObj_GetPos(wk, i, &x, &y, CLACT_SURFACE_SUB);
            PokedexListObj_SetPos(wk, i, x, y + delta, CLACT_SURFACE_SUB);
            PokedexListObj_SetPos(wk, i + POKEDEX_LIST_ICON_COUNT, x, y + delta, CLACT_SURFACE_SUB);
            if (y + delta <= -16 || y + delta >= TOP_LIST_HEIGHT + 8) {
                PokedexListObj_SetVisible(wk, i, FALSE);
                PokedexListObj_SetVisible(wk, i + POKEDEX_LIST_ICON_COUNT, FALSE);
            }
        }
    }
}

void PokedexListObj_SaveIcons(PokedexListWork *wk) {
    u32 i;

    wk->shownIcons = 0;
    wk->shownIconsSub = 0;
    for (i = POKEDEX_LIST_OBJ_ICON; i < POKEDEX_LIST_OBJ_ICON + POKEDEX_LIST_ICON_COUNT; i++) {
        if (PokedexListObj_IsVisible(wk, i)) {
            wk->shownIcons |= 1 << (i - POKEDEX_LIST_OBJ_ICON);
        }
    }
    for (i = POKEDEX_LIST_OBJ_ICON_SUB; i < POKEDEX_LIST_OBJ_ICON_SUB + POKEDEX_LIST_ICON_COUNT; i++) {
        if (PokedexListObj_IsVisible(wk, i)) {
            wk->shownIconsSub |= 1 << (i - POKEDEX_LIST_OBJ_ICON_SUB);
        }
    }
}

void PokedexListObj_HideSavedIcons(PokedexListWork *wk) {
    u32 i;

    for (i = POKEDEX_LIST_OBJ_ICON; i < POKEDEX_LIST_OBJ_ICON + POKEDEX_LIST_ICON_COUNT; i++) {
        if (wk->shownIcons & (1 << (i - POKEDEX_LIST_OBJ_ICON))) {
            PokedexListObj_SetVisible(wk, i, FALSE);
        }
    }
    for (i = POKEDEX_LIST_OBJ_ICON_SUB; i < POKEDEX_LIST_OBJ_ICON_SUB + POKEDEX_LIST_ICON_COUNT; i++) {
        if (wk->shownIconsSub & (1 << (i - POKEDEX_LIST_OBJ_ICON_SUB))) {
            PokedexListObj_SetVisible(wk, i, FALSE);
            PokedexListObj_SetVisible(wk, i + POKEDEX_LIST_ICON_COUNT, FALSE);
        }
    }
    wk->shownIcons = 0;
    wk->shownIconsSub = 0;
}

void PokedexListObj_SelectIcon(PokedexListWork *wk, u32 row) {
    u32 i;
    s16 x, y;

    // The y of the row's icon
    row = row * ROW_HEIGHT + 8;

    for (i = POKEDEX_LIST_OBJ_ICON; i < POKEDEX_LIST_OBJ_ICON + POKEDEX_LIST_ICON_COUNT; i++) {
        if (PokedexListObj_IsVisible(wk, i) == FALSE) {
            PokedexListObj_SetAnm(wk, i, 0);
        } else {
            PokedexListObj_GetPos(wk, i, &x, &y, CLACT_SURFACE_MAIN);
            if (row == y) {
                if (PokedexListObj_GetAnm(wk, i) != 1) {
                    PokedexListObj_SetAnm(wk, i, 1);
                }
            } else {
                PokedexListObj_SetAnm(wk, i, 0);
            }
        }
    }
}

void PokedexListObj_UpdateScrollBar(PokedexListWork *wk) {
    s16 x, y;
    u32 pos;

    PokedexListObj_GetPos(wk, POKEDEX_LIST_OBJ_SCROLL_BAR, &x, &y, CLACT_SURFACE_MAIN);
    pos = FrameList_ClampBarPos(wk->frameList, y);
    if (pos < 12) {
        pos = 12;
    } else if (pos > 156) {
        pos = 156;
    }
    PokedexListObj_SetPos(wk, POKEDEX_LIST_OBJ_SCROLL_BAR, 252, pos, CLACT_SURFACE_MAIN);
}

void PokedexListObj_SetPageButtons(PokedexListWork *wk, BOOL keep) {
    s16 cursor = FrameList_GetCursor(wk->frameList);

    if (PokedexListObj_GetAnm(wk, POKEDEX_LIST_OBJ_PAGE_UP) != 12 || keep == FALSE) {
        if (FrameList_GetScroll(wk->frameList) == 0 && cursor == 0) {
            PokedexListObj_SetAnm(wk, POKEDEX_LIST_OBJ_PAGE_UP, 18);
        } else {
            PokedexListObj_SetAnm(wk, POKEDEX_LIST_OBJ_PAGE_UP, 4);
        }
    }
    if (PokedexListObj_GetAnm(wk, POKEDEX_LIST_OBJ_PAGE_DOWN) != 13 || keep == FALSE) {
        if (FrameList_CanScrollDown(wk->frameList) == FALSE && cursor == wk->listRows - 1) {
            PokedexListObj_SetAnm(wk, POKEDEX_LIST_OBJ_PAGE_DOWN, 19);
        } else {
            PokedexListObj_SetAnm(wk, POKEDEX_LIST_OBJ_PAGE_DOWN, 5);
        }
    }
}
