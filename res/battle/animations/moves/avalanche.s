// MOVE_AVALANCHE's animation: member 419 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Avalanche

Avalanche:
    McssSetValue EFFVM_ALL, 1
    PaletteFade 4, 0, 13, 0, 32767
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 16
    CameraMove 2, 0, 0, 0, 0, 0x4000, 0, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W157_02, 1, EFFVM_DEFENDER, 0, 300, 115, 0, 0, 0
    PlaySE SEQ_SE_W157_01, 2, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 170, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W464_02, 1, EFFVM_DEFENDER, 90, 0, 127, 0, 0, 0
    MoveSE 1, 0, 1, 115, 0, 130, 48, 0, 0
    MoveSE 2, 0, 1, 120, 0, 110, 30, 0, 0
    SetYield TRUE
    ParticleLoad 598
    EmitterAdd 598, 5, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0xccd, 0x1000, 0x1000
    WaitFrames 5
    EmitterAdd 598, 5, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0xccd, 0x1000, 0x1000
    EmitterAdd 598, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 598, 4, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 598, 0, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x99a, 0x1000
    WaitFrames 5
    EmitterAdd 598, 4, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 598, 0, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    CameraMovePreset 1, 11, 32, 0, 28
    WaitFrames 10
    CameraShake 0, 0x800, 819, 2, 0, 9
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 6
    WaitFrames 20
    EmitterAdd 598, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 598, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetValue EFFVM_ALL, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    PaletteFade 4, 13, 0, 0, 32767
    Wait EFFVM_WAIT_ALL
    End
