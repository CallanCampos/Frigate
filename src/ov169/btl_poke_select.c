#include "battle/btl_poke_select.h"
#include "types.h"

static void PokeListCmd_SetSlotState(BtlvPokeListCmd *cmd, u8 state, u8 slot);

void PokeListCmd_Init(BtlvPokeListCmd *cmd, BattleParty *party, u8 count, u8 mode) {
    u32 i;

    cmd->mode = mode;
    cmd->count = count;
    cmd->party = party;
    for (i = 0; i < POKE_LIST_MAX_SLOTS; i++) {
        cmd->slotState[i] = 0;
    }
}

void PokeListCmd_SetCoverSlots(BtlvPokeListCmd *cmd, u8 numCoverPos) {
    u8 i;

    for (i = 0; i < numCoverPos; i++) {
        PokeListCmd_SetSlotState(cmd, 2, i);
    }
}

static void PokeListCmd_SetSlotState(BtlvPokeListCmd *cmd, u8 state, u8 slot) {
    cmd->slotState[slot] = state;
}

void PokeSelect_Init(BtlvPokeSelectParam *select, const BtlvPokeListCmd *cmd) {
    select->count = 0;
    select->max = cmd->count;
}

void PokeSelect_Push(BtlvPokeSelectParam *select, u8 index, u8 slot) {
    if (select->count < select->max) {
        select->partyIndex[select->count] = slot;
        select->position[select->count] = index;
        select->count++;
    }
}

void PokeSelect_Pop(BtlvPokeSelectParam *select) {
    if (select->count != 0) {
        select->count--;
    }
}

BOOL PokeSelect_IsSelecting(BtlvPokeSelectParam *select) {
    return select->selecting;
}

u32 PokeSelect_GetCount(BtlvPokeSelectParam *select) {
    return select->count;
}

u8 PokeSelect_GetLast(BtlvPokeSelectParam *select) {
    if (select->count != 0) {
        return select->partyIndex[select->count - 1];
    }
    return POKE_LIST_MAX_SLOTS;
}

u8 PokeSelect_Get(BtlvPokeSelectParam *select, u8 index) {
    if (index < select->count) {
        return select->partyIndex[index];
    }
    return POKE_LIST_MAX_SLOTS;
}
