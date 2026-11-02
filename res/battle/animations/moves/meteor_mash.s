// MOVE_METEOR_MASH's animation: member 309 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant MeteorMash

MeteorMash:
    PaletteFade 4, 0, 12, 3, 0
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_FLD_16, 1, EFFVM_DEFENDER, 0, 0, 50, 0, 0, 0
    PlaySE SEQ_SE_W234_HIKARI, 2, EFFVM_DEFENDER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 50, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 40, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 480
    EmitterAdd 480, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 480, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 480, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 480, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_DEFENDER, 0, 12, 2, 0x7fff
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 30
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 1, 3
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssPalFade EFFVM_DEFENDER, 12, 0, 2, 0x7fff
    McssSetVanish EFFVM_DEFENDER, FALSE
    PaletteFade 4, 12, 0, 3, 0
    Wait EFFVM_WAIT_ALL
    End
