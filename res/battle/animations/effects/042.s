// Effect BTLV_EFFECT_FIRST + 42 (0x25b): member 42 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect042

Effect042:
    IfVarConst EFFVM_VAR_ATTACKER_VANISH, EFFVM_CMP_EQ, TRUE, Effect042_018A
    ParticleLoad 149
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_06, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_06, 2, EFFVM_ATTACKER, 15, 0, 117, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_06, 1, EFFVM_ATTACKER, 30, 0, 122, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_06, 2, EFFVM_ATTACKER, 45, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_06, 1, EFFVM_ATTACKER, 60, 0, 117, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 149, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0xc4f, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL

Effect042_018A:
    End
