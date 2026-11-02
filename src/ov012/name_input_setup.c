// The ROM names this file namein_setup.c.
// The parameters of the name entry (overlay 280). The name is the file's own; setupNameEntry, setupPokemonNameEntry
// and pokemonNameEntry are swan's names (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "app/name_entry.h"
#include "constants/pokemon.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "pml/poke_party.h"

#define NAME_ENTRY_NAME_LENGTH 11

NameEntryParam *setupNameEntry(HeapID heapId, u32 mode, u32 gender, u32 sexAndForm, u32 maxLength, const StrBuf *name,
                               TrainerGameInfoSave *gameInfo) {
    NameEntryParam *param = GFL_HeapAllocate(heapId, sizeof(NameEntryParam), FALSE, "namein_setup.c", 55);

    MI_CpuFill8(param, 0, sizeof(NameEntryParam));
    param->mode = mode;
    param->maxLength = maxLength;
    param->gender = gender;
    param->sexAndForm = sexAndForm;
    param->gameInfo = gameInfo;
    param->skipChecks = 0;
    if (mode == NAME_ENTRY_GAME_SYNC_ID) {
        param->skipChecks = 3;
    }
    param->name = GFL_StrBufCreate(NAME_ENTRY_NAME_LENGTH, heapId);
    if (name != NULL) {
        GFL_StrBufCopy(param->name, name);
    }
    return param;
}

NameEntryParam *setupPokemonNameEntry(HeapID heapId, PartyPkm *pkm, u32 maxLength, const StrBuf *name,
                                      TrainerGameInfoSave *gameInfo) {
    NameEntryParam *param = GFL_HeapAllocate(heapId, sizeof(NameEntryParam), FALSE, "namein_setup.c", 95);
    u32 form;

    MI_CpuFill8(param, 0, sizeof(NameEntryParam));
    param->mode = NAME_ENTRY_POKEMON;
    param->maxLength = maxLength;
    param->pkm = pkm;
    param->gameInfo = gameInfo;
    param->skipChecks = 0;
    if (pkm != NULL) {
        param->gender = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
        param->sexAndForm = form | (PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL) << 8);
    }
    param->name = GFL_StrBufCreate(NAME_ENTRY_NAME_LENGTH, heapId);
    if (name != NULL) {
        GFL_StrBufCopy(param->name, name);
    }
    return param;
}

NameEntryParam *pokemonNameEntry(HeapID heapId, PartyPkm *pkm, u32 maxLength, const StrBuf *name, StrBuf *boxMessage,
                                 BoxSaveAccessor *boxes, u32 box, TrainerGameInfoSave *gameInfo) {
    NameEntryParam *param = setupPokemonNameEntry(heapId, pkm, maxLength, name, gameInfo);

    param->boxMessage = boxMessage;
    param->boxes = boxes;
    param->box = box;
    return param;
}

void NameEntryParam_Free(NameEntryParam *param) {
    GFL_StrBufFree(param->name);
    GFL_HeapFree(param);
}

void NameEntryParam_GetName(NameEntryParam *param, StrBuf *dest) {
    GFL_StrBufCopy(dest, param->name);
}

BOOL NameEntryParam_IsCanceled(NameEntryParam *param) {
    return param->canceled;
}

BOOL NameEntryParam_CompareName(NameEntryParam *param, const StrBuf *str) {
    return GFL_StrBufCmp(param->name, str);
}
