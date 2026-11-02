// Effect BTLV_EFFECT_FIRST + 70 (0x277): member 70 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect070

Effect070:
    IfVarConst EFFVM_VAR_ATTACKER_VANISH, EFFVM_CMP_EQ, TRUE, Effect070_01B0
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 201
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_031, 1, EFFVM_ATTACKER, 10, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_031, 1, EFFVM_ATTACKER, 48, 0, 127, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 201, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x1000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 6
    McssCircle EFFVM_ATTACKER, 3, 0, 0x99a, 0x333, 0x10000, 0, 0x5000, 0
    McssScale EFFVM_ATTACKER, 2, -0x333, 0x4cd, 8, 0, 5
    WaitFrames 30
    EmitterAddOrtho 201, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x1000, 0, 0x800, 0x1000, 0x800, 0x1000
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL

Effect070_01B0:
    End
    .balign 4
