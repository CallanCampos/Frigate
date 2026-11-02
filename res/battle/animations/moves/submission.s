// MOVE_SUBMISSION's animation: member 66 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Submission

Submission:
    CameraMovePreset 1, 14, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssCircle EFFVM_ATTACKER, 2, 0, 0x1000, 0x1000, 0xa000, 0x1000, 0x6000, 0
    SetYield FALSE
    PlaySE SEQ_SE_W027_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 110, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 2000, 0, 100, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 20, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 30, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 40, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 50, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 60, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 70, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 80, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 231
    McssCircle EFFVM_DEFENDER, 3, 0, 0x1000, 0x1000, 0xa000, 0x1000, 0x6000, 0
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    EmitterAddOrtho 231, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 231, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 231, 2, EFFVM_EMIT_DEFENDER, 8, 0, 0x1000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 8
    EmitterAddOrtho 231, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 16
    EmitterAddOrtho 231, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0x1000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 8
    EmitterAddOrtho 231, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 231, 2, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 8
    EmitterAddOrtho 231, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 8
    EmitterAddOrtho 231, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
