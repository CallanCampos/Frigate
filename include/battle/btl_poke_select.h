#ifndef POKEBW2_BATTLE_BTL_POKE_SELECT_H
#define POKEBW2_BATTLE_BTL_POKE_SELECT_H

// Overlay 169's btl_poke_select.c (named descriptively, as the original file isn't known: the code is a small
// accessor set for the two structs the battle client hands the party list screen, so it could also be the head of
// the next file): the party list command's parameters and the list of party Pokemon the player chose to send out.
// Names are not swan's, which has none for these

#include "types.h"
#include "struct_decls.h"

#define POKE_LIST_MAX_SLOTS 6
#define POKE_SELECT_MAX 3

// What the party list screen shows: the party, how many Pokemon it lists, a state for each of them and the mode
struct BtlvPokeListCmd {
    BattleParty *party;
    u8 count;
    u8 slotState[POKE_LIST_MAX_SLOTS];
    u8 mode;
};

// The Pokemon chosen on the party list, in order, each with the battle position it replaces
struct BtlvPokeSelectParam {
    u8 partyIndex[POKE_SELECT_MAX];
    u8 position[POKE_SELECT_MAX];
    u8 count;
    u8 max;
    u8 selecting;
};

void PokeListCmd_Init(BtlvPokeListCmd *cmd, BattleParty *party, u8 count, u8 mode);
void PokeListCmd_SetCoverSlots(BtlvPokeListCmd *cmd, u8 numCoverPos);
void PokeSelect_Init(BtlvPokeSelectParam *select, const BtlvPokeListCmd *cmd);
void PokeSelect_Push(BtlvPokeSelectParam *select, u8 index, u8 slot);
void PokeSelect_Pop(BtlvPokeSelectParam *select);
BOOL PokeSelect_IsSelecting(BtlvPokeSelectParam *select);
u32 PokeSelect_GetCount(BtlvPokeSelectParam *select);
u8 PokeSelect_GetLast(BtlvPokeSelectParam *select);
u8 PokeSelect_Get(BtlvPokeSelectParam *select, u8 index);

#endif // POKEBW2_BATTLE_BTL_POKE_SELECT_H
