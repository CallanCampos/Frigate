// MOVE_ERUPTION's animation: member 284 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Eruption

Eruption:
    CameraMovePreset 1, 9, 16, 0, 8
    PaletteFade 4, 0, 12, 3, 31
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 6, 2, 2
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 2, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_ATTACKER, 30, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W307_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    MoveSE 1, 0, 2, 0, 127, 90, 25, 0, 0
    MoveSE 1, 0, 0, 0, 500, 70, 25, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 70, 38, 0, 0
    SetYield TRUE
    ParticleLoad 455
    ParticleLoad 455
    EmitterAdd 455, 6, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 455, 5, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    EmitterAdd 455, 6, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 455, 5, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    McssSetValue EFFVM_ALL, 1
    CameraMovePreset 1, 11, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, TRUE
    EmitterAdd 455, 0, EFFVM_EMIT_DEFENDER, 8, 0xb33, 4096, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    SetYield FALSE
    PlaySE SEQ_SE_W306_01, 1, EFFVM_DEFENDER, 0, -500, 100, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 25, -400, 100, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 50, -400, 100, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 75, -400, 100, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 95, -400, 110, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 105, -400, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 120, -400, 127, 0, 0, 0
    SetYield TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 3, 0, 1
    WaitFrames 20
    CameraShake 0, 0x333, 0, 2, 0, 18
    EmitterAdd 455, 1, EFFVM_EMIT_DEFENDER, 8, 0x4cd, 0, 0, 0x1000, 0x1000, 0x99a, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 3, 0, 1
    WaitFrames 20
    EmitterAdd 455, 4, EFFVM_EMIT_DEFENDER, 8, 0xb33, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 3, 0, 1
    WaitFrames 20
    EmitterAdd 455, 0, EFFVM_EMIT_DEFENDER, 8, 0xb33, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 3, 0, 1
    WaitFrames 20
    EmitterAdd 455, 2, EFFVM_EMIT_DEFENDER, 8, 0x4cd, 0, 0, 0x1000, 0x1000, 0x99a, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 3, 0, 1
    WaitFrames 20
    EmitterAdd 455, 3, EFFVM_EMIT_DEFENDER, 8, 0xb33, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 3, 0, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    PaletteFade 4, 12, 0, 3, 31
    McssSetValue EFFVM_ALL, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
