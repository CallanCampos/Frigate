// MOVE_FIERY_DANCE's animation: member 552 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant FieryDance

FieryDance:
    SetYield FALSE
    PlaySE SEQ_SE_W245_BUCKL, 0, EFFVM_DEFENDER, 0, -1000, 127, 0, 0, 0
    MoveSE 0, 0, 0, -1000, 1000, 0, 50, 0, 0
    PlaySE SEQ_SE_DUMMY5, 0, EFFVM_DEFENDER, 50, 0, 0, 0, 0, 0
    SetYield TRUE
    CameraMovePreset 1, 11, 16, 0, 8
    McssSetValue EFFVM_ALL, 1
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 0, 8, 2, 31
    McssPalFade EFFVM_DEFENDER, 0, 8, 4, 0x1f
    McssSetVanish EFFVM_DEFENDER, TRUE
    ParticleLoad 725
    EmitterAdd 725, 0, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAdd 725, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 10
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_016, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_131, 2, EFFVM_DEFENDER, 0, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_131, 1, EFFVM_DEFENDER, 80, -500, 127, 0, 0, 0
    SetYield TRUE
    EmitterAdd 725, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 60
    McssScale EFFVM_DEFENDER, 2, 0x19a, -0x19a, 4, 0, 23
    EmitterAdd 725, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 10
    EmitterAdd 725, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 50
    EmitterAdd 725, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAdd 725, 1, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x800, 0x1000, 0x800, 0x1000
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 8, 0, 2, 31
    McssPalFade EFFVM_DEFENDER, 8, 0, 2, 0x1f
    McssScale EFFVM_DEFENDER, 2, 0x19a, -0x19a, 2, 0, 1
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetValue EFFVM_ALL, 0
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
