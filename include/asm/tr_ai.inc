// Macros for the trainer AI scripts, written by tools/script/tr_ai_script.py inc

#include "constants/abilities.h"
#include "constants/battle.h"
#include "constants/hold_effects.h"
#include "constants/items.h"
#include "constants/move_effects.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "constants/tr_ai.h"
#include "constants/types.h"

    // In .data, which LLVM pads with zeros: in .text, clang 18 pads `.balign 4, 0` with ARM NOPs
    .data

    .set FALSE, 0
    .set TRUE, 1

    .macro AICommand id
    .2byte \id
    .endm

    // An entry of a table for IfLoadedInTable, which ends with TABLE_END
    .macro TableEntry entry
    .4byte \entry
    .endm

    // An entry of a table for GoToByMoveEffect: the label to jump to, from the table's start
    .macro LabelDistance dst, src
    .4byte \dst - \src
    .endm

    // Jumps if a random number below 256 is less than value
    .macro IfRandomLessThan value, jump
    AICommand 0
    .4byte \value
    .4byte \jump - (. + 4)
    .endm

    .macro IfRandomGreaterThan value, jump
    AICommand 1
    .4byte \value
    .4byte \jump - (. + 4)
    .endm

    .macro IfRandomEqualTo value, jump
    AICommand 2
    .4byte \value
    .4byte \jump - (. + 4)
    .endm

    .macro IfRandomNotEqualTo value, jump
    AICommand 3
    .4byte \value
    .4byte \jump - (. + 4)
    .endm

    // Adds to the move's score, which stays at least 0
    .macro AddToMoveScore value
    AICommand 4
    .4byte \value
    .endm

    .macro IfHPPercentLessThan battler, percent, jump
    AICommand 5
    .4byte \battler
    .4byte \percent
    .4byte \jump - (. + 4)
    .endm

    .macro IfHPPercentGreaterThan battler, percent, jump
    AICommand 6
    .4byte \battler
    .4byte \percent
    .4byte \jump - (. + 4)
    .endm

    .macro IfHPPercentEqualTo battler, percent, jump
    AICommand 7
    .4byte \battler
    .4byte \percent
    .4byte \jump - (. + 4)
    .endm

    .macro IfHPPercentNotEqualTo battler, percent, jump
    AICommand 8
    .4byte \battler
    .4byte \percent
    .4byte \jump - (. + 4)
    .endm

    // Jumps if the battler has any major status condition. Gen 4's takes a mask of conditions
    .macro IfStatus battler, jump
    AICommand 9
    .4byte \battler
    .4byte \jump - (. + 4)
    .endm

    .macro IfNotStatus battler, jump
    AICommand 10
    .4byte \battler
    .4byte \jump - (. + 4)
    .endm

    // CONDITION_*
    .macro IfCondition battler, condition, jump
    AICommand 11
    .4byte \battler
    .4byte \condition
    .4byte \jump - (. + 4)
    .endm

    .macro IfNotCondition battler, condition, jump
    AICommand 12
    .4byte \battler
    .4byte \condition
    .4byte \jump - (. + 4)
    .endm

    .macro IfBadlyPoisoned battler, jump
    AICommand 13
    .4byte \battler
    .4byte \jump - (. + 4)
    .endm

    .macro IfNotBadlyPoisoned battler, jump
    AICommand 14
    .4byte \battler
    .4byte \jump - (. + 4)
    .endm

    .macro IfConditionFlag battler, flag, jump
    AICommand 15
    .4byte \battler
    .4byte \flag
    .4byte \jump - (. + 4)
    .endm

    .macro IfNotConditionFlag battler, flag, jump
    AICommand 16
    .4byte \battler
    .4byte \flag
    .4byte \jump - (. + 4)
    .endm

    // Jumps if the condition is active on the battler's side
    .macro IfSideCondition battler, condition, jump
    AICommand 17
    .4byte \battler
    .4byte \condition
    .4byte \jump - (. + 4)
    .endm

    .macro IfNotSideCondition battler, condition, jump
    AICommand 18
    .4byte \battler
    .4byte \condition
    .4byte \jump - (. + 4)
    .endm

    .macro IfLoadedLessThan value, jump
    AICommand 19
    .4byte \value
    .4byte \jump - (. + 4)
    .endm

    .macro IfLoadedGreaterThan value, jump
    AICommand 20
    .4byte \value
    .4byte \jump - (. + 4)
    .endm

    .macro IfLoadedEqualTo value, jump
    AICommand 21
    .4byte \value
    .4byte \jump - (. + 4)
    .endm

    .macro IfLoadedNotEqualTo value, jump
    AICommand 22
    .4byte \value
    .4byte \jump - (. + 4)
    .endm

    // Jumps if the loaded value has any of the bits
    .macro IfLoadedMask mask, jump
    AICommand 23
    .4byte \mask
    .4byte \jump - (. + 4)
    .endm

    .macro IfLoadedNotMask mask, jump
    AICommand 24
    .4byte \mask
    .4byte \jump - (. + 4)
    .endm

    // Compares the move being scored
    .macro IfMoveEqualTo move, jump
    AICommand 25
    .4byte \move
    .4byte \jump - (. + 4)
    .endm

    .macro IfMoveNotEqualTo move, jump
    AICommand 26
    .4byte \move
    .4byte \jump - (. + 4)
    .endm

    .macro IfLoadedInTable table, jump
    AICommand 27
    .4byte \table - (. + 4)
    .4byte \jump - (. + 4)
    .endm

    .macro IfLoadedNotInTable table, jump
    AICommand 28
    .4byte \table - (. + 4)
    .4byte \jump - (. + 4)
    .endm

    // Jumps if the attacker has a move with power
    .macro IfAttackerHasDamagingMoves jump
    AICommand 29
    .4byte \jump - (. + 4)
    .endm

    .macro IfAttackerHasNoDamagingMoves jump
    AICommand 30
    .4byte \jump - (. + 4)
    .endm

    .macro LoadTurnCount
    AICommand 31
    .endm

    // LOAD_*
    .macro LoadTypeFrom target
    AICommand 32
    .4byte \target
    .endm

    // The base power of the move being scored
    .macro LoadMovePower
    AICommand 33
    .endm

    // Loads AI_MOVE_DEALS_NO_DAMAGE, AI_NOT_HIGHEST_DAMAGE if another of the attacker's moves deals more damage, or AI_MOVE_IS_HIGHEST_DAMAGE
    .macro FlagMoveDamageScore damage
    AICommand 34
    .4byte \damage
    .endm

    .macro LoadBattlerPreviousMove battler
    AICommand 35
    .4byte \battler
    .endm

    // The same as IfLoadedEqualTo, as in Gen 4
    .macro IfTempEqualTo value, jump
    AICommand 36
    .4byte \value
    .4byte \jump - (. + 4)
    .endm

    // The same as IfLoadedNotEqualTo
    .macro IfTempNotEqualTo value, jump
    AICommand 37
    .4byte \value
    .4byte \jump - (. + 4)
    .endm

    // Compares the attacker's speed with the defender's, COMPARE_SPEED_*
    .macro IfSpeedCompareEqualTo compare, jump
    AICommand 38
    .4byte \compare
    .4byte \jump - (. + 4)
    .endm

    // Loads the number of the party's Pokemon that aren't in battle and can battle
    .macro CountAlivePartyBattlers battler
    AICommand 39
    .4byte \battler
    .endm

    .macro LoadCurrentMove
    AICommand 40
    .endm

    .macro LoadCurrentMoveEffect
    AICommand 41
    .endm

    // Loads the ability that the AI knows the battler to have, or its guess
    .macro LoadBattlerAbility battler
    AICommand 42
    .4byte \battler
    .endm

    // Does nothing. Gen 4's calculates the highest type effectiveness
    .macro Dummy2B
    AICommand 43
    .endm

    // TYPE_EFFECTIVENESS_* of the move being scored
    .macro IfMoveEffectivenessEquals effectiveness, jump
    AICommand 44
    .4byte \effectiveness
    .4byte \jump - (. + 4)
    .endm

    // Jumps if one of the party's Pokemon that aren't in battle has no major status condition
    .macro IfPartyMemberNotStatus battler, jump
    AICommand 45
    .4byte \battler
    .4byte \jump - (. + 4)
    .endm

    // Jumps if one of the party's Pokemon that aren't in battle has a major status condition
    .macro IfPartyMemberStatus battler, jump
    AICommand 46
    .4byte \battler
    .4byte \jump - (. + 4)
    .endm

    .macro LoadCurrentWeather
    AICommand 47
    .endm

    .macro IfCurrentMoveEffectEqualTo effect, jump
    AICommand 48
    .4byte \effect
    .4byte \jump - (. + 4)
    .endm

    .macro IfCurrentMoveEffectNotEqualTo effect, jump
    AICommand 49
    .4byte \effect
    .4byte \jump - (. + 4)
    .endm

    // Stages go from 0 to 12, and 6 is neutral
    .macro IfStatStageLessThan battler, stat, stage, jump
    AICommand 50
    .4byte \battler
    .4byte \stat
    .4byte \stage
    .4byte \jump - (. + 4)
    .endm

    .macro IfStatStageGreaterThan battler, stat, stage, jump
    AICommand 51
    .4byte \battler
    .4byte \stat
    .4byte \stage
    .4byte \jump - (. + 4)
    .endm

    .macro IfStatStageEqualTo battler, stat, stage, jump
    AICommand 52
    .4byte \battler
    .4byte \stat
    .4byte \stage
    .4byte \jump - (. + 4)
    .endm

    .macro IfStatStageNotEqualTo battler, stat, stage, jump
    AICommand 53
    .4byte \battler
    .4byte \stat
    .4byte \stage
    .4byte \jump - (. + 4)
    .endm

    // Jumps if the move's damage with USE_MIN_DAMAGE is at least the defender's HP. The first argument is not used
    .macro IfCurrentMoveKills unused, jump
    AICommand 54
    .4byte \unused
    .4byte \jump - (. + 4)
    .endm

    .macro IfCurrentMoveDoesNotKill unused, jump
    AICommand 55
    .4byte \unused
    .4byte \jump - (. + 4)
    .endm

    // For the defender, only the moves it was seen to use
    .macro IfMoveKnown battler, move, jump
    AICommand 56
    .4byte \battler
    .4byte \move
    .4byte \jump - (. + 4)
    .endm

    .macro IfMoveNotKnown battler, move, jump
    AICommand 57
    .4byte \battler
    .4byte \move
    .4byte \jump - (. + 4)
    .endm

    .macro IfMoveEffectKnown battler, effect, jump
    AICommand 58
    .4byte \battler
    .4byte \effect
    .4byte \jump - (. + 4)
    .endm

    .macro IfMoveEffectNotKnown battler, effect, jump
    AICommand 59
    .4byte \battler
    .4byte \effect
    .4byte \jump - (. + 4)
    .endm

    // Does nothing
    .macro Dummy3C
    AICommand 60
    .endm

    // Flees from the battle
    .macro Escape
    AICommand 61
    .endm

    // Does nothing. Gen 4's takes an argument
    .macro Dummy3E
    AICommand 62
    .endm

    // Does nothing
    .macro Dummy3F
    AICommand 63
    .endm

    .macro LoadHeldItem battler
    AICommand 64
    .4byte \battler
    .endm

    // HOLD_EFFECT_*
    .macro LoadHeldItemEffect battler
    AICommand 65
    .4byte \battler
    .endm

    .macro LoadGender battler
    AICommand 66
    .4byte \battler
    .endm

    // Loads TRUE if condition flag 0 is clear, which is when Fake Out works
    .macro LoadIsFirstTurnInBattle battler
    AICommand 67
    .4byte \battler
    .endm

    .macro LoadStockpileCount battler
    AICommand 68
    .4byte \battler
    .endm

    // BTL_STYLE_*
    .macro LoadBattleStyle
    AICommand 69
    .endm

    .macro LoadBattleType
    AICommand 70
    .endm

    // Loads the item that the battler consumed
    .macro LoadRecycleItem battler
    AICommand 71
    .4byte \battler
    .endm

    // Does nothing
    .macro Dummy48
    AICommand 72
    .endm

    // Replaces the loaded move with its power
    .macro LoadPowerOfLoadedMove
    AICommand 73
    .endm

    // Replaces the loaded move with its effect
    .macro LoadEffectOfLoadedMove
    AICommand 74
    .endm

    // Loads how many times in a row the battler used Protect, Detect or Endure
    .macro LoadProtectChain battler
    AICommand 75
    .4byte \battler
    .endm

    .macro GoTo jump
    AICommand 76
    .4byte \jump - (. + 4)
    .endm

    // Ends the script for this move
    .macro End
    AICommand 77
    .endm

    // Compares the attacker's level with the defender's, CHECK_*
    .macro IfLevel compare, jump
    AICommand 78
    .4byte \compare
    .4byte \jump - (. + 4)
    .endm

    .macro IfTargetIsTaunted jump
    AICommand 79
    .4byte \jump - (. + 4)
    .endm

    .macro IfTargetIsNotTaunted jump
    AICommand 80
    .4byte \jump - (. + 4)
    .endm

    .macro IfTargetIsPartner jump
    AICommand 81
    .4byte \jump - (. + 4)
    .endm

    // Loads TRUE if the battler has the type
    .macro FlagBattlerIsType battler, type
    AICommand 82
    .4byte \battler
    .4byte \type
    .endm

    // Loads TRUE if the ability that LoadBattlerAbility would load is this one
    .macro CheckBattlerAbility battler, ability
    AICommand 83
    .4byte \battler
    .4byte \ability
    .endm

    .macro IfActivatedFlashFire battler, jump
    AICommand 84
    .4byte \battler
    .4byte \jump - (. + 4)
    .endm

    .macro IfHeldItemEqualTo battler, item, jump
    AICommand 85
    .4byte \battler
    .4byte \item
    .4byte \jump - (. + 4)
    .endm

    // Gen 4's takes a mask of conditions
    .macro IfFieldCondition condition, jump
    AICommand 86
    .4byte \condition
    .4byte \jump - (. + 4)
    .endm

    // Loads the value of the condition on the battler's side, such as the layers of Spikes
    .macro LoadSideCondition battler, condition
    AICommand 87
    .4byte \battler
    .4byte \condition
    .endm

    // Meant to jump if one of the party's Pokemon that aren't in battle has lost HP, but checks the battler's HP instead
    .macro IfAnyPartyMemberIsWounded battler, jump
    AICommand 88
    .4byte \battler
    .4byte \jump - (. + 4)
    .endm

    // Meant to jump if one of the party's Pokemon that aren't in battle has used PP, but checks the battler's PP instead
    .macro IfAnyPartyMemberUsedPP battler, jump
    AICommand 89
    .4byte \battler
    .4byte \jump - (. + 4)
    .endm

    .macro LoadFlingPower battler
    AICommand 90
    .4byte \battler
    .endm

    .macro LoadCurrentMovePP
    AICommand 91
    .endm

    .macro IfCanUseLastResort battler, jump
    AICommand 92
    .4byte \battler
    .4byte \jump - (. + 4)
    .endm

    // MOVE_CATEGORY_*
    .macro LoadCurrentMoveClass
    AICommand 93
    .endm

    .macro LoadDefenderLastUsedMoveClass
    AICommand 94
    .endm

    // Loads the battler's place in the order of the turn
    .macro LoadBattlerSpeedRank battler
    AICommand 95
    .4byte \battler
    .endm

    // Loads a 16-bit value of the battler. Gen 4's command here loads the turns since the battler switched in
    .macro LoadBattlerUnk60 battler
    AICommand 96
    .4byte \battler
    .endm

    // Jumps if one of the party's Pokemon that aren't in battle deals more damage to the defender than the attacker
    .macro IfPartyMemberDealsMoreDamage damage, jump
    AICommand 97
    .4byte \damage
    .4byte \jump - (. + 4)
    .endm

    .macro IfHasSuperEffectiveMove jump
    AICommand 98
    .4byte \jump - (. + 4)
    .endm

    // Jumps if the battler's previous move deals more damage to the defender than the attacker's moves
    .macro IfBattlerDealsMoreDamage battler, damage, jump
    AICommand 99
    .4byte \battler
    .4byte \damage
    .4byte \jump - (. + 4)
    .endm

    .macro SumPositiveStatStages battler
    AICommand 100
    .4byte \battler
    .endm

    // Loads the battler's stat stage minus the attacker's
    .macro DiffStatStages battler, stat
    AICommand 101
    .4byte \battler
    .4byte \stat
    .endm

    // Does nothing
    .macro Dummy66
    AICommand 102
    .endm

    // Does nothing
    .macro Dummy67
    AICommand 103
    .endm

    // Does nothing
    .macro Dummy68
    AICommand 104
    .endm

    // Like FlagMoveDamageScore, with the moves of the attacker's partners too
    .macro CheckIfHighestDamageWithPartner damage
    AICommand 105
    .4byte \damage
    .endm

    .macro IfBattlerFainted battler, jump
    AICommand 106
    .4byte \battler
    .4byte \jump - (. + 4)
    .endm

    .macro IfBattlerNotFainted battler, jump
    AICommand 107
    .4byte \battler
    .4byte \jump - (. + 4)
    .endm

    // Loads the battler's actual ability
    .macro LoadAbility battler
    AICommand 108
    .4byte \battler
    .endm

    .macro IfBattlerHasSubstitute battler, jump
    AICommand 109
    .4byte \battler
    .4byte \jump - (. + 4)
    .endm

    .macro LoadSpecies battler
    AICommand 110
    .4byte \battler
    .endm

    // Like IfRandomLessThan, with a random number that stays the same for the turn
    .macro IfTurnRandomLessThan value, jump
    AICommand 111
    .4byte \value
    .4byte \jump - (. + 4)
    .endm

    .macro IfTurnRandomGreaterThan value, jump
    AICommand 112
    .4byte \value
    .4byte \jump - (. + 4)
    .endm

    .macro IfTurnRandomEqualTo value, jump
    AICommand 113
    .4byte \value
    .4byte \jump - (. + 4)
    .endm

    .macro IfTurnRandomNotEqualTo value, jump
    AICommand 114
    .4byte \value
    .4byte \jump - (. + 4)
    .endm

    // Jumps to the table's entry for the move's effect, or ends the script for effects above max. Modes other than 0 end the script
    .macro GoToByMoveEffect mode, max, table
    AICommand 115
    .4byte \mode
    .4byte \max
    .4byte \table - (. + 4)
    .endm

    // Jumps if position effect 3 is active at the battler's position
    .macro IfUnk74 battler, jump
    AICommand 116
    .4byte \battler
    .4byte \jump - (. + 4)
    .endm

    // Meant to compare the battler's Attack with its Special Attack, but compares its position with its Special Attack
    .macro IfAttackLessThanSpAttack battler, jump
    AICommand 117
    .4byte \battler
    .4byte \jump - (. + 4)
    .endm

    .macro IfAttackGreaterThanSpAttack battler, jump
    AICommand 118
    .4byte \battler
    .4byte \jump - (. + 4)
    .endm

    .macro IfAttackEqualToSpAttack battler, jump
    AICommand 119
    .4byte \battler
    .4byte \jump - (. + 4)
    .endm
