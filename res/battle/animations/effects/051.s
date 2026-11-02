// Effect BTLV_EFFECT_FIRST + 51 (0x264): member 51 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect051

Effect051:
    IfVarConst EFFVM_VAR_ATTACKER_VANISH, EFFVM_CMP_EQ, TRUE, Effect051_00F0
    McssMoveXY EFFVM_ATTACKER, 1, -0xc000, 0, 12, 0, 0
    PlaySE SEQ_SE_BE_SE_015, 5, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    McssEffect 0, EFFVM_ATTACKER, 0
    McssMoveXY EFFVM_ATTACKER, 1, 0xc000, 0, 12, 0, 0
    PlaySE SEQ_SE_BE_SE_015, 5, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    Wait EFFVM_WAIT_ALL

Effect051_00F0:
    End
    .balign 4
