// Effect BTLV_EFFECT_FIRST + 76 (0x27d): member 76 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect076

Effect076:
    IfVarConst EFFVM_VAR_ATTACKER_VANISH, EFFVM_CMP_EQ, TRUE, Effect076_017A
    ParticleLoad 444
    ParticleLoad 445
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    EmitterAddOrtho 444, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x1000, 0, 0x800, 0x1000, 0x800, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_039, 1, EFFVM_ATTACKER, 60, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_040, 2, EFFVM_ATTACKER, 70, 0, 100, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 444, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x800, 0x1000, 0xd55, 0x1000
    EmitterAddOrtho 445, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x800, 0x1000, 0xd55, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL

Effect076_017A:
    End
