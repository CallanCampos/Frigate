// MOVE_ENDEAVOR's animation: member 283 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Endeavor

Endeavor:
    CameraMovePreset 1, 14, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W123_01, 0, EFFVM_ATTACKER, 0, 200, 110, 0, 0, 0
    PlaySE SEQ_SE_W123_01, 0, EFFVM_ATTACKER, 20, 200, 110, 0, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 10, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 25, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 40, 0, 120, 0, 0, 0
    SetYield TRUE
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 4, 1, 3
    ParticleLoad 454
    McssSetVanish EFFVM_DEFENDER, TRUE
    EmitterAddOrtho 454, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 454, 2, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 3, 1, 1
    WaitFrames 12
    EmitterAddOrtho 454, 2, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 3, 1, 1
    WaitFrames 12
    EmitterAddOrtho 454, 2, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 3, 1, 1
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
