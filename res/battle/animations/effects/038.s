// Effect BTLV_EFFECT_FIRST + 38 (0x257): member 38 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect038

Effect038:
    IfVarConst EFFVM_VAR_ATTACKER_VANISH, EFFVM_CMP_EQ, TRUE, Effect038_00F2
    ParticleLoad 148
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_02, 1, EFFVM_ATTACKER, 25, 0, 127, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 148, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL

Effect038_00F2:
    End
