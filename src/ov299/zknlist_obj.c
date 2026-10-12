#include "types.h"
#include "app/ui/frame_list.h"
#include "app/zknlist_main.h"
#include "app/zknlist_obj.h"
#include "app/zukan_common.h"
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
#include "save/pokedex.h"
#include "system/app_menu_common.h"
#include "system/blink_palanm.h"
#include "system/game_data.h"
#include "system/palanm.h"
#include "system/poke_icon.h"

// The actors of the Pokédex's list: the touch bar's buttons, the scroll bar, the buttons of the Pokédexes, the icons
// of the rows and the picture of the species under the cursor. The ROM doesn't name this file; zknlist_obj.c is a
// guess after the search screen's zknsearch_obj.c

// The resources: the icons of each screen have their own characters, and the two pictures their own of each kind
enum {
    CHAR_RES_ICON,
    CHAR_RES_ICON_SUB = CHAR_RES_ICON + ZKNLIST_ICON_COUNT,
    CHAR_RES_MON = CHAR_RES_ICON_SUB + ZKNLIST_ICON_COUNT,
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

static void ZknListObj_InitSystem(void);
static void ZknListObj_LoadResources(ZknListWork *wk);
static void ZknListObj_InitBlinkAnm(ZknListWork *wk);
static void ZknListObj_ExitBlinkAnm(ZknListWork *wk);
static void ZknListObj_FreeResources(ZknListWork *wk);
static void ZknListObj_CreateActors(ZknListWork *wk);
static void ZknListObj_DeleteActor(ZknListWork *wk, u32 id);
static void ZknListObj_DeleteActors(ZknListWork *wk);
static void ZknListObj_CreateIcons(ZknListWork *wk);
static void ZknListObj_LoadIcon(ZknListWork *wk, u32 id, u16 species, u16 form, u16 sex, BOOL touchScreen);
static u32 ZknListObj_FindFreeIcon(ZknListWork *wk, BOOL touchScreen);

static const ZukanActorTemplate sZknListActorTemplates[] = {
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

void ZknListObj_Init(ZknListWork *wk) {
    ZknListObj_InitSystem();
    ZknListObj_LoadResources(wk);
    ZknListObj_InitBlinkAnm(wk);
    ZknListObj_CreateActors(wk);
    ZknListObj_CreateIcons(wk);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

void ZknListObj_Exit(ZknListWork *wk) {
    ZknListObj_ExitBlinkAnm(wk);
    ZknListObj_DeleteActors(wk);
    ZknListObj_FreeResources(wk);
    func_0204b758();
}

void ZknListObj_Update(ZknListWork *wk) {
    u32 i;

    if (wk->blinkStopped == FALSE) {
        BlinkPalAnm_Main(wk->blinkAnm);
    }
    for (i = 0; i < ZKNLIST_OBJ_COUNT; i++) {
        if (wk->actors[i] != NULL && func_0204c534(wk->actors[i]) != TRUE) {
            func_0204c4e0(wk->actors[i], FX32_ONE);
        }
    }
    func_0204b794();
}

void ZknListObj_SetAnmSeq(ZknListWork *wk, u32 id, u32 anm) {
    func_0204c4d4(wk->actors[id], 0);
    func_0204c488(wk->actors[id], anm);
    func_0204c540(wk->actors[id]);
}

void ZknListObj_EndAnm(ZknListWork *wk, u32 id) {
    func_0204c550(wk->actors[id]);
}

void ZknListObj_SetAnm(ZknListWork *wk, u32 id, u32 anm) {
    ZknListObj_SetAnmSeq(wk, id, anm);
    func_0204c520(wk->actors[id], TRUE);
}

u32 ZknListObj_GetAnm(ZknListWork *wk, u32 id) {
    return func_0204c4a0(wk->actors[id]);
}

BOOL ZknListObj_IsAnmPlaying(ZknListWork *wk, u32 id) {
    return func_0204c560(wk->actors[id]);
}

void ZknListObj_SetVisible(ZknListWork *wk, u32 id, BOOL visible) {
    if (wk->actors[id] != NULL) {
        func_0204c124(wk->actors[id], visible);
    }
}

BOOL ZknListObj_IsVisible(ZknListWork *wk, u32 id) {
    return func_0204c138(wk->actors[id]);
}

void ZknListObj_SetBlend(ZknListWork *wk, u32 id, BOOL blend) {
    if (blend == TRUE) {
        func_0204c318(wk->actors[id], GX_OAM_MODE_XLU);
    } else {
        func_0204c318(wk->actors[id], GX_OAM_MODE_NORMAL);
    }
}

void ZknListObj_SetPos(ZknListWork *wk, u32 id, s16 x, s16 y, u16 surface) {
    ClActorPos pos;

    pos.x = x;
    pos.y = y;
    func_0204c140(wk->actors[id], &pos, surface);
}

void ZknListObj_GetPos(ZknListWork *wk, u32 id, s16 *x, s16 *y, u16 surface) {
    ClActorPos pos;

    func_0204c178(wk->actors[id], &pos, surface);
    *x = pos.x;
    *y = pos.y;
}

static void ZknListObj_InitSystem(void) {
    ClActSysSetup setup = {
        0, 0,  0, 512, 4, 124, 4, 124, 0, ZKNLIST_CHAR_RES_COUNT, ZKNLIST_PALETTE_RES_COUNT, ZKNLIST_CELL_RES_COUNT,
        0, 16, 16
    };

    ClActSys_Create(&setup, ZknListMain_GetVRAMConfig(), HEAPID_ZUKAN_LIST);
}

static void ZknListObj_LoadResources(ZknListWork *wk) {
    ArcTool *arc;
    u32 i;
    u32 *chars, *palette, *cells;

    for (i = 0; i < ZKNLIST_CHAR_RES_COUNT; i++) {
        wk->charRes[i] = -1;
    }
    for (i = 0; i < ZKNLIST_PALETTE_RES_COUNT; i++) {
        wk->paletteRes[i] = -1;
    }
    for (i = 0; i < ZKNLIST_CELL_RES_COUNT; i++) {
        wk->cellRes[i] = -1;
    }

    arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_ZUKAN_LIST | HEAPID_TAIL_BIT);
    wk->charRes[CHAR_RES_LIST] = func_0204b81c(arc, 57, FALSE, CLACT_VRAM_MAIN, HEAPID_ZUKAN_LIST);
    wk->paletteRes[RES_LIST] = func_0204bba0(arc, 53, CLACT_VRAM_MAIN, 0 * 0x20, HEAPID_ZUKAN_LIST);
    wk->cellRes[RES_LIST] = func_0204bde0(arc, 63, 73, HEAPID_ZUKAN_LIST);
    GFL_ArcToolFree(arc);

    // The icons of each screen; their characters are loaded over when a row is drawn
    arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, HEAPID_ZUKAN_LIST | HEAPID_TAIL_BIT);
    chars = &wk->charRes[CHAR_RES_ICON];
    palette = &wk->paletteRes[RES_ICON];
    cells = &wk->cellRes[RES_ICON];
    for (i = 0; i < ZKNLIST_ICON_COUNT; i++) {
        chars[i] =
            func_0204b81c(arc, PokeParty_GetIconIndex(0, 0, 0, FALSE), FALSE, CLACT_VRAM_MAIN, HEAPID_ZUKAN_LIST);
    }
    *palette = func_0204bc48(arc, PokeIcon_GetPaletteFile(), CLACT_VRAM_MAIN, 4 * 0x20, HEAPID_ZUKAN_LIST);
    *cells = func_0204bde0(arc, PokeIcon_GetCellFile_Main(), getOBJTileMapping_MainEng(), HEAPID_ZUKAN_LIST);
    chars = &wk->charRes[CHAR_RES_ICON_SUB];
    palette = &wk->paletteRes[RES_ICON_SUB];
    cells = &wk->cellRes[RES_ICON_SUB];
    for (i = 0; i < ZKNLIST_ICON_COUNT; i++) {
        chars[i] = func_0204b81c(arc, PokeParty_GetIconIndex(0, 0, 0, FALSE), FALSE, CLACT_VRAM_SUB, HEAPID_ZUKAN_LIST);
    }
    *palette = func_0204bc48(arc, PokeIcon_GetPaletteFile(), CLACT_VRAM_SUB, 0 * 0x20, HEAPID_ZUKAN_LIST);
    *cells = func_0204bde0(arc, PokeIcon_GetCellFile_Sub(), getOBJTileMapping_SubEng(), HEAPID_ZUKAN_LIST);
    GFL_ArcToolFree(arc);

    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_ZUKAN_LIST | HEAPID_TAIL_BIT);
    wk->charRes[CHAR_RES_TOUCHBAR] = func_0204b81c(arc, func_0202d814(), FALSE, CLACT_VRAM_MAIN, HEAPID_ZUKAN_LIST);
    wk->paletteRes[RES_TOUCHBAR] = func_0204bba0(arc, func_0202d810(), CLACT_VRAM_MAIN, 7 * 0x20, HEAPID_ZUKAN_LIST);
    wk->cellRes[RES_TOUCHBAR] = func_0204bde0(arc, func_0202d818(2), func_0202d81c(2), HEAPID_ZUKAN_LIST);
    GFL_ArcToolFree(arc);
}

// The cursor's colors, in the palette of the list's actors
static void ZknListObj_InitBlinkAnm(ZknListWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_ZUKAN_LIST);

    wk->blinkAnm = BlinkPalAnm_Create(1 * 0x10, 0x10, BLINK_PALANM_OBJ_MAIN, HEAPID_ZUKAN_LIST);
    BlinkPalAnm_SetPalBufferArcTool(wk->blinkAnm, arc, 53, 1 * 0x10, 2 * 0x10);
    GFL_ArcToolFree(arc);
}

static void ZknListObj_ExitBlinkAnm(ZknListWork *wk) {
    BlinkPalAnm_Free(wk->blinkAnm);
}

void ZknListObj_StopBlink(ZknListWork *wk, BOOL stopped) {
    wk->blinkStopped = stopped;
}

static void ZknListObj_FreeResources(ZknListWork *wk) {
    u32 i;

    for (i = 0; i < ZKNLIST_CHAR_RES_COUNT; i++) {
        if (wk->charRes[i] != -1) {
            func_0204b98c(wk->charRes[i]);
        }
    }
    for (i = 0; i < ZKNLIST_PALETTE_RES_COUNT; i++) {
        if (wk->paletteRes[i] != -1) {
            func_0204bcd0(wk->paletteRes[i]);
        }
    }
    for (i = 0; i < ZKNLIST_CELL_RES_COUNT; i++) {
        if (wk->cellRes[i] != -1) {
            func_0204be64(wk->cellRes[i]);
        }
    }
}

static void ZknListObj_CreateActors(ZknListWork *wk) {
    BOOL national = PokeDex_IsNationalObtained(wk->param->pokedex);
    BOOL habitat = PokeDex_IsHabitatListEnabled(wk->param->pokedex);
    u32 i;
    s16 x, y;

    wk->actorUnit = func_0204bf1c(ZKNLIST_OBJ_COUNT, 0, HEAPID_ZUKAN_LIST);
    for (i = 0; i < ZKNLIST_OBJ_COUNT; i++) {
        wk->actors[i] = NULL;
    }
    for (i = ZKNLIST_OBJ_SCROLL_BAR; i <= ZKNLIST_OBJ_START; i++) {
        ZukanActorTemplate template = sZknListActorTemplates[i];

        template.chars = wk->charRes[template.chars];
        template.palette = wk->paletteRes[template.palette];
        template.cells = wk->cellRes[template.cells];
        wk->actors[i] = ZukanCommon_CreateActor(wk->actorUnit, &template, HEAPID_ZUKAN_LIST);
        func_0204c5c8(wk->actors[i], FALSE);
    }
    if (wk->param->count <= ZKNLIST_ROW_COUNT) {
        ZknListObj_SetVisible(wk, ZKNLIST_OBJ_SCROLL_BAR, FALSE);
        ZknListObj_SetVisible(wk, ZKNLIST_OBJ_SCROLL_RAIL, FALSE);
    }
    if (func_0200d1dc(wk->param->pokedex) == 0) {
        ZknListObj_SetVisible(wk, ZKNLIST_OBJ_START, FALSE);
    }
    if (habitat == FALSE) {
        ZknListObj_SetVisible(wk, ZKNLIST_OBJ_SELECT, FALSE);
        ZknListObj_GetPos(wk, ZKNLIST_OBJ_START, &x, &y, CLACT_SURFACE_MAIN);
        ZknListObj_SetPos(wk, ZKNLIST_OBJ_START, x - 32, y, CLACT_SURFACE_MAIN);
    }
    // The button of the Pokédex that the list is of stays pressed
    if (national == FALSE && habitat == FALSE) {
        ZknListObj_SetVisible(wk, ZKNLIST_OBJ_UNOVA, FALSE);
        ZknListObj_SetVisible(wk, ZKNLIST_OBJ_NATIONAL, FALSE);
        ZknListObj_SetVisible(wk, ZKNLIST_OBJ_HABITAT, FALSE);
    } else if (national == FALSE && habitat == TRUE) {
        ZknListObj_SetVisible(wk, ZKNLIST_OBJ_UNOVA, FALSE);
        ZknListObj_SetAnmSeq(wk, ZKNLIST_OBJ_NATIONAL, 14);
        ZknListObj_EndAnm(wk, ZKNLIST_OBJ_NATIONAL);
    } else if (national == TRUE && habitat == TRUE) {
        if (func_0200d1f8(wk->param->pokedex) != 1) {
            ZknListObj_SetAnmSeq(wk, ZKNLIST_OBJ_UNOVA, 14);
            ZknListObj_EndAnm(wk, ZKNLIST_OBJ_UNOVA);
        } else {
            ZknListObj_SetAnmSeq(wk, ZKNLIST_OBJ_NATIONAL, 14);
            ZknListObj_EndAnm(wk, ZKNLIST_OBJ_NATIONAL);
        }
    }
    if (wk->param->searched == TRUE) {
        ZknListObj_SetVisible(wk, ZKNLIST_OBJ_SELECT, FALSE);
        ZknListObj_SetVisible(wk, ZKNLIST_OBJ_SHORTCUT, FALSE);
        ZknListObj_SetVisible(wk, ZKNLIST_OBJ_UNOVA, FALSE);
        ZknListObj_SetVisible(wk, ZKNLIST_OBJ_NATIONAL, FALSE);
        ZknListObj_SetVisible(wk, ZKNLIST_OBJ_HABITAT, FALSE);
    } else if (GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_POKEDEX_LIST) == TRUE) {
        ZknListObj_SetAnm(wk, ZKNLIST_OBJ_SHORTCUT, 7);
    }
}

static void ZknListObj_DeleteActor(ZknListWork *wk, u32 id) {
    if (wk->actors[id] != NULL) {
        func_0204c108(wk->actors[id]);
        wk->actors[id] = NULL;
    }
}

static void ZknListObj_DeleteActors(ZknListWork *wk) {
    u32 i;

    for (i = 0; i < ZKNLIST_OBJ_COUNT; i++) {
        ZknListObj_DeleteActor(wk, i);
    }
    func_0204bf98(wk->actorUnit);
}

// Loads the picture into the actor that isn't shown, then hides the other
void ZknListObj_PutMon(ZknListWork *wk, u16 species) {
    ZukanActorTemplate template = { { 48, 48, 0, 0, 1 }, CHAR_RES_MON, RES_MON, RES_MON, 0, CLACT_SURFACE_MAIN };
    u32 sex, form, rare;
    ArcTool *arc;
    void *personal;
    u32 id, prevId;
    u32 paletteOffset;

    if (wk->monIndex == 0) {
        prevId = ZKNLIST_OBJ_MON + 1;
        id = ZKNLIST_OBJ_MON;
        paletteOffset = MON_PALETTE_OFFSET;
    } else {
        prevId = ZKNLIST_OBJ_MON;
        template.chars = CHAR_RES_MON + 1;
        id = ZKNLIST_OBJ_MON + 1;
        template.palette = RES_MON + 1;
        template.cells = RES_MON + 1;
        paletteOffset = MON_PALETTE_OFFSET + 0x20;
    }
    if (wk->actors[id] != NULL) {
        ZknListObj_DeleteActor(wk, id);
        if (wk->charRes[template.chars] != -1) {
            func_0204b98c(wk->charRes[template.chars]);
        }
        if (wk->paletteRes[template.palette] != -1) {
            func_0204bcd0(wk->paletteRes[template.palette]);
        }
        if (wk->cellRes[template.cells] != -1) {
            func_0204be64(wk->cellRes[template.cells]);
        }
    }
    if (species == SPECIES_NONE) {
        arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_ZUKAN_LIST | HEAPID_TAIL_BIT);
        wk->charRes[template.chars] = func_0204b81c(arc, 56, FALSE, CLACT_VRAM_MAIN, HEAPID_ZUKAN_LIST);
        wk->paletteRes[template.palette] =
            func_0204bbb8(arc, 52, CLACT_VRAM_MAIN, paletteOffset, 0, 1, HEAPID_ZUKAN_LIST);
        wk->cellRes[template.cells] = func_0204bde0(arc, 62, 72, HEAPID_ZUKAN_LIST);
        GFL_ArcToolFree(arc);
        ZknListObj_SetVisible(wk, ZKNLIST_OBJ_MON_BASE, FALSE);
    } else {
        u32 personality;

        func_0200d3c8(wk->param->pokedex, species, &sex, &rare, &form, HEAPID_ZUKAN_LIST | HEAPID_TAIL_BIT);
        arc = MakePokeGraArcHandle(HEAPID_ZUKAN_LIST | HEAPID_TAIL_BIT);
        if (species == SPECIES_SPINDA) {
            personality = func_0200da18(wk->param->pokedex, 0);
        } else {
            personality = 0;
        }
        wk->charRes[template.chars] = PokeGra_LoadClActChars(arc, species, form, sex, rare, 0, FALSE, personality,
                                                             CLACT_VRAM_MAIN, HEAPID_ZUKAN_LIST);
        wk->paletteRes[template.palette] = PokeGra_LoadClActPalette(arc, species, form, sex, rare, 0, FALSE,
                                                                    CLACT_VRAM_MAIN, paletteOffset, HEAPID_ZUKAN_LIST);
        wk->cellRes[template.cells] =
            PokeGra_LoadClActCellAnims(species, form, sex, rare, 0, FALSE, 2, CLACT_VRAM_MAIN, HEAPID_ZUKAN_LIST);
        GFL_ArcToolFree(arc);
        ZknListObj_SetVisible(wk, ZKNLIST_OBJ_MON_BASE, TRUE);
    }
    template.chars = wk->charRes[template.chars];
    template.palette = wk->paletteRes[template.palette];
    template.cells = wk->cellRes[template.cells];
    wk->actors[id] = ZukanCommon_CreateActor(wk->actorUnit, &template, HEAPID_ZUKAN_LIST);
    func_0204c5c8(wk->actors[id], FALSE);

    // BUG: for the unknown picture the form is whatever the stack holds, since nothing read it from the Pokédex
    personal = PML_PersonalLoad(species, form, HEAPID_ZUKAN_LIST | HEAPID_TAIL_BIT);
    if (species != SPECIES_NONE && PML_PersonalGetParam(personal, PERSONAL_SPRITE_FLIP) == 0) {
        func_0204c2b0(wk->actors[id], 1, TRUE);
    }
    PML_PersonalFree(personal);
    ZknListObj_SetVisible(wk, prevId, FALSE);
    PaletteFade_LoadFromVRAM(wk->paletteFade, PALFADE_BUFFER_MAIN_OBJ, paletteOffset / 0x20 * 0x10, 0x20);
    wk->monIndex ^= 1;
}

static void ZknListObj_CreateIcons(ZknListWork *wk) {
    ZukanActorTemplate template;
    ZukanActorTemplate icon = { { ICON_X, 8, 0, 0, 2 }, CHAR_RES_ICON, RES_ICON, RES_ICON, 0, CLACT_SURFACE_MAIN };
    u32 i;

    template = icon;
    template.palette = wk->paletteRes[RES_ICON];
    template.cells = wk->cellRes[RES_ICON];
    for (i = 0; i < ZKNLIST_ICON_COUNT; i++) {
        template.setup.y = i * ROW_HEIGHT + 8;
        template.chars = wk->charRes[CHAR_RES_ICON + i];
        wk->actors[ZKNLIST_OBJ_ICON + i] = ZukanCommon_CreateActor(wk->actorUnit, &template, HEAPID_ZUKAN_LIST);
        func_0204c5c8(wk->actors[ZKNLIST_OBJ_ICON + i], FALSE);
    }

    template = icon;
    template.palette = wk->paletteRes[RES_ICON_SUB];
    template.cells = wk->cellRes[RES_ICON_SUB];
    for (i = 0; i < ZKNLIST_ICON_COUNT; i++) {
        template.setup.y = i * ROW_HEIGHT + 32;
        template.chars = wk->charRes[CHAR_RES_ICON_SUB + i];
        template.surface = CLACT_SURFACE_SUB;
        wk->actors[ZKNLIST_OBJ_ICON_SUB + i] = ZukanCommon_CreateActor(wk->actorUnit, &template, HEAPID_ZUKAN_LIST);
        func_0204c5c8(wk->actors[ZKNLIST_OBJ_ICON_SUB + i], FALSE);
        ZknListObj_SetBlend(wk, ZKNLIST_OBJ_ICON_SUB + i, TRUE);

        wk->actors[ZKNLIST_OBJ_ICON_WINDOW + i] = ZukanCommon_CreateActor(wk->actorUnit, &template, HEAPID_ZUKAN_LIST);
        func_0204c5c8(wk->actors[ZKNLIST_OBJ_ICON_WINDOW + i], FALSE);
        func_0204c318(wk->actors[ZKNLIST_OBJ_ICON_WINDOW + i], OAM_MODE_OBJWND);
    }
    for (i = 0; i < ZKNLIST_ICON_COUNT; i++) {
        ZknListObj_SetVisible(wk, ZKNLIST_OBJ_ICON + i, FALSE);
    }
    for (i = 0; i < ZKNLIST_ICON_COUNT; i++) {
        ZknListObj_SetVisible(wk, ZKNLIST_OBJ_ICON_SUB + i, FALSE);
        ZknListObj_SetVisible(wk, ZKNLIST_OBJ_ICON_WINDOW + i, FALSE);
    }
}

// Loads a species' icon over an icon actor's characters
static void ZknListObj_LoadIcon(ZknListWork *wk, u32 id, u16 species, u16 form, u16 sex, BOOL touchScreen) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, HEAPID_ZUKAN_LIST | HEAPID_TAIL_BIT);
    NNSG2dCharacterData *chars;
    void *file = GFL_G2DIOReadOBJNCGRArc(arc, PokeParty_GetIconIndex(species, form, sex, FALSE), FALSE, &chars,
                                         HEAPID_ZUKAN_LIST | HEAPID_TAIL_BIT);

    if (touchScreen == TRUE) {
        func_0204bab8(func_0204c428(wk->actors[id]), chars->rawData, 0x400, 0, CLACT_VRAM_MAIN);
    } else {
        func_0204bab8(func_0204c428(wk->actors[id]), chars->rawData, 0x400, 0, CLACT_VRAM_SUB);
    }
    GFL_HeapFree(file);
    GFL_ArcToolFree(arc);
    func_0204c378(wk->actors[id], PokeIcon_GetPalette(species, form, sex, FALSE), 1);
}

// An icon actor of a screen that isn't shown
static u32 ZknListObj_FindFreeIcon(ZknListWork *wk, BOOL touchScreen) {
    u32 i;

    if (touchScreen == TRUE) {
        for (i = ZKNLIST_OBJ_ICON; i < ZKNLIST_OBJ_ICON + ZKNLIST_ICON_COUNT; i++) {
            if (ZknListObj_IsVisible(wk, i) == FALSE) {
                return i;
            }
        }
    } else {
        for (i = ZKNLIST_OBJ_ICON_SUB; i < ZKNLIST_OBJ_ICON_SUB + ZKNLIST_ICON_COUNT; i++) {
            if (ZknListObj_IsVisible(wk, i) == FALSE) {
                return i;
            }
        }
    }
    return 0;
}

void ZknListObj_PutIcon(ZknListWork *wk, u16 species, s16 y, BOOL touchScreen) {
    u32 sex, form, rare;
    u32 id;

    func_0200d3c8(wk->param->pokedex, species, &sex, &rare, &form, HEAPID_ZUKAN_LIST | HEAPID_TAIL_BIT);
    id = ZknListObj_FindFreeIcon(wk, touchScreen);
    ZknListObj_LoadIcon(wk, id, species, form, sex, touchScreen);
    ZknListObj_SetVisible(wk, id, TRUE);
    ZknListObj_SetAnm(wk, id, 0);
    if (touchScreen == TRUE) {
        ZknListObj_SetPos(wk, id, ICON_X, y + 8, CLACT_SURFACE_MAIN);
    } else {
        ZknListObj_SetPos(wk, id, ICON_X, y + 8, CLACT_SURFACE_SUB);
        ZknListObj_SetPos(wk, id + ZKNLIST_ICON_COUNT, ICON_X, y + 8, CLACT_SURFACE_SUB);
        ZknListObj_SetVisible(wk, id + ZKNLIST_ICON_COUNT, TRUE);
    }
}

void ZknListObj_ScrollIcons(ZknListWork *wk, s8 delta) {
    u32 i;
    s16 x, y;

    for (i = ZKNLIST_OBJ_ICON; i < ZKNLIST_OBJ_ICON + ZKNLIST_ICON_COUNT; i++) {
        if (ZknListObj_IsVisible(wk, i)) {
            ZknListObj_GetPos(wk, i, &x, &y, CLACT_SURFACE_MAIN);
            ZknListObj_SetPos(wk, i, x, y + delta, CLACT_SURFACE_MAIN);
            if (y + delta <= -16 || y + delta >= TOP_LIST_HEIGHT + 8) {
                ZknListObj_SetVisible(wk, i, FALSE);
            }
        }
    }
    for (i = ZKNLIST_OBJ_ICON_SUB; i < ZKNLIST_OBJ_ICON_SUB + ZKNLIST_ICON_COUNT; i++) {
        if (ZknListObj_IsVisible(wk, i)) {
            ZknListObj_GetPos(wk, i, &x, &y, CLACT_SURFACE_SUB);
            ZknListObj_SetPos(wk, i, x, y + delta, CLACT_SURFACE_SUB);
            ZknListObj_SetPos(wk, i + ZKNLIST_ICON_COUNT, x, y + delta, CLACT_SURFACE_SUB);
            if (y + delta <= -16 || y + delta >= TOP_LIST_HEIGHT + 8) {
                ZknListObj_SetVisible(wk, i, FALSE);
                ZknListObj_SetVisible(wk, i + ZKNLIST_ICON_COUNT, FALSE);
            }
        }
    }
}

void ZknListObj_SaveIcons(ZknListWork *wk) {
    u32 i;

    wk->shownIcons = 0;
    wk->shownIconsSub = 0;
    for (i = ZKNLIST_OBJ_ICON; i < ZKNLIST_OBJ_ICON + ZKNLIST_ICON_COUNT; i++) {
        if (ZknListObj_IsVisible(wk, i)) {
            wk->shownIcons |= 1 << (i - ZKNLIST_OBJ_ICON);
        }
    }
    for (i = ZKNLIST_OBJ_ICON_SUB; i < ZKNLIST_OBJ_ICON_SUB + ZKNLIST_ICON_COUNT; i++) {
        if (ZknListObj_IsVisible(wk, i)) {
            wk->shownIconsSub |= 1 << (i - ZKNLIST_OBJ_ICON_SUB);
        }
    }
}

void ZknListObj_HideSavedIcons(ZknListWork *wk) {
    u32 i;

    for (i = ZKNLIST_OBJ_ICON; i < ZKNLIST_OBJ_ICON + ZKNLIST_ICON_COUNT; i++) {
        if (wk->shownIcons & (1 << (i - ZKNLIST_OBJ_ICON))) {
            ZknListObj_SetVisible(wk, i, FALSE);
        }
    }
    for (i = ZKNLIST_OBJ_ICON_SUB; i < ZKNLIST_OBJ_ICON_SUB + ZKNLIST_ICON_COUNT; i++) {
        if (wk->shownIconsSub & (1 << (i - ZKNLIST_OBJ_ICON_SUB))) {
            ZknListObj_SetVisible(wk, i, FALSE);
            ZknListObj_SetVisible(wk, i + ZKNLIST_ICON_COUNT, FALSE);
        }
    }
    wk->shownIcons = 0;
    wk->shownIconsSub = 0;
}

void ZknListObj_SelectIcon(ZknListWork *wk, u32 row) {
    u32 i;
    s16 x, y;

    // The y of the row's icon
    row = row * ROW_HEIGHT + 8;

    for (i = ZKNLIST_OBJ_ICON; i < ZKNLIST_OBJ_ICON + ZKNLIST_ICON_COUNT; i++) {
        if (ZknListObj_IsVisible(wk, i) == FALSE) {
            ZknListObj_SetAnm(wk, i, 0);
        } else {
            ZknListObj_GetPos(wk, i, &x, &y, CLACT_SURFACE_MAIN);
            if (row == y) {
                if (ZknListObj_GetAnm(wk, i) != 1) {
                    ZknListObj_SetAnm(wk, i, 1);
                }
            } else {
                ZknListObj_SetAnm(wk, i, 0);
            }
        }
    }
}

void ZknListObj_UpdateScrollBar(ZknListWork *wk) {
    s16 x, y;
    u32 pos;

    ZknListObj_GetPos(wk, ZKNLIST_OBJ_SCROLL_BAR, &x, &y, CLACT_SURFACE_MAIN);
    pos = FrameList_ClampBarPos(wk->frameList, y);
    if (pos < 12) {
        pos = 12;
    } else if (pos > 156) {
        pos = 156;
    }
    ZknListObj_SetPos(wk, ZKNLIST_OBJ_SCROLL_BAR, 252, pos, CLACT_SURFACE_MAIN);
}

void ZknListObj_SetPageButtons(ZknListWork *wk, BOOL keep) {
    s16 cursor = FrameList_GetCursor(wk->frameList);

    if (ZknListObj_GetAnm(wk, ZKNLIST_OBJ_PAGE_UP) != 12 || keep == FALSE) {
        if (FrameList_GetScroll(wk->frameList) == 0 && cursor == 0) {
            ZknListObj_SetAnm(wk, ZKNLIST_OBJ_PAGE_UP, 18);
        } else {
            ZknListObj_SetAnm(wk, ZKNLIST_OBJ_PAGE_UP, 4);
        }
    }
    if (ZknListObj_GetAnm(wk, ZKNLIST_OBJ_PAGE_DOWN) != 13 || keep == FALSE) {
        if (FrameList_CanScrollDown(wk->frameList) == FALSE && cursor == wk->listRows - 1) {
            ZknListObj_SetAnm(wk, ZKNLIST_OBJ_PAGE_DOWN, 19);
        } else {
            ZknListObj_SetAnm(wk, ZKNLIST_OBJ_PAGE_DOWN, 5);
        }
    }
}
