#include "asm/tr_ai.inc"

// AI flag 13, CatchTutorial: flees once the player's Pokemon is at 20% of its HP or less.

CatchTutorial_Main:
    IfHPPercentEqualTo AI_BATTLER_DEFENDER, 20, CatchTutorial_Escape
    IfHPPercentLessThan AI_BATTLER_DEFENDER, 20, CatchTutorial_Escape
    End

CatchTutorial_Escape:
    Escape
    End
    .balign 4, 0
