#include "types.h"
#include "gfl/std.h"
#include "save/medal_box.h"
#include "save/pokedex_habitat_savedata.h"

// The Pokédex's habitat list in the save: what the player has done of each habitat, and the zone, season and kind
// of habitat that the list shows. The ROM has no string for the file, so the name is a guess. The list is not a save
// block of its own: the medal box's block has it at its end.

static u32 getSize(void);

void PokedexHabitatSave_Init(PokedexHabitatSave *data) {
    MI_CpuFill8(data, 0, getSize());
}

static u32 getSize(void) {
    return sizeof(PokedexHabitatSave);
}

PokedexHabitatSave *SaveControl_GetPokedexHabitatSave(SaveControl *save) {
    return MedalBox_GetHabitatList(SaveControl_GetMedalBox(save));
}

u8 PokedexHabitatSave_GetHabitatField(PokedexHabitatSave *save, u8 habitat, u32 type) {
    switch (type) {
    case POKEDEX_HABITAT_TYPE_0:
        return save->habitats[habitat].type0;
    case POKEDEX_HABITAT_TYPE_1:
        return save->habitats[habitat].type1;
    case POKEDEX_HABITAT_TYPE_2:
        return save->habitats[habitat].type2;
    }
    return 0;
}

BOOL PokedexHabitatSave_IsHabitatFlagSet(PokedexHabitatSave *save, u8 habitat) {
    return save->habitats[habitat].flag;
}

void PokedexHabitatSave_SetHabitatFlag(PokedexHabitatSave *save, u8 habitat) {
    save->habitats[habitat].flag = TRUE;
}

void PokedexHabitatSave_SetHabitatField(PokedexHabitatSave *save, u8 habitat, u32 type, u32 value) {
    switch (type) {
    case POKEDEX_HABITAT_TYPE_0:
        save->habitats[habitat].type0 = (u8)value;
        break;
    case POKEDEX_HABITAT_TYPE_1:
        save->habitats[habitat].type1 = (u8)value;
        break;
    case POKEDEX_HABITAT_TYPE_2:
        save->habitats[habitat].type2 = (u8)value;
        break;
    }
}

void PokedexHabitatSave_SetZone(PokedexHabitatSave *save, u16 zone) {
    save->zone = zone;
}

u16 PokedexHabitatSave_GetZone(PokedexHabitatSave *save) {
    return save->zone;
}

void PokedexHabitatSave_SetSeason(PokedexHabitatSave *save, u32 season) {
    save->season = season;
}

u8 PokedexHabitatSave_GetSeason(PokedexHabitatSave *save) {
    return save->season;
}

void PokedexHabitatSave_SetType(PokedexHabitatSave *save, u16 type) {
    save->type = type;
}

u8 PokedexHabitatSave_GetType(PokedexHabitatSave *save) {
    return save->type;
}

u8 PokedexHabitatSave_IsSeenMessageShown(PokedexHabitatSave *save) {
    return save->seenMessageShown;
}

u8 PokedexHabitatSave_IsCaughtMessageShown(PokedexHabitatSave *save) {
    return save->caughtMessageShown;
}

void PokedexHabitatSave_SetSeenMessageShown(PokedexHabitatSave *save) {
    save->seenMessageShown = TRUE;
}

void PokedexHabitatSave_SetCaughtMessageShown(PokedexHabitatSave *save) {
    save->caughtMessageShown = TRUE;
}
