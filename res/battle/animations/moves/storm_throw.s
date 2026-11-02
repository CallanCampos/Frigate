// MOVE_STORM_THROW's animation: member 480 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant StormThrow

StormThrow:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_031, 1, EFFVM_DEFENDER, 10, 200, 127, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_031, 1, EFFVM_DEFENDER, 30, 200, 127, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 73, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 77, 0, 127, 0, 0, 0
    SetYield TRUE
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 4, 1, 3
    ParticleLoad 655
    McssSetVanish EFFVM_ATTACKER, TRUE
    EmitterAdd 655, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 655, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, -0x666, 0x800, 4, 1, 3
    WaitFrames 45
    EmitterAdd 655, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 655, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 3
    EmitterAdd 655, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 655, 1, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x666, -0x800, 4, 1, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
