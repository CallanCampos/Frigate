#ifndef POKEBW2_PML_PERSONAL_H
#define POKEBW2_PML_PERSONAL_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// Species data

#define PERSONAL_TYPE_1 6
#define PERSONAL_TYPE_2 7
#define PERSONAL_ABILITY_1 26
#define PERSONAL_ABILITY_2 27
#define PERSONAL_ABILITY_HIDDEN 28
#define PERSONAL_SEX_RATIO 20
#define PERSONAL_HATCH_CYCLES 21
// The offset of the forms' sprites (not from swan)
#define PERSONAL_FORM_SPRITE_OFFSET 31
#define PERSONAL_FORM_COUNT 32
// Whether the forms only change the palette, as Arceus's do (not from swan)
// Whether the sprite may be flipped
#define PERSONAL_SPRITE_FLIP 34
#define PERSONAL_PALETTE_FORMS 35
// Not from swan: a flag that keeps the summary screen's sprite from bouncing, and the weight
#define PERSONAL_NO_BOUNCE 16
#define PERSONAL_WEIGHT 38

u32 PML_PersonalGetParamSingle(u16 species, u16 form, u32 param);
// The same from Black and White's personal data, which the musical's sprites still follow
u32 PML_PersonalGetParamSingleBW1(u16 species, u16 form, u32 param);
void *PML_PersonalLoad(u16 species, u16 form, HeapID heapId);
u32 PML_PersonalGetParam(void *personal, u32 param);
void PML_PersonalFree(void *personal);
// The moves a species learns by level, pairs of move and level that end with two 0xffff
void PML_LearnsetLvUpLoad(u16 species, u8 form, void *dest);
// Allocates the regional Pokédex's order of species
u16 *PML_PersonalLoadRegionalDexTable(HeapID heapId, u32 a1);
// The experience a Pokémon of the species needs for the level
u32 PML_UtilGetPkmLvExp(u16 species, u16 form, u16 level);
// The regional Pokédex numbers, by national number. The caller frees the table
u16 *PML_PersonalLoadRegionalDexTable(HeapID heapId, u32 a1);

ArcTool *loadEvolutionFile(HeapID heapId);
BOOL func_02020bf0(ArcTool *evoFile, u16 species, u16 form, u16 index);

#endif // POKEBW2_PML_PERSONAL_H
