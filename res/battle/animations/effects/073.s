// Effect BTLV_EFFECT_FIRST + 73 (0x27a): member 73 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect073

Effect073:
    IfVarConst EFFVM_VAR_ATTACKER_VANISH, EFFVM_CMP_EQ, TRUE, Effect073_02BC
    ParticleLoad 297
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 10, -500, 110, 0, 0, 0
    PlaySE SEQ_SE_MSCL_06, 2, EFFVM_ATTACKER, 30, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_FLD_44, 1, EFFVM_ATTACKER, 30, 0, 120, 0, 0, 0
    SetYield TRUE
    IfVarConst EFFVM_VAR_ATTACKER_SIDE, EFFVM_CMP_EQ, 1, Effect073_01BE
    EmitterAddOrtho 297, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 297, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 297, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0x1000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 297, 3, EFFVM_EMIT_ATTACKER, 8, 0, 0x1000, 0, 0x800, 0x1000, 0x800, 0x1000
    Jump Effect073_0276

Effect073_01BE:
    EmitterAddOrtho 297, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x800
    EmitterAddOrtho 297, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x800
    EmitterAddOrtho 297, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0x400, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 297, 3, EFFVM_EMIT_ATTACKER, 8, 0, 0x400, 0, 0x800, 0x1000, 0x800, 0x1000

Effect073_0276:
    WaitFrames 20
    McssScale EFFVM_ATTACKER, 2, -0x333, 0x333, 8, 0, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL

Effect073_02BC:
    End
    .balign 4
