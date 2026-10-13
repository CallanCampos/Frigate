#ifndef POKEBW2_BATTLE_BTL_WAZAREC_H
#define POKEBW2_BATTLE_BTL_WAZAREC_H

// Overlay 169's btl_wazarec.c, named after the ring of move records it keeps (GF calls moves "waza"): every move a
// mon uses in the battle, with the turn it was used in. Names from swan (https://github.com/ds-pokemon-hacking/swan,
// GPL-3.0) where it has them

#include "types.h"
#include "struct_decls.h"

#define BTL_WAZAREC_MAX 120

typedef struct BtlWazaRecord {
    u32 turn;
    u16 move;
    u8 monId;
    u8 succeeded; // set when the move took effect (BtlWazaRec_MarkLastSucceeded)
} BtlWazaRecord;

// The newest record is the one before head; head wraps around over the oldest
typedef struct BtlWazaRec {
    u32 head;
    BtlWazaRecord records[BTL_WAZAREC_MAX];
} BtlWazaRec;

// Empties the ring
void BtlWazaRec_Init(BtlWazaRec *rec);
// Records a use of a move by a mon in a turn
void BtlWazaRec_Add(BtlWazaRec *rec, u16 move, u32 turn, u8 monId);
// Marks the newest record as a move that took effect
void BtlWazaRec_MarkLastSucceeded(BtlWazaRec *rec);
// Whether a move was used in a turn
BOOL BtlWazaRec_IsUsed(BtlWazaRec *rec, u16 move, u32 turn);
// How many times a move was used in a turn
u32 GetUsedMoveCount(BtlWazaRec *rec, u16 move, u32 turn);
// The newest move of a turn that took effect, or MOVE_NONE
u16 BtlWazaRec_GetSucceededMove(BtlWazaRec *rec, u32 turn);

#endif // POKEBW2_BATTLE_BTL_WAZAREC_H
