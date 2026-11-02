// Effect BTLV_EFFECT_FIRST + 52 (0x265): member 52 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect052

Effect052:
    IfVarConst EFFVM_VAR_ATTACKER_VANISH, EFFVM_CMP_EQ, TRUE, Effect052_00F0
    McssMoveXY EFFVM_ATTACKER, 1, -0xc000, 0, 12, 0, 0
    PlaySE SEQ_SE_BE_SE_015, 5, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    McssEffect 1, EFFVM_ATTACKER, 0
    McssMoveXY EFFVM_ATTACKER, 1, 0xc000, 0, 12, 0, 0
    PlaySE SEQ_SE_BE_SE_015, 5, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    Wait EFFVM_WAIT_ALL

Effect052_00F0:
    End
    .balign 4
