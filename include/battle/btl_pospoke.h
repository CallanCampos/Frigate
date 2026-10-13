#ifndef POKEBW2_BATTLE_BTL_POSPOKE_H
#define POKEBW2_BATTLE_BTL_POSPOKE_H

// Overlay 169's btl_pospoke.c, named descriptively after the table it keeps: which mon stands at each battle
// position, and where each mon stands. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0) where
// it has them

#include "types.h"
#include "struct_decls.h"

#define BTL_POSPOKE_NUM_POS 6
#define BTL_POS_NONE 6
#define BTL_POSPOKE_NO_MON 0x1f

// A battle position: whether it exists in this battle style, the client it belongs to, and the mon on it
typedef struct BtlPosPokeEntry {
    u8 exists;
    u8 clientId;
    u8 monId; // BTL_POSPOKE_NO_MON when empty
} BtlPosPokeEntry;

typedef struct BtlPosPoke {
    BtlPosPokeEntry entries[BTL_POSPOKE_NUM_POS];
    u8 posOfMon[BTL_POSPOKE_NUM_POS]; // the position of a mon ID, BTL_POS_NONE if it has none
} BtlPosPoke;

// Sets up the positions of a battle style and the mons that start on them (battleStyle is not used)
void BtlPosPoke_Init(BtlPosPoke *posPoke, BtlMainModule *mainModule, BtlPokeCon *pokeCon, u32 battleStyle);
// Forgets the mon with an ID, leaving its position empty
void BtlPosPoke_RemoveMon(BtlPosPoke *posPoke, u8 monId);
// Puts a mon on a position, and has it and the opponents on the field record each other (for EXP)
void BtlPosPoke_SetMon(BtlPosPoke *posPoke, u8 pos, u8 monId, BtlPokeCon *pokeCon);
// Rotates the mons of a client in a rotation battle: the party slot that comes in swaps with the active one
void BtlPosPoke_Rotate(BtlPosPoke *posPoke, u8 slot, u8 clientId, BattleMon *mon, BtlPokeCon *pokeCon);
// Swaps the mons of two positions
void BtlPosPoke_Swap(BtlPosPoke *posPoke, u8 firstPos, u8 secondPos);
// Collects the empty positions of a client into positions and returns how many there are
u8 BtlPosPoke_GetEmptyPositions(BtlPosPoke *posPoke, u8 clientId, u8 *positions);
BOOL DoesBattleMonExist(BtlPosPoke *posPoke, u8 monId);
// Whether a mon has a position that the battle's main module accepts (name guessed)
BOOL BtlPosPoke_IsMonAtActivePos(BtlPosPoke *posPoke, BtlMainModule *mainModule, u8 monId);
// The position of a mon, or BTL_POS_NONE
u8 GetBattlePos(BtlPosPoke *posPoke, u8 monId);
// The position that a mon ID is recorded to be at, even if it has left the position since
u8 BtlPosPoke_GetPosOfMon(BtlPosPoke *posPoke, u8 monId);
// The mon on a position, or BTL_POSPOKE_NO_MON
u8 GetExistPokeID(BtlPosPoke *posPoke, u8 pos);

#endif // POKEBW2_BATTLE_BTL_POSPOKE_H
