#ifndef POKEBW2_PML_PERSONAL_H
#define POKEBW2_PML_PERSONAL_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// Species data

// The fields that PML_PersonalGetParam reads, by number, each one of PersonalData's
enum {
    // The base stats (not from swan)
    PERSONAL_BASE_HP,
    PERSONAL_BASE_ATTACK,
    PERSONAL_BASE_DEFENSE,
    PERSONAL_BASE_SPEED,
    PERSONAL_BASE_SP_ATTACK,
    PERSONAL_BASE_SP_DEFENSE,
    PERSONAL_TYPE_1,
    PERSONAL_TYPE_2,
    // Not from swan: the catch rate, the experience that defeating it gives, and the effort values, by stat
    PERSONAL_CATCH_RATE,
    PERSONAL_BASE_EXP,
    PERSONAL_EV_HP,
    PERSONAL_EV_ATTACK,
    PERSONAL_EV_DEFENSE,
    PERSONAL_EV_SPEED,
    PERSONAL_EV_SP_ATTACK,
    PERSONAL_EV_SP_DEFENSE,
    // Not from swan: a flag that keeps the summary screen's sprite from bouncing
    PERSONAL_NO_BOUNCE,
    // The three items of PersonalData.items (not from swan)
    PERSONAL_ITEM_1,
    PERSONAL_ITEM_2,
    PERSONAL_ITEM_3,
    PERSONAL_SEX_RATIO,
    PERSONAL_HATCH_CYCLES,
    PERSONAL_FRIENDSHIP,
    PERSONAL_GROWTH_RATE,
    // The egg groups (not from swan)
    PERSONAL_EGG_GROUP_1,
    PERSONAL_EGG_GROUP_2,
    PERSONAL_ABILITY_1,
    PERSONAL_ABILITY_2,
    PERSONAL_ABILITY_HIDDEN,
    // Not from swan: the flee rate, and the file of the first other form, or 0
    PERSONAL_FLEE_RATE,
    PERSONAL_FORM_FILE,
    // The offset of the forms' sprites (not from swan)
    PERSONAL_FORM_SPRITE_OFFSET,
    PERSONAL_FORM_COUNT,
    // The color the Pokédex searches by, a COLOR_* (not from swan)
    PERSONAL_COLOR,
    // A flag that keeps the sprite from being mirrored (not from swan)
    PERSONAL_NO_FLIP,
    // Whether the forms only change the palette, as Arceus's do (not from swan)
    PERSONAL_PALETTE_FORMS,
    // Not from swan: PersonalData.stage, the height and the weight, the four words of TM and HM flags, the free move
    // tutors' flags, and the four words of the paid move tutors' flags
    PERSONAL_STAGE,
    PERSONAL_HEIGHT,
    PERSONAL_WEIGHT,
    PERSONAL_MACHINES_1,
    PERSONAL_MACHINES_2,
    PERSONAL_MACHINES_3,
    PERSONAL_MACHINES_4,
    PERSONAL_TUTORS,
    PERSONAL_TUTOR_PAID_1,
    PERSONAL_TUTOR_PAID_2,
    PERSONAL_TUTOR_PAID_3,
    PERSONAL_TUTOR_PAID_4,
};

// A species' data, the files of ARCID_PERSONAL. PML_PersonalGetParam reads its fields by number
typedef struct {
    u8 hp;
    u8 attack;
    u8 defense;
    u8 speed;
    u8 spAttack;
    u8 spDefense;
    u8 type1;
    u8 type2;
    u8 catchRate;
    u8 stage;
    u16 evHP : 2;
    u16 evAttack : 2;
    u16 evDefense : 2;
    u16 evSpeed : 2;
    u16 evSpAttack : 2;
    u16 evSpDefense : 2;
    u16 noBounce : 1;
    u16 items[3];
    u8 sexRatio;
    u8 hatchCycles;
    u8 friendship;
    u8 growthRate;
    u8 eggGroup1;
    u8 eggGroup2;
    u8 abilities[3];
    u8 fleeRate;
    // The file of the first other form, or 0, and the offset of the forms' sprites
    u16 formFile;
    u16 formSpriteOffset;
    u8 formCount;
    u8 color : 6;
    u8 noFlip : 1;
    u8 paletteForms : 1;
    u16 baseExp;
    u16 height;
    u16 weight;
    u32 machines[4];
    u32 tutors[5];
} PersonalData;

// Opens the personal archives, once
void PML_PersonalArcInit(HeapID heapId);
void PML_PersonalFetchBW2(u16 species, u16 form, PersonalData *dest);
void PML_PersonalFetchBW1(u16 species, u16 form, PersonalData *dest);
u32 PML_PersonalGetParamSingle(u16 species, u16 form, u32 param);
// The same from Black and White's personal data, which the musical's sprites still follow
u32 PML_PersonalGetParamSingleBW1(u16 species, u16 form, u32 param);
PersonalData *PML_PersonalLoad(u16 species, u16 form, HeapID heapId);
u32 PML_PersonalGetParam(PersonalData *personal, u32 param);
void PML_PersonalFree(PersonalData *personal);
#define LEARNSET_MAX 26
#define LEARNSET_END 0xffff

// A move a species learns at a level
typedef struct {
    u16 move;
    u16 level;
} LearnsetEntry;

static inline BOOL LearnsetEntry_IsEnd(LearnsetEntry entry) {
    return entry.move == LEARNSET_END && entry.level == LEARNSET_END;
}

static inline u16 LearnsetEntry_GetMove(LearnsetEntry entry) {
    return entry.move;
}

static inline u16 LearnsetEntry_GetLevel(LearnsetEntry entry) {
    return entry.level;
}

// The moves a species learns by level, at most LEARNSET_MAX entries that end with one whose move and level are both
// LEARNSET_END
void PML_LearnsetLvUpLoad(u16 species, u16 form, LearnsetEntry *dest);
// The experience a Pokémon of the species needs for the level
u32 PML_UtilGetPkmLvExp(u16 species, u16 form, u16 level);
// The regional Pokédex numbers, by national number, and, if count is not NULL, how many species have one. The caller
// frees the table
u16 *PML_PersonalLoadRegionalDexTable(HeapID heapId, u16 *count);

ArcTool *loadEvolutionFile(HeapID heapId);
BOOL getEvolutionInfoParam(ArcTool *evoFile, u16 species, u16 form, u16 index);

#endif // POKEBW2_PML_PERSONAL_H
