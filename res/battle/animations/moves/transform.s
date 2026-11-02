// MOVE_TRANSFORM's animation: member 144 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Transform

Transform:
    End
    McssAlpha EFFVM_ATTACKER, 0, 16, 0, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W048_01, 1, EFFVM_ATTACKER, 0, -500, 127, 0, 0, 0
    MoveSE 1, 0, 1, 127, 0, 30, 10, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 40, 0, 0, 0, 0, 0
    SetYield TRUE
    McssMosaic EFFVM_ATTACKER, 1, 8, 8, 1, 0
    Wait EFFVM_WAIT_ALL
    McssSwap
    McssMosaic EFFVM_ATTACKER, 1, 0, 8, 1, 0
    Wait EFFVM_WAIT_ALL
    McssAlpha EFFVM_ATTACKER, 0, 31, 0, 0, 0
    End
