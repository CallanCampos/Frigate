// The actual Pokédex list's PutMon (src/ov299/pokedex_list_obj.c), with typed UI/resource spies only in this TU.
// No ROM: check the personal-data call for an unknown picture and preserve seen forms/flip rules and both slots.
#define GFL_ArcSysCreateFileHandle SpyOpen
#define ClActSys_LoadChars SpyChars
#define ClActSys_LoadPlttRange SpyPalette
#define ClActSys_LoadCellAnims SpyCells
#define GFL_ArcToolFree SpyClose
#define PokedexCommon_CreateActor SpyActor
#define ClActor_SetDrawAnim SpyDraw
#define ClActor_SetVisible SpyVisible
#define ClActor_SetFlip SpyFlip
#define PML_PersonalLoad SpyPersonal
#define PML_PersonalGetParam SpyParam
#define PML_PersonalFree SpyFree
#define PaletteFade_LoadFromVRAM SpyFade
#define PokeDex_GetShownSexRareForm SpyShown
#define MakePokeGraArcHandle SpyGraphics
#define PokeGra_LoadClActChars SpyPokeChars
#define PokeGra_LoadClActPalette SpyPokePalette
#define PokeGra_LoadClActCellAnims SpyPokeCells
#include "src/ov299/pokedex_list_obj.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static u64 sArcStorage, sActorStorage;
static PersonalData sPersonal;
static u16 sSpecies, sForm;
static BOOL sNoFlip;
static unsigned sCreated, sLoaded, sFreed, sFaded, sFlips, sShown;
#define CHECK(x)                                                                                                       \
    do {                                                                                                               \
        if (!(x)) {                                                                                                    \
            fprintf(stderr, "pokedex form line %d: %s\n", __LINE__, #x);                                               \
            exit(1);                                                                                                   \
        }                                                                                                              \
    } while (0)
static ArcTool *Arc(void) {
    return (ArcTool *)&sArcStorage;
}
static ClActor *Actor(void) {
    return (ClActor *)&sActorStorage;
}
ArcTool *SpyOpen(u32 id, HeapID heap) {
    (void)id;
    (void)heap;
    return Arc();
}
u32 SpyChars(ArcTool *arc, u32 file, BOOL compressed, u32 vram, HeapID heap) {
    CHECK(arc == Arc());
    (void)file;
    (void)compressed;
    (void)vram;
    (void)heap;
    return 1;
}
u32 SpyPalette(ArcTool *arc, u32 file, u32 vram, u16 offset, u16 start, u16 count, HeapID heap) {
    CHECK(arc == Arc());
    (void)file;
    (void)vram;
    (void)offset;
    (void)start;
    (void)count;
    (void)heap;
    return 2;
}
u32 SpyCells(ArcTool *arc, u32 cell, u32 anim, HeapID heap) {
    CHECK(arc == Arc());
    (void)cell;
    (void)anim;
    (void)heap;
    return 3;
}
void SpyClose(ArcTool *arc) {
    CHECK(arc == Arc());
}
ClActor *SpyActor(ClActUnit *unit, const PokedexActorTemplate *template, HeapID heap) {
    (void)unit;
    (void)heap;
    CHECK(template->chars == 1 && template->palette == 2 && template->cells == 3);
    ++sCreated;
    return Actor();
}
void SpyDraw(ClActor *actor, BOOL draw) {
    CHECK(actor == Actor() && draw == FALSE);
}
void SpyVisible(ClActor *actor, BOOL visible) {
    CHECK(actor == Actor());
    (void)visible;
}
void SpyFlip(ClActor *actor, u32 axis, BOOL flip) {
    CHECK(actor == Actor() && axis == 1 && flip == TRUE);
    ++sFlips;
}
PersonalData *SpyPersonal(u16 species, u16 form, HeapID heap) {
    CHECK(sCreated == 1 && sLoaded == 0 && sFreed == 0 && sFaded == 0);
    CHECK(species == sSpecies && heap == (HEAPID_POKEDEX_LIST | HEAPID_TAIL_BIT));
    if (form != sForm) {
        fprintf(stderr, "pokedex form: species %u expected form %u, got %u\n", species, sForm, form);
        exit(1);
    }
    ++sLoaded;
    return &sPersonal;
}
u32 SpyParam(PersonalData *personal, u32 param) {
    CHECK(personal == &sPersonal && param == PERSONAL_NO_FLIP);
    return sNoFlip;
}
void SpyFree(PersonalData *personal) {
    CHECK(personal == &sPersonal && sLoaded == 1 && !sFreed);
    ++sFreed;
}
void SpyFade(PaletteFade *fade, u32 vram, u16 offset, u32 size) {
    (void)fade;
    (void)vram;
    (void)offset;
    CHECK(size == 0x20 && sFreed == 1);
    ++sFaded;
}
void SpyShown(PokeDexSave *pokedex, u16 species, u32 *sex, u32 *rare, u32 *form, HeapID heap) {
    (void)pokedex;
    (void)heap;
    CHECK(species == sSpecies && species != SPECIES_NONE);
    *sex = 1;
    *rare = TRUE;
    *form = sForm;
    ++sShown;
}
ArcTool *SpyGraphics(HeapID heap) {
    (void)heap;
    return Arc();
}
u32 SpyPokeChars(ArcTool *arc, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg, u32 personality, u32 vram,
                 HeapID heap) {
    CHECK(arc == Arc() && species == sSpecies && form == sForm && sex == 1 && rare == TRUE);
    (void)dir;
    (void)egg;
    (void)personality;
    (void)vram;
    (void)heap;
    return 1;
}
u32 SpyPokePalette(ArcTool *arc, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg, u32 vram, u16 offset,
                   HeapID heap) {
    CHECK(arc == Arc() && species == sSpecies && form == sForm && sex == 1 && rare == TRUE);
    (void)dir;
    (void)egg;
    (void)vram;
    (void)offset;
    (void)heap;
    return 2;
}
u32 SpyPokeCells(u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg, u32 mapping, u32 vram, HeapID heap) {
    CHECK(species == sSpecies && form == sForm && sex == 1 && rare == TRUE);
    (void)dir;
    (void)egg;
    (void)mapping;
    (void)vram;
    (void)heap;
    return 3;
}
int main(void) {
    for (unsigned slot = 0; slot < 2; ++slot) {
        for (unsigned mode = 0; mode < 3; ++mode) {
            PokedexListWork work;
            PokedexListParam param;
            memset(&work, 0, sizeof(work));
            memset(&param, 0, sizeof(param));
            work.param = &param;
            work.monIndex = slot;
            sSpecies = mode ? SPECIES_UNOWN : SPECIES_NONE;
            sForm = mode ? 7 : 0;
            sNoFlip = mode == 2;
            sCreated = sLoaded = sFreed = sFaded = sFlips = sShown = 0;
            PokedexListObj_PutMon(&work, sSpecies);
            CHECK(sCreated == 1 && sLoaded == 1 && sFreed == 1 && sFaded == 1);
            CHECK(sShown == (mode != 0) && sFlips == (mode == 1));
            CHECK(work.monIndex == !slot && work.actors[POKEDEX_LIST_OBJ_MON + slot] == Actor());
        }
    }
    puts("pokedex form: six actual PutMon flows PASS");
    return 0;
}
