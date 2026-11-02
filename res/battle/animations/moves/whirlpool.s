// MOVE_WHIRLPOOL's animation: member 250 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Whirlpool

Whirlpool:
    LoadBg 114
    PaletteFade 4, 0, 16, 2, 0
    CameraMovePreset 1, 16, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 3, 16, 0, 3, 0
    SetVanish 2, 0
    SetVanish 0, 1
    SetVanish 1, 1
    Wait EFFVM_WAIT_BG
    BgMove 4, 0, 5, 80, 0, 0
    BgWave 0, 0x1800, 8, 80, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W250_UZUSHIO, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PanSE 2, 0, EFFVM_DEFENDER, EFFVM_DEFENDER, 0, 120, 0, 0
    PlaySE SEQ_SE_W239_01, 2, EFFVM_DEFENDER, 0, -1000, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 120, 0, 127, 0, 0, 0
    MoveSE 2, 0, 0, -1000, 1000, 0, 100, 0, 0
    MoveSE 2, 0, 1, 127, 0, 80, 40, 0, 0
    SetYield TRUE
    ParticleLoad 420
    EmitterAdd 420, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 420, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 420, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 420, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_DEFENDER, 0, 12, 2, 0x514a
    McssCircle EFFVM_DEFENDER, 2, 0, 0x800, 0x800, 0xa000, 0, 0x8000, 0
    WaitFrames 30
    McssPalFade EFFVM_DEFENDER, 12, 0, 2, 0x514a
    PaletteFade 3, 0, 16, 1, 0
    Wait EFFVM_WAIT_PARTICLES
    CameraMovePreset 1, 17, 24, 0, 0
    SetVanish 0, 0
    SetVanish 1, 0
    PaletteFade 4, 16, 0, 2, 0
    Wait EFFVM_WAIT_ALL
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
