// The ROM names this file waza_oshie.c.
// The move reminder's parameters and the moves a Pokémon can remember. PokeParty_GetRememberableMoves and
// doesPkmHaveLevelMoveToLearn are swan's names (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "constants/pokemon.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "pml/move_reminder.h"
#include "pml/personal.h"
#include "pml/poke_party.h"

MoveReminderProcessData *MoveReminderProcessData_Create(HeapID heapId) {
    MoveReminderProcessData *data =
        GFL_HeapAllocate(heapId, sizeof(MoveReminderProcessData), FALSE, "waza_oshie.c", 35);

    MI_CpuFill8(data, 0, sizeof(MoveReminderProcessData));
    return data;
}

void MoveReminderProcessData_Free(MoveReminderProcessData *data) {
    GFL_HeapFree(data);
}

u16 *PokeParty_GetRememberableMoves(PartyPkm *pkm, HeapID heapId) {
    LearnsetEntry *learnset;
    u16 *moves;
    u16 species;
    u8 form;
    u8 level;
    u16 known[4];
    u8 count;
    u8 i;
    u8 j;

    species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
    level = PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);
    for (i = 0; i < 4; i++) {
        known[i] = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + i, NULL);
    }
    learnset = GFL_HeapAllocate(heapId, LEARNSET_MAX * sizeof(LearnsetEntry), FALSE, "waza_oshie.c", 85);
    moves = GFL_HeapAllocate(heapId, LEARNSET_MAX * sizeof(u16), FALSE, "waza_oshie.c", 86);
    PML_LearnsetLvUpLoad(species, form, learnset);
    count = 0;
    for (i = 0; i < LEARNSET_MAX; i++) {
        if (LearnsetEntry_IsEnd(learnset[i])) {
            moves[count] = LEARNSET_END;
            break;
        }
        if (LearnsetEntry_GetLevel(learnset[i]) > level) {
            continue;
        }
        for (j = 0; j < 4; j++) {
            if (known[j] == LearnsetEntry_GetMove(learnset[i])) {
                break;
            }
        }
        if (j != 4) {
            continue;
        }
        for (j = 0; j < count; j++) {
            if (moves[j] == LearnsetEntry_GetMove(learnset[i])) {
                break;
            }
        }
        if (j != count) {
            continue;
        }
        moves[count] = LearnsetEntry_GetMove(learnset[i]);
        count++;
    }
    GFL_HeapFree(learnset);
    return moves;
}

BOOL doesPkmHaveLevelMoveToLearn(const u16 *moves) {
    if (moves[0] != LEARNSET_END) {
        return TRUE;
    }
    return FALSE;
}
