#ifndef POKEBW2_BATTLE_BTL_TABLES_H
#define POKEBW2_BATTLE_BTL_TABLES_H

// Overlay 169's btl_tables.c (named descriptively: the lists of moves, abilities and items that the battle handlers
// special-case, and the predicates that search them). Names from swan (https://github.com/ds-pokemon-hacking/swan,
// GPL-3.0) where it has them: IsAbilityMultitype and IsArceus

#include "types.h"

// Moves
BOOL IsEncoreExcludedMove(u16 move); // Encore can't repeat it
BOOL IsMeFirstExcludedMove(u16 move); // Me First can't use it
BOOL IsDelayedAttackMove(u16 move); // Future Sight, Doom Desire (and no move)
BOOL IsPressureFieldMove(u16 move); // Pressure also takes extra PP for it
BOOL IsPledgeMove(u16 move); // The Pledge moves
BOOL func_ov169_0689cb5c(u16 effect);
BOOL IsSleepTalkExcludedMove(u16 move);         // Sleep Talk can't call it
BOOL IsAssistExcludedMove(u16 move);         // Assist can't call it
BOOL IsCopycatExcludedMove(u16 move);         // Copycat can't call it
BOOL IsMimicExcludedMove(u16 move);         // Mimic can't copy it
const u16 *GetMetronomeExcludedMoves(u32 *count); // The moves Metronome can't call

// Items
BOOL func_ov169_0689ca84(u16 item);
BOOL IsEscapeItem(u16 item); // Fluffy Tail, Poke Doll and Poke Toy
BOOL IsArceusPlate(u16 item); // Arceus plates
BOOL IsGenesectDrive(u16 item); // Genesect drives

// Abilities
BOOL IsRolePlayExcludedAbility(u16 ability);
BOOL func_ov169_0689cac4(u16 ability);
BOOL func_ov169_0689cacc(u16 ability);
BOOL IsSkillSwapExcludedAbility(u16 ability);
BOOL IsAbilityMultitype(u16 ability);
BOOL func_ov169_0689cb38(u16 ability);

BOOL func_ov169_0689cb28(u16 background);
BOOL IsArceus(u16 species);

// The nth condition of the lists that the battle walks through, or 0 past their ends
u32 func_ov169_0689cb6c(u32 index);
u32 func_ov169_0689cb80(u32 index);

#endif // POKEBW2_BATTLE_BTL_TABLES_H
