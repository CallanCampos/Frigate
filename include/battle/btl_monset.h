#ifndef POKEBW2_BATTLE_BTL_MONSET_H
#define POKEBW2_BATTLE_BTL_MONSET_H

// Overlay 169's btl_monset.c, named descriptively: a set of up to 6 battle Pokemon that a move or an effect picks
// (the targets of a move, the mons that were hit, the allies of a mon), with the damage each one took and the order
// of the set by speed. btl_handler.h's "mon sets in overlay 169's format" are these. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0) where it has them

#include "types.h"
#include "struct_decls.h"

#define BTL_MONSET_MAX 6

// What a mon of the set has taken, for BtlMonSet.damageKind
#define BTL_MONSET_NO_DAMAGE 0
#define BTL_MONSET_HP_DAMAGE 1
#define BTL_MONSET_SUBSTITUTE_DAMAGE 2

typedef struct BtlMonSet {
    BattleMon *mons[BTL_MONSET_MAX];
    u16 damage[BTL_MONSET_MAX];
    u16 substituteDamage[BTL_MONSET_MAX];
    u8 damageKind[BTL_MONSET_MAX];
    u16 speed[BTL_MONSET_MAX];
    u8 count;
    u8 countMax;
    u8 getIndex;
    u8 numTargets;
} BtlMonSet;

// Empties the set
void BtlMonSet_Clear(BtlMonSet *set);
// Adds a mon unless it is in the set already or the set is full
void BtlMonSet_Add(BtlMonSet *set, BattleMon *mon);
// Adds a mon and records damage it took, to its substitute if that flag is set
void BtlMonSet_AddDamage(BtlMonSet *set, BattleMon *mon, u16 damage, BOOL substitute);
// Removes a mon from the set
void BtlMonSet_Remove(BtlMonSet *set, BattleMon *mon);
BattleMon *BtlMonSet_Get(BtlMonSet *set, u32 index);
// Restarts BtlMonSet_GetNext
void BtlMonSet_ResetIndex(BtlMonSet *set);
// The next mon of the set, or NULL after the last
BattleMon *BtlMonSet_GetNext(BtlMonSet *set);
// The damage a mon took, HP and substitute together; FALSE when it took none
BOOL BtlMonSet_GetTotalDamage(const BtlMonSet *set, BattleMon *mon, u32 *damage);
// The HP damage a mon took; FALSE when it took none
BOOL BtlMonSet_GetHpDamage(const BtlMonSet *set, BattleMon *mon, u32 *damage);
u32 BtlMonSet_GetCount(BtlMonSet *set);
u32 BtlMonSet_GetCountMax(BtlMonSet *set);
void BtlMonSet_SetNumTargets(BtlMonSet *set, u8 numTargets);
// Whether the move had targets (it was aimed at some mon) and every one of them is gone
BOOL BtlMonSet_IsEmptyAfterTargeting(BtlMonSet *set);
// Copies the mons that haven't fainted into dest and returns how many there are
u32 BtlMonSet_CopyNotFainted(BtlMonSet *set, BtlMonSet *dest);
// Copies the mons that are allies of a mon (it too) into out, and returns how many there are
u32 BtlMonSet_CopyAllies(BtlMonSet *set, BattleMon *mon, BtlMonSet *out);
// Copies the mons that are not allies of a mon into out, and returns how many there are
u32 BtlMonSet_CopyOpponents(BtlMonSet *set, BattleMon *mon, BtlMonSet *out);
// Removes the fainted mons
void BtlMonSet_RemoveFainted(BtlMonSet *set);
// Sorts the set by speed, fastest first, with a coin flip for ties
void SortBySpeed(BtlMonSet *set, BtlServerFlow *flow);

#endif // POKEBW2_BATTLE_BTL_MONSET_H
