// Effect BTLV_EFFECT_FIRST + 43 (0x25c): member 43 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect043

Effect043:
    IfVarConst EFFVM_VAR_ATTACKER_VANISH, EFFVM_CMP_EQ, TRUE, Effect043_0164
    ParticleLoad 150
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_07, 1, EFFVM_ATTACKER, 0, 50, 120, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_07, 2, EFFVM_ATTACKER, 10, 100, 110, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_07, 1, EFFVM_ATTACKER, 20, 50, 100, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_07, 2, EFFVM_ATTACKER, 30, 100, 120, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 150, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL

Effect043_0164:
    End
    .balign 4
