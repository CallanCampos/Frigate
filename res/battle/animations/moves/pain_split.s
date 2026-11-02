// MOVE_PAIN_SPLIT's animation: member 220 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant PainSplit

PainSplit:
    CameraMovePreset 1, 14, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 4, 2, 1
    McssScale EFFVM_ATTACKER, 2, -0x333, 0x333, 4, 2, 1
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_ATTACKER, 5, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_ATTACKER, 10, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_ATTACKER, 15, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_ATTACKER, 20, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_ATTACKER, 25, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_ATTACKER, 30, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_ATTACKER, 35, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_ATTACKER, 40, 0, 120, 0, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 0, 400, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 5, 400, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 10, 400, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 15, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 20, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 25, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 30, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 35, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 40, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 391
    EmitterAddOrtho 391, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 391, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 391, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 391, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssMoveXY EFFVM_ATTACKER, 2, 0x800, 0, 2, 1, 8
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 2, 1, 8
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
