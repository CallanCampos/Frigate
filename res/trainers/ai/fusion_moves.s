#include "asm/tr_ai.inc"

// AI flag 5. This game replaces Gen 4's PrioritizeExtremes routine: when the attacker is Reshiram or Zekrom, Fusion
// Bolt and Fusion Flare gain 10 points on the first turn.

FusionMoves_Main:
    LoadSpecies AI_BATTLER_ATTACKER
    IfLoadedEqualTo SPECIES_RESHIRAM, FusionMoves_FirstTurn
    IfLoadedEqualTo SPECIES_ZEKROM, FusionMoves_FirstTurn
    GoTo Terminate

FusionMoves_FirstTurn:
    LoadTurnCount
    IfLoadedNotEqualTo 0, Terminate
    IfMoveEqualTo MOVE_FUSION_BOLT, ScorePlus10
    IfMoveEqualTo MOVE_FUSION_FLARE, ScorePlus10
    GoTo Terminate

ScorePlus10:
    AddToMoveScore 10

Terminate:
    End
    .balign 4, 0
