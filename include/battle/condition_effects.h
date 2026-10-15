#ifndef POKEBW2_BATTLE_CONDITION_EFFECTS_H
#define POKEBW2_BATTLE_CONDITION_EFFECTS_H

// Overlay 169's condition_effects.c, named descriptively after overlay 167's handler files (a guess): what the
// conditions of a battle Pokemon do at the end of a turn and when they end, the messages for them, and the checks
// of the conditions that keep a Pokemon grounded or in place.
// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0) where it has them

#include "types.h"
#include "struct_decls.h"
#include "battle/btl_pokeparam.h"

// What a condition does at the end of a turn, or when it is cured: the damage or recovery, the countdown and the
// messages. A cured condition gets its own message
void Condition_EndTurnEffect(BattleMon *mon, u32 condition, BattleCondition prev, BOOL cured, BtlServerFlow *flow);
// Sets the message of the damage of a condition (poison, burn, Curse and Nightmare). Always returns 0
BOOL Condition_SetDamageMessage(BattleHandlerString *string, BattleMon *mon, u32 condition);
// Sets the message of a condition that was inflicted, or clears the string when it has none
void Condition_SetAddMessage(u32 condition, BattleCondition value, BattleMon *mon, BattleHandlerString *string);
// Sets the message for the cure of a condition, and returns whether it has one. The caller passes a condition that
// ended, and a count that selects a second message
BOOL Condition_SetCureMessage(u32 condition, BattleCondition cont, BattleMon *mon, u16 count, BattleHandlerString *string);
// Whether the condition of a Pokemon keeps it from being switched out, in the event
void Condition_CheckTrapped(BtlServerFlow *flow, BattleMon *mon);
// Whether the conditions of a Pokemon ground it, or give it a type chart check, in the event
void Condition_CheckFloating(BtlServerFlow *flow, BattleMon *mon);
void Condition_CheckTypeImmunity(BtlServerFlow *flow, BattleMon *target);
// Ingrain stops a Pokemon from being forced out, with its message
void Condition_CheckIngrainForceSwitch(BtlServerFlow *flow, BattleMon *mon);
// Whether Baton Pass passes a condition on
BOOL Condition_IsPassedByBatonPass(u32 condition);

#endif // POKEBW2_BATTLE_CONDITION_EFFECTS_H
