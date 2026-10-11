#include "asm/tr_ai.inc"

// AI flag 1, EvalAttack: scores damaging moves by their damage. A move that isn't the attacker's strongest loses a
// point, a move that knocks the target out gains 4, and a 4x effective move usually gains 2.

EvalAttack_Main:
    IfTargetIsPartner EvalAttack_Terminate
    IfCurrentMoveKills 0, EvalAttack_ApplyKillBonuses
    FlagMoveDamageScore USE_MIN_DAMAGE
    IfLoadedEqualTo AI_NOT_HIGHEST_DAMAGE, ScoreMinus1
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_HALVE_DEFENSE, EvalAttack_MaybeDeprioritize
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_HIT_LAST_WHIFF_IF_HIT, EvalAttack_MaybeDeprioritize
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_HIT_FIRST_IF_TARGET_ATTACKING, EvalAttack_MaybeDeprioritize
    GoTo EvalAttack_CheckQuadEffective

EvalAttack_MaybeDeprioritize:
    IfRandomLessThan 51, EvalAttack_CheckQuadEffective
    AddToMoveScore -2

EvalAttack_CheckQuadEffective:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_QUADRUPLE, EvalAttack_TryScorePlus2
    End

EvalAttack_TryScorePlus2:
    IfRandomLessThan 80, EvalAttack_Terminate
    AddToMoveScore 2
    End

EvalAttack_ApplyKillBonuses:
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_HALVE_DEFENSE, EvalAttack_Terminate
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_HIT_LAST_WHIFF_IF_HIT, EvalAttack_TryScorePlus4
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_HIT_FIRST_IF_TARGET_ATTACKING, EvalAttack_TryScorePlus4
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_HIT_IN_3_TURNS, EvalAttack_TryScorePlus4
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_PRIORITY_1, EvalAttack_ScorePlus2
    GoTo EvalAttack_ScorePlus4

EvalAttack_TryScorePlus4:
    IfRandomLessThan 170, EvalAttack_Terminate
    GoTo EvalAttack_ScorePlus4

EvalAttack_ScorePlus2:
    AddToMoveScore 2

EvalAttack_ScorePlus4:
    AddToMoveScore 4

EvalAttack_Terminate:
    End

ScoreMinus1:
    AddToMoveScore -1
    End

ScoreMinus2:
    AddToMoveScore -2
    End

ScoreMinus3:
    AddToMoveScore -3
    End

ScoreMinus5:
    AddToMoveScore -5
    End

ScoreMinus6:
    AddToMoveScore -6
    End

ScoreMinus8:
    AddToMoveScore -8
    End

ScoreMinus10:
    AddToMoveScore -10
    End

ScoreMinus12:
    AddToMoveScore -12
    End

ScoreMinus30:
    AddToMoveScore -30
    End

ScorePlus1:
    AddToMoveScore 1
    End

ScorePlus2:
    AddToMoveScore 2
    End

ScorePlus3:
    AddToMoveScore 3
    End

ScorePlus5:
    AddToMoveScore 5
    End

ScorePlus10:
    AddToMoveScore 10
    End
