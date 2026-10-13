#ifndef POKEBW2_BATTLE_BTL_FAINTREC_H
#define POKEBW2_BATTLE_BTL_FAINTREC_H

// Overlay 169's btl_faintrec.c, named descriptively: the record of the mons that fainted in the last four turns,
// kept by the battle server flow (the "dead record" of other Gen 5 battle decomps). Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0) where it has them

#include "types.h"
#include "struct_decls.h"

#define BTL_FAINTREC_TURNS 4
#define BTL_FAINTREC_MAX_MONS 24
#define BTL_FAINTREC_NO_MON 0x1f

// The mons that fainted in one turn, in the order they did
typedef struct BtlFaintRecTurn {
    u8 count;
    u8 handled[BTL_FAINTREC_MAX_MONS]; // set once the server flow has dealt with the faint
    u8 monIds[BTL_FAINTREC_MAX_MONS];
} BtlFaintRecTurn;

// Turn 0 is the current turn, 1 the one before it, and so on
typedef struct BtlFaintRec {
    BtlFaintRecTurn turns[BTL_FAINTREC_TURNS];
} BtlFaintRec;

// Empties the record of all turns
void BtlFaintRec_Init(BtlFaintRec *rec);
// Moves every turn one back, and starts an empty current turn
void BtlFaintRec_NextTurn(BtlFaintRec *rec);
// Records a mon that fainted in the current turn
void BtlFaintRec_Add(BtlFaintRec *rec, u8 monId);
// How many mons fainted in a turn
u32 BtlFaintRec_GetCount(BtlFaintRec *rec, u32 turn);
// The mon that fainted in a turn at an index, or BTL_FAINTREC_NO_MON
u8 BtlFaintRec_GetMonId(BtlFaintRec *rec, u32 turn, u8 index);
BOOL BtlFaintRec_IsHandled(BtlFaintRec *rec, u32 turn, u8 index);
void BtlFaintRec_SetHandled(BtlFaintRec *rec, u32 turn, u8 index);
// The mon that fainted last, in the newest turn where one did, or BTL_FAINTREC_NO_MON
u8 BtlFaintRec_GetLastFainted(BtlFaintRec *rec);

#endif // POKEBW2_BATTLE_BTL_FAINTREC_H
