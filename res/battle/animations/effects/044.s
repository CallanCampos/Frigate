// Effect BTLV_EFFECT_FIRST + 44 (0x25d): member 44 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect044

Effect044:
    IfVarConst EFFVM_VAR_ATTACKER_VANISH, EFFVM_CMP_EQ, TRUE, Effect044_0120
    ParticleLoad 151
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_08, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 151, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 151, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL

Effect044_0120:
    End
    .balign 4
