// MOVE_THRASH's animation: member 37 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Thrash

Thrash:
    CameraMovePreset 1, 14, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssCircle EFFVM_ATTACKER, 2, 0, 0x1000, 0x1000, 0x10000, 0x1000, 0x3000, 0
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 15, 30, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 203
    EmitterAddOrtho 203, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 203, 2, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    WaitFrames 20
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 0, -600, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 10, 0, 120, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 203, 0, EFFVM_EMIT_DEFENDER, 8, 0x800, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 203, 1, EFFVM_EMIT_DEFENDER, 8, 0x800, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    WaitFrames 20
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 0, 200, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 30, -100, 120, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 203, 0, EFFVM_EMIT_DEFENDER, 8, -0x800, 0x1000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 203, 2, EFFVM_EMIT_DEFENDER, 8, -0x800, 0x1000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    WaitFrames 20
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 0, -1000, 120, 0, 0, 0
    PanSE 1, 0, EFFVM_DEFENDER, EFFVM_DEFENDER, 0, 40, 0, 0
    SetYield TRUE
    EmitterAddOrtho 203, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0x3000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 203, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0x3000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    McssSetVanish EFFVM_DEFENDER, TRUE
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
