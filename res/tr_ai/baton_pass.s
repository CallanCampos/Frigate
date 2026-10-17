#include "asm/tr_ai.inc"

// AI flag 6, BatonPass: for status moves, when the attacker has other Pokemon left, favors raising its stats and
// protecting itself, and then Baton Pass once its stats are raised. Applies more often when it knows Baton Pass.

BatonPass_Main:
    IfTargetIsPartner BatonPass_Terminate
    CountAlivePartyBattlers AI_BATTLER_ATTACKER
    IfLoadedEqualTo 0, BatonPass_Terminate
    FlagMoveDamageScore USE_MIN_DAMAGE
    IfLoadedNotEqualTo AI_MOVE_DEALS_NO_DAMAGE, BatonPass_Terminate
    IfMoveEffectKnown AI_BATTLER_ATTACKER, BATTLE_EFFECT_PASS_STATS_AND_STATUS, BatonPass_EvalMove
    IfRandomLessThan 80, BatonPass_Terminate

BatonPass_EvalMove:
    IfMoveEqualTo MOVE_SWORDS_DANCE, BatonPass_SetupAtHighHP
    IfMoveEqualTo MOVE_DRAGON_DANCE, BatonPass_SetupAtHighHP
    IfMoveEqualTo MOVE_CALM_MIND, BatonPass_SetupAtHighHP
    IfMoveEqualTo MOVE_NASTY_PLOT, BatonPass_SetupAtHighHP
    IfCurrentMoveEffectEqualTo BATTLE_EFFECT_PROTECT, BatonPass_EvalProtect
    IfMoveEqualTo MOVE_BATON_PASS, BatonPass_EvalBatonPass
    IfRandomLessThan 20, BatonPass_Terminate
    AddToMoveScore 3

BatonPass_SetupAtHighHP:
    LoadTurnCount
    IfLoadedEqualTo 0, ScorePlus5
    IfHPPercentLessThan AI_BATTLER_ATTACKER, 60, ScoreMinus10
    GoTo ScorePlus1

BatonPass_EvalProtect:
    LoadBattlerPreviousMove AI_BATTLER_ATTACKER
    IfLoadedInTable BatonPass_ProtectDetect, ScoreMinus2
    AddToMoveScore 2
    End

BatonPass_ProtectDetect:
    TableEntry MOVE_PROTECT
    TableEntry MOVE_DETECT
    TableEntry TABLE_END

BatonPass_EvalBatonPass:
    LoadTurnCount
    IfLoadedEqualTo 0, ScoreMinus2
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 8, ScorePlus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 7, ScorePlus2
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_ATTACK_STAGE, 6, ScorePlus1
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_SP_ATTACK_STAGE, 8, ScorePlus3
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_SP_ATTACK_STAGE, 7, ScorePlus2
    IfStatStageGreaterThan AI_BATTLER_ATTACKER, BATTLEMON_SP_ATTACK_STAGE, 6, ScorePlus1
    End

BatonPass_Terminate:
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
    .balign 4, 0
