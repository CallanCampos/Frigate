// Effect BTLV_EFFECT_FIRST + 75 (0x27c): member 75 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect075

Effect075:
    IfVarConst EFFVM_VAR_ATTACKER_VANISH, EFFVM_CMP_EQ, TRUE, Effect075_02AA
    ParticleLoad 502
    CameraMovePreset 1, 15, 24, 0, 0
    McssSetValue EFFVM_ALL, 1
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_038, 1, EFFVM_ATTACKER, 10, -3000, 127, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_038, 1, EFFVM_ATTACKER, 18, -2500, 127, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_038, 1, EFFVM_ATTACKER, 26, -2000, 127, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_038, 1, EFFVM_ATTACKER, 34, -1500, 127, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_038, 1, EFFVM_ATTACKER, 42, -1000, 127, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_038, 1, EFFVM_ATTACKER, 50, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_038, 1, EFFVM_ATTACKER, 58, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_037, 2, EFFVM_ATTACKER, 0, 0, 20, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 502, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 502, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 502, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x800, 0x1000, 0x800, 0x1000
    McssSetVanish EFFVM_ATTACKER, TRUE
    WaitFrames 30
    McssMoveXY EFFVM_ATTACKER, 2, 0x4cd, 0, 3, 1, 8
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    McssSetValue EFFVM_ALL, 0
    Wait EFFVM_WAIT_ALL

Effect075_02AA:
    End
