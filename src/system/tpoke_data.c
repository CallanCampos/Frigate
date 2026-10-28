#include "types.h"
#include "constants/arc.h"
#include "field/field_actor.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "system/tpoke_data.h"

// The field models of walking Pokémon: which object code a species, sex and form use. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except TPokeEntry, TPOKE_SEX_ANY and the fields

// An entry's sex when it fits both
#define TPOKE_SEX_ANY 2

// An entry of ARCID_TPOKE's table
typedef struct {
    u16 species;
    u16 sex;
    u16 form;
    u16 objCode;
} TPokeEntry;

struct TPokeData {
    TPokeEntry *entries;
    void *mmodelTable;
    // The object codes' records, from 4 bytes into ARCID_MMODEL_TBL
    FieldActorConfig *configs;
    u32 count;
};

static int GetFieldPokemonActorLUTIndex(TPokeData *data, u16 species, u16 sex, u16 form);
static FieldActorConfig *GetIndexOfTPokeObjCode(TPokeData *data, u16 objCode);

TPokeData *LoadTPokeData(HeapID heapId) {
    u32 size;
    TPokeData *data = GFL_HeapAllocate(heapId, sizeof(TPokeData), TRUE, "tpoke_data.c", 82);

    data->entries = GFL_ArcSysReadHeapNewLZGetLen(ARCID_TPOKE, 0, FALSE, heapId, &size);
    data->count = size / sizeof(TPokeEntry);
    data->mmodelTable = GFL_ArcSysReadHeapNew(ARCID_MMODEL_TBL, 0, heapId);
    data->configs = (FieldActorConfig *)((u8 *)data->mmodelTable + 4);
    return data;
}

void FreeTPokeData(TPokeData *data) {
    GFL_HeapFree(data->mmodelTable);
    GFL_HeapFree(data->entries);
    GFL_HeapFree(data);
}

u16 GetPokemonFieldOBJCODE(TPokeData *data, u16 species, u16 sex, u16 form) {
    int index = GetFieldPokemonActorLUTIndex(data, species, sex, form);

    return data->entries[index].objCode;
}

BOOL IsFieldPokemonSpriteHugeBillboard(GameData *unused, TPokeData *data, u16 species, u16 sex, u16 form) {
    int index = GetFieldPokemonActorLUTIndex(data, species, sex, form);
    FieldActorConfig *config = GetIndexOfTPokeObjCode(data, data->entries[index].objCode);

    if (config->billboardSize == 2) {
        return TRUE;
    }
    return FALSE;
}

int GetFieldPokemonActorLUTIndex_(const TPokeData *data, u16 species, u16 sex, u16 form) {
    u32 i;

    for (i = 0; i < data->count; i++) {
        if (species == data->entries[i].species &&
            (data->entries[i].sex == TPOKE_SEX_ANY || sex == data->entries[i].sex) && form == data->entries[i].form) {
            return i;
        }
    }
    return TPOKE_INDEX_NONE;
}

static int GetFieldPokemonActorLUTIndex(TPokeData *data, u16 species, u16 sex, u16 form) {
    int index = GetFieldPokemonActorLUTIndex_(data, species, sex, form);

    // BUG: GetFieldPokemonActorLUTIndex_ returns TPOKE_INDEX_NONE, not a negative index, when no entry fits, so a
    // Pokémon missing from the table reads entry 0xFFFF, past the end of the table, instead of entry 0
#ifdef BUGFIX
    if (index == TPOKE_INDEX_NONE) {
#else
    if (index < 0) {
#endif
        index = 0;
    }
    return index;
}

static FieldActorConfig *GetIndexOfTPokeObjCode(TPokeData *data, u16 objCode) {
    return &data->configs[GetIndexOfObjID(objCode)];
}
