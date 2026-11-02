// MOVE_FACADE's animation: member 263 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Facade

Facade:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 6, 1, 3
    ParticleLoad 433
    EmitterAddOrtho 433, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 433, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 433, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    SetYield FALSE
    SetYield FALSE
    PlaySE SEQ_SE_W075_03, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    MoveSE 1, 0, 1, 127, 0, 40, 30, 0, 0
    SetYield TRUE
    SetYield TRUE
    McssPalFade EFFVM_ATTACKER, 0, 8, 2, 0x1f
    WaitFrames 6
    McssPalFade EFFVM_ATTACKER, 8, 0, 2, 0x1f
    WaitFrames 6
    McssPalFade EFFVM_ATTACKER, 0, 8, 2, 0x7c00
    WaitFrames 6
    McssPalFade EFFVM_ATTACKER, 8, 0, 2, 0x7c00
    WaitFrames 6
    McssPalFade EFFVM_ATTACKER, 0, 12, 2, 0x3ff
    WaitFrames 6
    McssPalFade EFFVM_ATTACKER, 8, 0, 2, 0x3ff
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 16, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, TRUE
    EmitterAddOrtho 433, 3, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 3, 1, 1
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 0, 0, 115, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
