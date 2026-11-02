// MOVE_SACRED_FIRE's animation: member 221 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant SacredFire

SacredFire:
    CameraMovePreset 1, 11, 16, 0, 8
    PaletteFade 4, 0, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W221_NOISE, 1, EFFVM_DEFENDER, 0, 200, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 100, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W014_01, 2, EFFVM_DEFENDER, 10, 0, 127, 0, 0, 0
    MoveSE 1, 0, 0, 200, 2000, 0, 100, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 105, -800, 127, 0, 0, 0
    PlaySE SEQ_SE_W307_02, 2, EFFVM_DEFENDER, 95, 500, 127, 0, 0, 0
    SetYield TRUE
    PaletteFade 3, 16, 0, 2, 0
    LoadBg 99
    SetVanish 0, 1
    SetVanish 1, 1
    SetVanish 2, 0
    BgMove 4, 2, 0, 160, 0, 0
    BgWave 0, 0x3000, 16, 160, 0, 0
    ParticleLoad 392
    EmitterAdd 392, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    EmitterAdd 392, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 392, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    EmitterAdd 392, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssPalFade EFFVM_DEFENDER, 0, 13, 0, 0x1f
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 2, 1, 10
    Wait EFFVM_WAIT_PARTICLES
    PaletteFade 3, 0, 16, 2, 0
    McssPalFade EFFVM_DEFENDER, 13, 0, 0, 0x1f
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 16, 0, 0, 0
    SetVanish 0, 0
    SetVanish 1, 0
    SetVanish 2, 1
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
