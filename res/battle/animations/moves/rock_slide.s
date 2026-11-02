// MOVE_ROCK_SLIDE's animation: member 157 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant RockSlide

RockSlide:
    CameraMovePreset 1, 11, 16, 0, 8
    PlaySE SEQ_SE_W157_02, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W157_01, 0, EFFVM_DEFENDER, 0, 0, 70, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 140, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 0, EFFVM_DEFENDER, 140, 0, 0, 0, 0, 0
    MoveSE 0, 0, 1, 70, 0, 100, 40, 0, 0
    MoveSE 1, 0, 1, 120, 0, 100, 40, 0, 0
    McssSetValue EFFVM_ALL, 1
    Wait EFFVM_WAIT_PARTICLES
    ParticleLoad 325
    EmitterAdd 325, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 6
    EmitterAdd 325, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x800, 0x1000
    EmitterAdd 325, 2, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    EmitterAdd 325, 0, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    WaitFrames 6
    EmitterAdd 325, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    EmitterAdd 325, 2, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x99a, 0x1000
    EmitterAdd 325, 0, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x800, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 0, 0, 90, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 20, 0, 90, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 30, 0, 90, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 40, 0, 90, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 50, 0, 90, 0, 0, 0
    SetYield TRUE
    WaitFrames 20
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 1, 6
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetValue EFFVM_ALL, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
