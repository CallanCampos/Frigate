// The ROM names this file zukan_common.c.
#include "types.h"
#include "app/pokedex_common.h"
#include "app/pokedex_search_engine.h"
#include "constants/flags.h"
#include "constants/species.h"
#include "constants/zones.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "save/event_work.h"
#include "save/medal_box.h"
#include "save/pokedex_savedata.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/wipe.h"

// What the Pokédex's overlays share: the list of the species, and the habitat lists

// The heap of the Pokédex
// The archive of the areas' files
#define ARCID_POKEDEX_HABITAT 296
// The file of an area that the table does not name
#define FILE_NONE 0x18
// The flags that make an area's conditions true
#define FLAG_CONDITION_1 EVENT_FLAG_0x0987
#define FLAG_CONDITION_2 EVENT_FLAG_0x0985

static BOOL PokedexCommon_IsInHabitat(PokeDexSave *pokedex, const PokedexHabitatFileEntry *entry, u8 type, u8 season,
                                      u32 bySeason);
static void PokedexCommon_GetHabitatStates(u32 *states, const PokedexHabitatFile *file, PokeDexSave *pokedex);
static BOOL PokedexCommon_IsAreaShown(const PokedexHabitatFile *file, const PokedexHabitatConditions *conditions);
static void PokedexCommon_GetConditions(PokedexHabitatConditions *conditions, EventWork *eventWork);
static int PokedexCommon_GetAreaFile(u16 zone);
static u16 *PokedexCommon_CreateSpeciesList(const PokedexHabitatFile *file, PokeDexSave *pokedex, u8 type, u8 season,
                                            u16 *total, HeapID heapId);
static u32 PokedexCommon_CountAreas(EventWork *eventWork, ArcTool *arc, const PokedexHabitatConditions *conditions);

// The areas of the habitat list: the flag that shows it, its zone and its file
static const PokedexHabitatTableEntry sPokedexHabitats[POKEDEX_HABITAT_COUNT] = {
    { EVENT_FLAG_ARRIVED_ASPERTIA_CITY, ZONE_ASPERTIA_CITY, 24, 0 },
    { EVENT_FLAG_0x09c0, ZONE_ROUTE_19, 23, 0 },
    { EVENT_FLAG_0x09c1, ZONE_ROUTE_20, 28, 0 },
    { EVENT_FLAG_0x09c2, ZONE_FLOCCESY_RANCH, 26, 0 },
    { EVENT_FLAG_ARRIVED_VIRBANK_CITY, ZONE_VIRBANK_CITY, 29, 0 },
    { EVENT_FLAG_0x09c3, ZONE_VIRBANK_COMPLEX, 32, 0 },
    { EVENT_FLAG_ARRIVED_CASTELIA_CITY, ZONE_CASTELIA_CITY, 34, 0 },
    { EVENT_FLAG_0x09c4, ZONE_CASTELIA_SEWERS, 30, 0 },
    { EVENT_FLAG_0x09c5, ZONE_ROUTE_4, 0, 0 },
    { EVENT_FLAG_0x09c6, ZONE_DESERT_RESORT, 41, 0 },
    { EVENT_FLAG_0x09c7, ZONE_RELIC_CASTLE, 21, 0 },
    { EVENT_FLAG_0x09c8, ZONE_ROUTE_5, 9, 0 },
    { EVENT_FLAG_0x09c9, ZONE_ROUTE_16, 8, 0 },
    { EVENT_FLAG_0x09ca, ZONE_LOSTLORN_FOREST, 55, 0 },
    { EVENT_FLAG_0x09cb, ZONE_DRIFTVEIL_DRAWBRIDGE, 53, 0 },
    { EVENT_FLAG_0x09cc, ZONE_ROUTE_6, 13, 0 },
    { EVENT_FLAG_0x09cd, ZONE_RELIC_PASSAGE, 7, 0 },
    { EVENT_FLAG_0x09f0, ZONE_CLAY_TUNNEL, 3, 0 },
    { EVENT_FLAG_0x09ce, ZONE_MISTRALTON_CAVE, 49, 0 },
    { EVENT_FLAG_0x09cf, ZONE_CHARGESTONE_CAVE, 57, 0 },
    { EVENT_FLAG_0x09d0, ZONE_ROUTE_7, 19, 0 },
    { EVENT_FLAG_0x09d1, ZONE_CELESTIAL_TOWER_2, 39, 0 },
    { EVENT_FLAG_0x09d2, ZONE_REVERSAL_MOUNTAIN, 37, 0 },
    { EVENT_FLAG_0x09d3, ZONE_STRANGE_HOUSE, 27, 0 },
    { EVENT_FLAG_ARRIVED_UNDELLA_TOWN, ZONE_UNDELLA_TOWN, 12, 0 },
    { EVENT_FLAG_0x09d4, ZONE_ROUTE_13, 50, 0 },
    { EVENT_FLAG_0x09d5, ZONE_UNDELLA_BAY, 14, 0 },
    { EVENT_FLAG_0x09d6, ZONE_ROUTE_14, 56, 0 },
    { EVENT_FLAG_0x09d7, ZONE_ABUNDANT_SHRINE, 11, 0 },
    { EVENT_FLAG_0x09d8, ZONE_ROUTE_12, 47, 0 },
    { EVENT_FLAG_0x09d9, ZONE_VILLAGE_BRIDGE, 5, 0 },
    { EVENT_FLAG_0x09da, ZONE_ROUTE_11, 42, 0 },
    { EVENT_FLAG_0x09db, ZONE_ROUTE_9, 31, 0 },
    { EVENT_FLAG_0x09dc, ZONE_SEASIDE_CAVE, 54, 0 },
    { EVENT_FLAG_0x09dd, ZONE_ROUTE_21, 2, 0 },
    { EVENT_FLAG_0x09de, ZONE_ROUTE_15, 1, 0 },
    { EVENT_FLAG_0x09bf, ZONE_MARVELOUS_BRIDGE, 10, 0 },
    { EVENT_FLAG_ARRIVED_HUMILAU_CITY, ZONE_HUMILAU_CITY, 36, 0 },
    { EVENT_FLAG_0x09df, ZONE_ROUTE_22, 40, 0 },
    { EVENT_FLAG_0x09e0, ZONE_GIANT_CHASM, 18, 0 },
    { EVENT_FLAG_0x09e1, ZONE_ROUTE_23, 45, 0 },
    { EVENT_FLAG_ARRIVED_VICTORY_ROAD, ZONE_VICTORY_ROAD, 20, 0 },
    { EVENT_FLAG_0x09e3, ZONE_ROUTE_8, 25, 0 },
    { EVENT_FLAG_0x09e4, ZONE_MOOR_OF_ICIRRUS, 46, 0 },
    { EVENT_FLAG_ARRIVED_ICIRRUS_CITY, ZONE_ICIRRUS_CITY, 52, 0 },
    { EVENT_FLAG_0x09e5, ZONE_DRAGONSPIRAL_TOWER, 6, 0 },
    { EVENT_FLAG_0x09e6, ZONE_TWIST_MOUNTAIN, 4, 0 },
    { EVENT_FLAG_0x09e7, ZONE_PINWHEEL_FOREST, 35, 0 },
    { EVENT_FLAG_0x09e8, ZONE_ROUTE_3, 51, 0 },
    { EVENT_FLAG_0x09e9, ZONE_WELLSPRING_CAVE, 15, 0 },
    { EVENT_FLAG_ARRIVED_STRIATON_CITY, ZONE_STRIATON_CITY, 22, 0 },
    { EVENT_FLAG_0x09ea, ZONE_DREAMYARD, 38, 0 },
    { EVENT_FLAG_0x09ec, ZONE_ROUTE_2, 48, 0 },
    { EVENT_FLAG_0x09eb, ZONE_ROUTE_1, 44, 0 },
    { EVENT_FLAG_0x09ed, ZONE_ROUTE_17, 43, 0 },
    { EVENT_FLAG_0x09ee, ZONE_ROUTE_18, 17, 0 },
    { EVENT_FLAG_0x09ef, ZONE_P2_LABORATORY, 33, 0 },
};

u32 PokedexCommon_CreateList(PokeDexSave *pokedex, u16 **list, HeapID heapId) {
    u16 *species;
    u32 count;
    u32 last;
    u32 i;

    if (PokeDex_GetMode(pokedex) == 1) {
        count = PokedexSearchEngine_LoadList(0, heapId, &species);
    } else {
        count = PokedexSearchEngine_LoadList(1, heapId, &species);
    }
    last = 0;
    for (i = 0; i < count; i++) {
        if (PokeDex_IsCaught(pokedex, species[i]) == 1 || PokeDex_IsSeen(pokedex, species[i]) == 1) {
            last = (u16)(i + 1);
        }
    }
    *list = species;
    return last;
}

static BOOL PokedexCommon_IsInHabitat(PokeDexSave *pokedex, const PokedexHabitatFileEntry *entry, u8 type, u8 season,
                                      u32 bySeason) {
    if (PokeDex_IsCaught(pokedex, entry->species) == 1 || PokeDex_IsSeen(pokedex, entry->species) == 1) {
        if (bySeason == 1) {
            if (entry->present[season][type] == 1) {
                return TRUE;
            }
        } else if (entry->present[0][type] == 1) {
            return TRUE;
        }
    }
    return FALSE;
}

void PokedexCommon_SelectSeen(PokeDexSave *pokedex, const u16 *list, u16 count) {
    u16 first = 0xffff;
    u16 current = PokeDex_GetLastSpecies(pokedex);
    u16 i;

    for (i = 0; i < count; i++) {
        if (PokeDex_IsSeen(pokedex, list[i]) == 1) {
            if (current == list[i]) {
                return;
            }
            if (first == 0xffff) {
                first = list[i];
            }
        }
    }
    if (first != 0xffff) {
        PokeDex_SetLastSpecies(pokedex, first);
    }
}

BOOL PokedexCommon_IsFirstShown(PokeDexSave *pokedex) {
    if (PokeDex_GetMode(pokedex) == 1) {
        return TRUE;
    }
    if (PokeDex_IsCaught(pokedex, SPECIES_VICTINI) == 1 || PokeDex_IsSeen(pokedex, SPECIES_VICTINI) == 1) {
        return TRUE;
    }
    return FALSE;
}

void PokedexCommon_InitCriteria(PokeDexSave *pokedex, PokedexSearchCriteria *criteria) {
    if (PokeDex_GetMode(pokedex) == 1) {
        criteria->regional = 0;
    } else {
        criteria->regional = 1;
    }
    criteria->order = 0;
    criteria->initial = 0xff;
    criteria->type1 = 0xff;
    criteria->type2 = 0xff;
    criteria->color = 0xff;
    criteria->shape = 0xff;
}

void PokedexCommon_WipeIn(HeapID heapId) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, 0, 6, 1, heapId);
}

void PokedexCommon_WipeOut(HeapID heapId) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, 0, 6, 1, heapId);
}

void PokedexCommon_ScrollBGs(u8 bg1, u8 bg2, int *timer) {
    (*timer)++;
    if (*timer == 4) {
        GFL_BGSysMoveBGReq(bg1, BG_MOVE_RIGHT, 1);
        GFL_BGSysMoveBGReq(bg2, BG_MOVE_RIGHT, 1);
        *timer = 0;
    }
}

ClActor *PokedexCommon_CreateActor(ClActUnit *unit, const PokedexActorTemplate *template, HeapID heapId) {
    return ClActor_Create(unit, template->chars, template->palette, template->cells, &template->setup,
                          template->surface, heapId);
}

u32 PokedexCommon_LoadHabitatSpecies(PokeDexSave *pokedex, PokedexHabitatSave *habitat, u16 **outList, HeapID heapId) {
    u16 total;
    PokedexHabitatFile file;
    u16 zone;
    u16 *list;
    int areaFile;
    ArcTool *arc;
    u8 season;
    u8 type;
    u32 i;
    u32 n = 0;

    total = 0;
    zone = PokedexHabitatSave_GetZone(habitat);
    type = PokedexHabitatSave_GetType(habitat);
    season = PokedexHabitatSave_GetSeason(habitat);
    arc = GFL_ArcSysCreateFileHandle(ARCID_POKEDEX_HABITAT, heapId);
    areaFile = PokedexCommon_GetAreaFile(zone);
    if (areaFile < 0) {
        areaFile = 0;
    }
    MI_CpuFill8(&file, 0, sizeof(PokedexHabitatFile));
    GFL_ArcToolRead(arc, areaFile, &file);
    list = PokedexCommon_CreateSpeciesList(&file, pokedex, type, season, &total, heapId);
    for (i = 0; i < file.count; i++) {
        if (PokedexCommon_IsInHabitat(pokedex, &file.entries[i], type, season, file.values[type]) == TRUE) {
            list[n] = file.entries[i].species;
            n++;
        }
    }
    *outList = list;
    GFL_ArcToolFree(arc);
    return total;
}

void PokedexCommon_BuildHabitatList(PokedexHabitatSave *habitat, PokedexHabitatListParam *param, BOOL current) {
    PokeDexSave *pokedex;
    PokedexHabitatConditions conditions;
    int habitatIndex;
    EventWork *eventWork = GameData_GetEventWork(param->gameData);
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEDEX_HABITAT, HEAPID_POKEDEX);
    int index;

    PokedexCommon_GetConditions(&conditions, eventWork);
    pokedex = GameData_GetPokedex(param->gameData);
    if (current == FALSE) {
        param->count = PokedexCommon_CountAreas(eventWork, arc, &conditions);
    } else {
        param->count = 1;
    }
    if (param->count != 0) {
        MI_CpuFill8(param->areas, 0, sizeof(param->areas));
        index = 0;
        for (habitatIndex = 0; habitatIndex < POKEDEX_HABITAT_COUNT; habitatIndex++) {
            const PokedexHabitatTableEntry *entry = &sPokedexHabitats[habitatIndex];
            BOOL flag = EventWork_FlagGet(eventWork, entry->flag);

            if (current == TRUE) {
                if (entry->zone != PokedexHabitatSave_GetZone(habitat)) {
                    flag = FALSE;
                }
            }
            if (flag == TRUE) {
                PokedexHabitatFile file;

                MI_CpuFill8(&file, 0, sizeof(PokedexHabitatFile));
                GFL_ArcToolRead(arc, entry->file, &file);
                if (PokedexCommon_IsAreaShown(&file, &conditions)) {
                    int j;
                    int k;
                    PokedexHabitatSpecies *species;
                    PokedexHabitatArea *area;

                    area = &param->areas[index];
                    MI_CpuFill8(area, 0, 4);
                    area->complete = 1;
                    area->zone = entry->zone;
                    area->unk8 = file.unk6;
                    PokedexCommon_GetHabitatStates(area->states, &file, pokedex);
                    for (j = 0; j < 3; j++) {
                        area->values[j] = file.values[j];
                        if (file.conditions[j] == 1) {
                            if (area->states[j] != 2 || conditions.conditions[j] == 0) {
                                area->complete = 0;
                            }
                        }
                    }
                    for (k = 0; k < file.count; k++) {
                        u8 season;

                        species = &area->species[k];
                        species->species = file.entries[k].species;
                        for (season = 0; season < POKEDEX_SEASON_COUNT; season++) {
                            u8 type;

                            for (type = 0; type < POKEDEX_HABITAT_TYPE_COUNT; type++) {
                                species->present[season][type] =
                                    file.entries[k].present[season][type] & conditions.conditions[type];
                                species->other[season][type] = file.entries[k].other[season][type];
                            }
                        }
                        if (PokeDex_IsCaught(pokedex, species->species)) {
                            species->status = 3;
                        } else if (PokeDex_IsSeen(pokedex, species->species)) {
                            species->status = 2;
                        } else {
                            species->status = 1;
                        }
                    }
                    index++;
                }
            }
        }
        GFL_ArcToolFree(arc);
    }
}

void PokedexCommon_BuildHabitatSummary(PokedexHabitatSave *habitat, PokedexHabitatSummaryParam *param) {
    PokedexHabitatConditions conditions;
    PokeDexSave *pokedex;
    EventWork *eventWork = GameData_GetEventWork(param->gameData);
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEDEX_HABITAT, HEAPID_POKEDEX);
    int habitatIndex;
    int index;
    int i;
    PokedexHabitatFile countFile;
    PokedexHabitatSave *habitatSave = SaveControl_GetPokedexHabitatSave(GameData_GetSaveControl(param->gameData));
    PokedexHabitatFile file;

    pokedex = GameData_GetPokedex(param->gameData);

    PokedexCommon_GetConditions(&conditions, eventWork);
    for (i = 0; i < POKEDEX_HABITAT_COUNT; i++) {
        const PokedexHabitatTableEntry *entry = &sPokedexHabitats[i];

        if (EventWork_FlagGet(eventWork, entry->flag) == 1) {
            MI_CpuFill8(&countFile, 0, sizeof(PokedexHabitatFile));
            GFL_ArcToolRead(arc, entry->file, &countFile);
            if (PokedexCommon_IsAreaShown(&countFile, &conditions)) {
                param->count++;
            }
        }
    }
    MI_CpuFill8(param->areas, 0, sizeof(param->areas));
    index = 0;
    for (habitatIndex = 0; habitatIndex < POKEDEX_HABITAT_COUNT; habitatIndex++) {
        const PokedexHabitatTableEntry *entry = &sPokedexHabitats[habitatIndex];

        if (EventWork_FlagGet(eventWork, entry->flag) == 1) {
            u8 j = 0;

            MI_CpuFill8(&file, 0, sizeof(PokedexHabitatFile));
            GFL_ArcToolRead(arc, entry->file, &file);
            if (PokedexCommon_IsAreaShown(&file, &conditions)) {
                PokedexHabitatSummary *area = &param->areas[index];
                u8 slot;

                area->zone = entry->zone;
                area->conditions[0] = file.conditions[0];
                area->conditions[1] = file.conditions[1];
                area->conditions[2] = file.conditions[2];
                area->conditionsMet[0] = conditions.conditions[0];
                area->conditionsMet[1] = conditions.conditions[1];
                area->conditionsMet[2] = conditions.conditions[2];
                PokedexCommon_GetHabitatStates(area->states, &file, pokedex);
                slot = habitatIndex;
                for (j = 0; j < 3; j++) {
                    u8 done = PokedexHabitatSave_GetHabitatField(habitatSave, slot, j);

                    if (area->conditions[j] == 1) {
                        switch (area->states[j]) {
                        case 2:
                            if (done == 2) {
                                area->display[j] = 2;
                            } else {
                                area->display[j] = 1;
                            }
                            break;
                        case 1:
                            if (done == 1) {
                                area->display[j] = 3;
                            } else {
                                area->display[j] = 1;
                            }
                            break;
                        case 0:
                            area->display[j] = 4;
                            break;
                        }
                    } else {
                        area->display[j] = 4;
                    }
                }
                index++;
            }
        }
    }
    GFL_ArcToolFree(arc);
}

static void PokedexCommon_GetHabitatStates(u32 *states, const PokedexHabitatFile *file, PokeDexSave *pokedex) {
    int i;
    u8 present[3];
    u8 j;
    u8 k;

    states[0] = 2;
    states[1] = 2;
    states[2] = 2;
    for (i = 0; i < file->count; i++) {
        MI_CpuFill8(present, 0, 3);
        for (j = 0; j < 3; j++) {
            for (k = 0; k < POKEDEX_SEASON_COUNT; k++) {
                present[j] |= file->entries[i].present[k][j];
            }
        }
        for (j = 0; j < 3; j++) {
            if (present[j] >= 1) {
                if (states[j] == 2 && !PokeDex_IsCaught(pokedex, file->entries[i].species)) {
                    states[j] = 1;
                }
                if (states[j] == 1 && !PokeDex_IsSeen(pokedex, file->entries[i].species)) {
                    states[j] = 0;
                }
            }
        }
    }
}

static BOOL PokedexCommon_IsAreaShown(const PokedexHabitatFile *file, const PokedexHabitatConditions *conditions) {
    int i;

    for (i = 0; i < 3; i++) {
        if (file->conditions[i] == 1 && conditions->conditions[i] == 1) {
            return TRUE;
        }
    }
    return FALSE;
}

static void PokedexCommon_GetConditions(PokedexHabitatConditions *conditions, EventWork *eventWork) {
    conditions->conditions[0] = TRUE;
    conditions->conditions[1] = EventWork_FlagGet(eventWork, FLAG_CONDITION_1);
    conditions->conditions[2] = EventWork_FlagGet(eventWork, FLAG_CONDITION_2);
}

static int PokedexCommon_GetAreaFile(u16 zone) {
    int i;

    for (i = 0; i < POKEDEX_HABITAT_COUNT; i++) {
        if (zone == sPokedexHabitats[i].zone) {
            return sPokedexHabitats[i].file;
        }
    }
    return FILE_NONE;
}

static u16 *PokedexCommon_CreateSpeciesList(const PokedexHabitatFile *file, PokeDexSave *pokedex, u8 type, u8 season,
                                            u16 *total, HeapID heapId) {
    int i = 0;

    *total = 0;
    for (; i < file->count; i++) {
        if (PokedexCommon_IsInHabitat(pokedex, &file->entries[i], type, season, file->values[type]) == TRUE) {
            (*total)++;
        }
    }
    return GFL_HeapAllocate(heapId, *total * 2, TRUE, "zukan_common.c", 867);
}

static u32 PokedexCommon_CountAreas(EventWork *eventWork, ArcTool *arc, const PokedexHabitatConditions *conditions) {
    PokedexHabitatFile file;
    int i;
    u32 count = 0;

    for (i = 0; i < POKEDEX_HABITAT_COUNT; i++) {
        const PokedexHabitatTableEntry *entry = &sPokedexHabitats[i];

        if (EventWork_FlagGet(eventWork, entry->flag) == 1) {
            MI_CpuFill8(&file, 0, sizeof(PokedexHabitatFile));
            GFL_ArcToolRead(arc, entry->file, &file);
            if (PokedexCommon_IsAreaShown(&file, conditions)) {
                count++;
            }
        }
    }
    return count;
}

void PokedexCommon_StepGlow(u16 *phase, int *ev) {
    u16 current = *phase;
    int next = current + 0x400;
    s16 value;

    if (next >= 0x10000) {
        *phase = current - 0xfc00;
    } else {
        *phase = next;
    }
    value = FX_CosIdx(*phase);
    value = (value + FX32_ONE) / 2;
    *ev = ((value * 14) >> FX32_SHIFT) + 1;
}

void PokedexCommon_StartGlow(u16 *phase, int *ev) {
    *ev = 1;
    *phase = 0x8000;
}
