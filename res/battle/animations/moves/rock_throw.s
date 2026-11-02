// MOVE_ROCK_THROW's animation: member 88 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant RockThrow

RockThrow:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    ParticleLoad 259
    EmitterAdd 259, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 259, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 259, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W157_01, 1, EFFVM_DEFENDER, 0, 0, 125, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 35, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W088_02, 2, EFFVM_DEFENDER, 35, 0, 127, 0, 0, 0
    SetYield TRUE
    WaitFrames 40
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 3, 1, 1
    WaitFrames 10
    McssScale EFFVM_DEFENDER, 2, 0xcd, -0xcd, 2, 1, 4
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
