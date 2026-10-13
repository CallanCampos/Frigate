#include "types.h"
#include "app/zukan_search_engine.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "pml/personal.h"
#include "save/pokedex.h"

// The Pokédex's search: the species of the list that match the search screen's criteria, found a few at a time

// The archive of the search's data, and its files: the forms of the shape lists, the initials' groups and the shapes
#define ARCID_ZUKAN_SEARCH 97
#define FILE_FORMS 0
#define FILE_INITIALS 1
#define FILE_SHAPES 2
// The file of the national Pokédex's species, in their order
#define FILE_NATIONAL_ORDER 4
// The number of species of the national Pokédex
#define NATIONAL_COUNT 649
// A species' value in the regional Pokédex's table when it is not in it
#define REGIONAL_NONE 999
// The species and the form of an entry of the list
#define ENTRY_SPECIES_MASK 0x3ff
#define ENTRY_FORM_MASK 0xfc00
#define ENTRY_FORM_SHIFT 10
// The entries that a step looks at
#define STEP_ENTRIES 7
// Any value of a criterion
#define CRITERIA_ANY 0xff

// The steps of the search
enum {
    STEP_OPEN,
    STEP_LOAD_LIST,
    STEP_LOAD_TABLES,
    STEP_FILTER,
    STEP_FINISH,
    STEP_DONE,
};

// The orders of the lists that the search engine can load, in a file each
enum {
    ORDER_DEFAULT,
};

struct ZukanSearchEngine {
    PokeDexSave *pokedex;
    const ZukanSearchCriteria *criteria;
    u16 step;
    ArcTool *arc;
    // Where the filter is, and the number of entries of the list
    u16 position;
    u16 total;
    u16 *list;
    // For each entry of the list, 1 if the species does not match
    u8 *excluded;
    u16 unk1c;
    u16 *regionalTable;
    u16 initialsSize;
    u8 *initials;
    u16 shapesSize;
    u8 *shapes;
    u16 formCount;
    u16 *forms;
    void *personal;
    u16 resultCount;
    u16 *result;
};

static u16 ZukanSearchEngine_LoadOrder(ArcTool *arc, u32 regional, u32 order, HeapID heapId, u16 **list);
static void ZukanSearchEngine_LoadTables(ArcTool *arc, HeapID heapId, u16 *unk1c, u16 **regionalTable,
                                         u16 *initialsSize, u8 **initials, u16 *shapesSize, u8 **shapes, u16 *formCount,
                                         u16 **forms);
static u8 ZukanSearchEngine_GetShape(const u8 *shapes, const u16 *forms, u16 species, u16 form);

// The files of the lists in each order, which the search engine reads with them
static const u32 sZukanSearchOrderFiles[6] = { 4, 5, 8, 9, 6, 7 };

ZukanSearchEngine *ZukanSearchEngine_Create(PokeDexSave *pokedex, const ZukanSearchCriteria *criteria, HeapID heapId) {
    ZukanSearchEngine *engine = GFL_HeapAllocate(heapId, sizeof(ZukanSearchEngine), TRUE, "zukan_search_engine.c", 462);

    engine->pokedex = pokedex;
    engine->criteria = criteria;
    engine->step = 0;
    engine->arc = NULL;
    engine->position = 0;
    engine->personal = NULL;
    engine->resultCount = 0;
    engine->result = NULL;
    return engine;
}

void ZukanSearchEngine_Free(ZukanSearchEngine *engine) {
    GFL_HeapFree(engine);
}

BOOL ZukanSearchEngine_Step(ZukanSearchEngine *engine, HeapID heapId, u16 *count, u16 **list) {
    switch (engine->step) {
    case STEP_OPEN:
        engine->arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_SEARCH, heapId);
        engine->step = STEP_LOAD_LIST;
        break;
    case STEP_LOAD_LIST:
        engine->total = ZukanSearchEngine_LoadOrder(engine->arc, engine->criteria->regional, engine->criteria->order,
                                                    heapId, &engine->list);
        engine->excluded = GFL_HeapAllocate(heapId, engine->total, TRUE, "zukan_search_engine.c", 551);
        engine->step = STEP_LOAD_TABLES;
        break;
    case STEP_LOAD_TABLES:
        ZukanSearchEngine_LoadTables(engine->arc, heapId, &engine->unk1c, &engine->regionalTable, &engine->initialsSize,
                                     &engine->initials, &engine->shapesSize, &engine->shapes, &engine->formCount,
                                     &engine->forms);
        engine->step = STEP_FILTER;
        break;
    case STEP_FILTER: {
        u16 total = engine->total;
        u16 i = engine->position;
        u16 end;

        if (i >= total) {
            engine->step = STEP_FINISH;
            break;
        }
        end = i + STEP_ENTRIES;
        if (end > total) {
            end = total;
        }
        engine->position = end;
        for (; i < end; i++) {
            u16 species = engine->list[i] & ENTRY_SPECIES_MASK;
            u16 form = (engine->list[i] & ENTRY_FORM_MASK) >> ENTRY_FORM_SHIFT;
            u32 sex;
            u32 rare;
            u32 shownForm;
            u16 displayedForm;
            u8 type1;
            u8 type2;

            if (!PokeDex_IsSeen(engine->pokedex, species)) {
                engine->excluded[i] = 1;
                continue;
            }
            func_0200d3c8(engine->pokedex, species, &sex, &rare, &shownForm, heapId);
            displayedForm = shownForm;
            if (engine->total == NATIONAL_COUNT) {
                form = displayedForm;
            }
            if (form != displayedForm) {
                engine->excluded[i] = 1;
                continue;
            }
            if ((engine->criteria->order >= 2 && engine->criteria->order <= 5) ||
                engine->criteria->type1 != CRITERIA_ANY || engine->criteria->type2 != CRITERIA_ANY) {
                if (!PokeDex_IsCaught(engine->pokedex, species)) {
                    engine->excluded[i] = 1;
                    continue;
                }
            }
            if (engine->criteria->regional == 1 && engine->regionalTable[species] == REGIONAL_NONE) {
                engine->excluded[i] = 1;
                continue;
            }
            if (engine->criteria->initial != CRITERIA_ANY && engine->criteria->initial != engine->initials[species]) {
                engine->excluded[i] = 1;
                continue;
            }
            if (engine->personal != NULL) {
                PML_PersonalFree(engine->personal);
            }
            engine->personal = PML_PersonalLoad(species, form, heapId);
            type1 = PML_PersonalGetParam(engine->personal, 6);
            type2 = PML_PersonalGetParam(engine->personal, 7);
            if (engine->criteria->type1 != CRITERIA_ANY && engine->criteria->type2 != CRITERIA_ANY) {
                if (!((type1 == engine->criteria->type1 && type2 == engine->criteria->type2) ||
                      (type1 == engine->criteria->type2 && type2 == engine->criteria->type1))) {
                    engine->excluded[i] = 1;
                    continue;
                }
            } else if (engine->criteria->type1 != CRITERIA_ANY && engine->criteria->type2 == CRITERIA_ANY) {
                if (type1 != engine->criteria->type1 && type2 != engine->criteria->type1) {
                    engine->excluded[i] = 1;
                    continue;
                }
            } else if (engine->criteria->type1 == CRITERIA_ANY && engine->criteria->type2 != CRITERIA_ANY) {
                if (type1 != engine->criteria->type2 && type2 != engine->criteria->type2) {
                    engine->excluded[i] = 1;
                    continue;
                }
            }
            if (engine->criteria->color != CRITERIA_ANY) {
                u8 color = PML_PersonalGetParam(engine->personal, 0x21);

                if (color != engine->criteria->color) {
                    engine->excluded[i] = 1;
                    continue;
                }
            }
            if (engine->criteria->shape != CRITERIA_ANY) {
                u8 shape = ZukanSearchEngine_GetShape(engine->shapes, engine->forms, species, form);

                if (shape != engine->criteria->shape) {
                    engine->excluded[i] = 1;
                    continue;
                }
            }
        }
        break;
    }
    case STEP_FINISH: {
        u16 found;
        u16 i;
        u16 n;
        u16 *result;

        if (engine->personal != NULL) {
            PML_PersonalFree(engine->personal);
        }
        found = 0;
        for (i = 0; i < engine->total; i++) {
            if (engine->excluded[i] == 0) {
                found++;
            }
        }
        if (found != 0) {
            result = GFL_HeapAllocate(heapId, found * 2, FALSE, "zukan_search_engine.c", 742);
            n = 0;
            for (i = 0; i < engine->total; i++) {
                if (engine->excluded[i] == 0) {
                    result[n] = engine->list[i] & ENTRY_SPECIES_MASK;
                    n++;
                }
            }
        } else {
            result = NULL;
        }
        GFL_HeapFree(engine->list);
        GFL_HeapFree(engine->excluded);
        GFL_HeapFree(engine->regionalTable);
        GFL_HeapFree(engine->initials);
        GFL_HeapFree(engine->shapes);
        GFL_HeapFree(engine->forms);
        engine->resultCount = found;
        engine->result = result;
        GFL_ArcToolFree(engine->arc);
        engine->step = STEP_DONE;
        break;
    }
    case STEP_DONE:
    default:
        *list = engine->result;
        *count = engine->resultCount;
        return TRUE;
    }
    return FALSE;
}

u32 ZukanSearchEngine_LoadList(u32 type, HeapID heapId, u16 **outList) {
    u16 size;

    if (type == 0) {
        u32 bytes = 0x512;
        u16 *list = GFL_HeapAllocate(heapId, bytes, FALSE, "zukan_search_engine.c", 825);
        u16 i;

        for (i = 0; i < bytes / 2; i++) {
            list[i] = i + 1;
        }
        *outList = list;
        return NATIONAL_COUNT;
    } else {
        u16 *regionalTable = PML_PersonalLoadRegionalDexTable(heapId, &size);
        u32 length;
        u16 *national = GFL_ArcSysReadHeapNewLZGetLen(ARCID_ZUKAN_SEARCH, FILE_NATIONAL_ORDER, FALSE, heapId, &length);
        u16 count = length >> 1;
        u16 *list = GFL_HeapAllocate(heapId, size * 2, FALSE, "zukan_search_engine.c", 862);
        u16 n = 0;
        u16 i;

        for (i = 0; i < count; i++) {
            u16 species = national[i];

            if (regionalTable[species] != REGIONAL_NONE) {
                list[n] = species;
                n++;
            }
        }
        GFL_HeapFree(national);
        GFL_HeapFree(regionalTable);
        *outList = list;
        return size;
    }
}

static u16 ZukanSearchEngine_LoadOrder(ArcTool *arc, u32 regional, u32 order, HeapID heapId, u16 **list) {
    u16 count;
    u16 *data;

    if (regional == 0 && order == 0) {
        u32 bytes = 0x512;
        u16 i;

        data = GFL_HeapAllocate(heapId, bytes, FALSE, "zukan_search_engine.c", 945);
        for (i = 0; i < bytes / 2; i++) {
            data[i] = i + 1;
        }
        count = NATIONAL_COUNT;
    } else {
        u32 length;

        data = GFL_ArcToolReadHeapNewLZGetLen(arc, sZukanSearchOrderFiles[order], FALSE, heapId, &length);
        count = length >> 1;
    }
    *list = data;
    return count;
}

static void ZukanSearchEngine_LoadTables(ArcTool *arc, HeapID heapId, u16 *unk1c, u16 **regionalTable,
                                         u16 *initialsSize, u8 **initials, u16 *shapesSize, u8 **shapes, u16 *formCount,
                                         u16 **forms) {
    u32 length;

    *regionalTable = PML_PersonalLoadRegionalDexTable(heapId, 0);
    *unk1c = 650;
    *initials = GFL_ArcToolReadHeapNewLZGetLen(arc, FILE_INITIALS, FALSE, heapId, &length);
    *initialsSize = length;
    *shapes = GFL_ArcToolReadHeapNewLZGetLen(arc, FILE_SHAPES, FALSE, heapId, &length);
    *shapesSize = length;
    *forms = GFL_ArcToolReadHeapNewLZGetLen(arc, FILE_FORMS, FALSE, heapId, &length);
    *formCount = length >> 1;
}

static u8 ZukanSearchEngine_GetShape(const u8 *shapes, const u16 *forms, u16 species, u16 form) {
    u16 i = 0;

    if (form != 0) {
        do {
            species = forms[species];
            if (species == 0) {
                break;
            }
            i++;
        } while (i != form);
    }
    return shapes[species];
}
