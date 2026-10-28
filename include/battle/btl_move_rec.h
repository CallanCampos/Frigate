#ifndef POKEBW2_BATTLE_BTL_MOVE_REC_H
#define POKEBW2_BATTLE_BTL_MOVE_REC_H

// Overlay 169's btl_move_rec.c, named after the ring of move records it keeps: every move a
// mon uses in the battle, with the turn it was used in. Names from swan (https://github.com/ds-pokemon-hacking/swan,
// GPL-3.0) where it has them

#include "types.h"
#include "struct_decls.h"

#define BTL_MOVE_REC_MAX 120

typedef struct BtlMoveRecord {
    u32 turn;
    u16 move;
    u8 monId;
    u8 succeeded; // set when the move took effect (BtlMoveRec_MarkLastSucceeded)
} BtlMoveRecord;

// The newest record is the one before head; head wraps around over the oldest
typedef struct BtlMoveRec {
    u32 head;
    BtlMoveRecord records[BTL_MOVE_REC_MAX];
} BtlMoveRec;

// Empties the ring
void BtlMoveRec_Init(BtlMoveRec *rec);
// Records a use of a move by a mon in a turn
void BtlMoveRec_Add(BtlMoveRec *rec, u16 move, u32 turn, u8 monId);
// Marks the newest record as a move that took effect
void BtlMoveRec_MarkLastSucceeded(BtlMoveRec *rec);
// Whether a move was used in a turn
BOOL BtlMoveRec_IsUsed(BtlMoveRec *rec, u16 move, u32 turn);
// How many times a move was used in a turn
u32 GetUsedMoveCount(BtlMoveRec *rec, u16 move, u32 turn);
// The newest move of a turn that took effect, or MOVE_NONE
u16 BtlMoveRec_GetSucceededMove(BtlMoveRec *rec, u32 turn);

#endif // POKEBW2_BATTLE_BTL_MOVE_REC_H
