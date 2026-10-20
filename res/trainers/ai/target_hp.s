#include "asm/tr_ai.inc"

// AI flag 4. This game replaces Gen 4's Risky routine, and only its table of effects is left, unused. Status moves
// gain a point half the time when the target has 50% of its HP or less, and another half the time at 25% or less.
// Damaging moves gain a point on the first turn.

TargetHP_Main:
    FlagMoveDamageScore USE_MIN_DAMAGE
    IfLoadedNotEqualTo AI_MOVE_DEALS_NO_DAMAGE, TargetHP_DamagingMove
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 50, Terminate
    IfRandomLessThan 128, TargetHP_StatusMove_Below25Percent
    AddToMoveScore 1

TargetHP_StatusMove_Below25Percent:
    IfHPPercentGreaterThan AI_BATTLER_DEFENDER, 25, Terminate
    IfRandomLessThan 128, Terminate
    AddToMoveScore 1
    GoTo Terminate

TargetHP_DamagingMove:
    LoadTurnCount
    IfLoadedNotEqualTo 0, Terminate
    AddToMoveScore 1

Terminate:
    End

UnusedRiskyEffects:
    TableEntry BATTLE_EFFECT_STATUS_SLEEP
    TableEntry BATTLE_EFFECT_HALVE_DEFENSE
    TableEntry BATTLE_EFFECT_COPY_MOVE
    TableEntry BATTLE_EFFECT_ONE_HIT_KO
    TableEntry BATTLE_EFFECT_HIGH_CRITICAL
    TableEntry BATTLE_EFFECT_STATUS_CONFUSE
    TableEntry BATTLE_EFFECT_CALL_RANDOM_MOVE
    TableEntry BATTLE_EFFECT_RANDOM_DAMAGE_1_TO_150_LEVEL
    TableEntry BATTLE_EFFECT_COUNTER
    TableEntry BATTLE_EFFECT_KO_MON_THAT_DEFEATED_USER
    TableEntry BATTLE_EFFECT_ATK_UP_2_STATUS_CONFUSION
    TableEntry BATTLE_EFFECT_INFATUATE
    TableEntry BATTLE_EFFECT_RANDOM_POWER_MAYBE_HEAL
    TableEntry BATTLE_EFFECT_RAISE_ALL_STATS_HIT
    TableEntry BATTLE_EFFECT_MAX_ATK_LOSE_HALF_MAX_HP
    TableEntry BATTLE_EFFECT_MIRROR_COAT
    TableEntry BATTLE_EFFECT_HIT_LAST_WHIFF_IF_HIT
    TableEntry BATTLE_EFFECT_DOUBLE_POWER_IF_HIT
    TableEntry BATTLE_EFFECT_CONFUSE_ALL
    TableEntry BATTLE_EFFECT_POWER_BASED_ON_LOW_SPEED
    TableEntry BATTLE_EFFECT_RANDOM_STAT_UP_2
    TableEntry BATTLE_EFFECT_METAL_BURST
    TableEntry BATTLE_EFFECT_DOUBLE_POWER_IF_MOVING_SECOND
    TableEntry BATTLE_EFFECT_USE_MOVE_FIRST
    TableEntry BATTLE_EFFECT_HIT_FIRST_IF_TARGET_ATTACKING
    TableEntry TABLE_END
    .balign 4, 0
