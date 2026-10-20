#include "types.h"
#include "gfl/std.h"
#include "save/medal_box.h"
#include "save/zukan_habitat_savedata.h"

// The Pokédex's habitat list in the save: what the player has done of each habitat, and the zone, season and kind
// of habitat that the list shows. The ROM has no string for the file, so the name is a guess. The list is not a save
// block of its own: the medal box's block has it at its end.

static u32 getSize(void);

void ZukanHabitatSave_Init(void *data) {
    MI_CpuFill8(data, 0, getSize());
}

static u32 getSize(void) {
    return sizeof(ZukanHabitatSave);
}

void *SaveControl_GetZukanHabitatSave(SaveControl *save) {
    return MedalBox_GetHabitatList(SaveControl_GetMedalBox(save));
}

u8 ZukanHabitatSave_GetHabitatField(void *data, u8 habitat, u32 type) {
    ZukanHabitatSave *save = data;

    switch (type) {
    case ZUKAN_HABITAT_TYPE_0:
        return save->habitats[habitat].type0;
    case ZUKAN_HABITAT_TYPE_1:
        return save->habitats[habitat].type1;
    case ZUKAN_HABITAT_TYPE_2:
        return save->habitats[habitat].type2;
    }
    return 0;
}

BOOL ZukanHabitatSave_IsHabitatFlagSet(ZukanHabitatSave *save, u8 habitat) {
    return save->habitats[habitat].flag;
}

void ZukanHabitatSave_SetHabitatFlag(ZukanHabitatSave *save, u8 habitat) {
    save->habitats[habitat].flag = TRUE;
}

void ZukanHabitatSave_SetHabitatField(ZukanHabitatSave *save, u8 habitat, u32 type, u32 value) {
    switch (type) {
    case ZUKAN_HABITAT_TYPE_0:
        save->habitats[habitat].type0 = (u8)value;
        break;
    case ZUKAN_HABITAT_TYPE_1:
        save->habitats[habitat].type1 = (u8)value;
        break;
    case ZUKAN_HABITAT_TYPE_2:
        save->habitats[habitat].type2 = (u8)value;
        break;
    }
}

void ZukanHabitatSave_SetZone(void *data, u16 zone) {
    ZukanHabitatSave *save = data;

    save->zone = zone;
}

u16 ZukanHabitatSave_GetZone(void *data) {
    ZukanHabitatSave *save = data;

    return save->zone;
}

void ZukanHabitatSave_SetSeason(void *data, u32 season) {
    ZukanHabitatSave *save = data;

    save->season = season;
}

u8 ZukanHabitatSave_GetSeason(void *data) {
    ZukanHabitatSave *save = data;

    return save->season;
}

void ZukanHabitatSave_SetType(void *data, u16 type) {
    ZukanHabitatSave *save = data;

    save->type = type;
}

u8 ZukanHabitatSave_GetType(void *data) {
    ZukanHabitatSave *save = data;

    return save->type;
}

u8 func_02010da0(ZukanHabitatSave *save) {
    return save->unk94;
}

u8 func_02010da8(ZukanHabitatSave *save) {
    return save->unk95;
}

void func_02010db0(ZukanHabitatSave *save) {
    save->unk94 = TRUE;
}

void func_02010db8(ZukanHabitatSave *save) {
    save->unk95 = TRUE;
}
