#include "types.h"
#include "save/zukan_habitat_savedata.h"
#include "gfl/std.h"
#include "save/medal_box.h"

// The Pokédex's habitat list in the save: what the player has done of each habitat, and the zone, season and kind
// of habitat that the list shows. The ROM has no string for the file, so the name is a guess. The list is not a save
// block of its own: the medal box's block has it at its end.

static u32 getSize(void);

void func_02010ca0(void *data) {
    sys_memset(data, 0, getSize());
}

static u32 getSize(void) {
    return sizeof(ZukanHabitatSave);
}

void *func_02010cb8(SaveControl *save) {
    return MedalBox_GetHabitatList(SaveControl_GetMedalBox(save));
}

u8 func_02010cc4(void *data, u8 habitat, u8 type) {
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

BOOL func_02010cfc(ZukanHabitatSave *save, u32 habitat) {
    return save->habitats[habitat].flag;
}

void func_02010d08(ZukanHabitatSave *save, u32 habitat) {
    save->habitats[habitat].flag = TRUE;
}

void func_02010d14(ZukanHabitatSave *save, u32 habitat, u32 type, u32 value) {
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

void func_02010d70(void *data, u16 zone) {
    ZukanHabitatSave *save = data;

    save->zone = zone;
}

u16 func_02010d78(void *data) {
    ZukanHabitatSave *save = data;

    return save->zone;
}

void func_02010d80(void *data, u32 season) {
    ZukanHabitatSave *save = data;

    save->season = season;
}

u8 func_02010d88(void *data) {
    ZukanHabitatSave *save = data;

    return save->season;
}

void func_02010d90(void *data, u8 type) {
    ZukanHabitatSave *save = data;

    save->type = type;
}

u8 func_02010d98(void *data) {
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
