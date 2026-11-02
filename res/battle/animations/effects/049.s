// Effect BTLV_EFFECT_FIRST + 49 (0x262): member 49 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect049

Effect049:
    IfVarConst EFFVM_VAR_ATTACKER_VANISH, EFFVM_CMP_EQ, TRUE, Effect049_00F2
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    ParticleLoad 156
    EmitterAddOrtho 156, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x5000, 0, 0x1000, 0x400, 0x800, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_13, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL

Effect049_00F2:
    End
