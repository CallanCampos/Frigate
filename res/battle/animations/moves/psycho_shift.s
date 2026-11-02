// MOVE_PSYCHO_SHIFT's animation: member 375 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant PsychoShift

PsychoShift:
    CameraMovePreset 1, 14, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W095_01, 1, EFFVM_ATTACKER, 5, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W051_03, 2, EFFVM_ATTACKER, 15, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W051_03, 1, EFFVM_ATTACKER, 25, 0, 70, 0, 0, 0
    PlaySE SEQ_SE_W051_03, 2, EFFVM_ATTACKER, 35, 0, 50, 0, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W095_01, 1, EFFVM_DEFENDER, 40, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W051_03, 2, EFFVM_DEFENDER, 55, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W051_03, 1, EFFVM_DEFENDER, 65, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W051_03, 2, EFFVM_DEFENDER, 75, 0, 90, 0, 0, 0
    PlaySE SEQ_SE_W051_03, 1, EFFVM_DEFENDER, 85, 0, 80, 0, 0, 0
    PlaySE SEQ_SE_W051_03, 2, EFFVM_DEFENDER, 95, 0, 70, 0, 0, 0
    PlaySE SEQ_SE_W051_03, 1, EFFVM_DEFENDER, 105, 0, 60, 0, 0, 0
    PlaySE SEQ_SE_W051_03, 2, EFFVM_DEFENDER, 115, 0, 50, 0, 0, 0
    PlaySE SEQ_SE_W051_03, 1, EFFVM_DEFENDER, 125, 0, 40, 0, 0, 0
    PlaySE SEQ_SE_W051_03, 2, EFFVM_DEFENDER, 135, 0, 30, 0, 0, 0
    SetYield TRUE
    PaletteFade 2, 0, 8, 2, 0
    McssMoveXY EFFVM_ATTACKER, 2, 0x333, 0, 2, 0, 30
    ParticleLoad 554
    EmitterAddOrtho 554, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 10
    EmitterAddOrtho 554, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 20
    EmitterAddOrtho 554, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 2, 0, 30
    WaitFrames 10
    EmitterAddOrtho 554, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 10
    EmitterAddOrtho 554, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 20
    EmitterAddOrtho 554, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 50
    PaletteFade 2, 8, 0, 2, 0
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
