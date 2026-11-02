// MOVE_WATERFALL's animation: member 127 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Waterfall

Waterfall:
    LoadBg 63
    PaletteFade 2, 0, 16, 1, 0
    CameraMovePreset 1, 9, 16, 0, 12
    WaitFrames 24
    PaletteFade 3, 16, 0, 1, 0
    SetVanish 0, 1
    SetVanish 1, 1
    SetVanish 2, 0
    BgMove 4, 0, 8, 140, 1, 0
    BgWave 0, 0x2000, 12, 140, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W055_01, 1, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 30, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 1500, 0, 40, 0, 0
    MoveSE 1, 0, 1, 80, 0, 0, 40, 0, 0
    PlaySE SEQ_SE_W239_01, 0, EFFVM_ATTACKER, 0, 0, 80, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 296
    EmitterAdd 296, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 296, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 296, 2, EFFVM_EMIT_DEFENDER, 8, 0x800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    McssSetVanish EFFVM_DEFENDER, TRUE
    CameraMovePreset 1, 11, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W055_01, 2, EFFVM_DEFENDER, 0, -800, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 80, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 0, EFFVM_DEFENDER, 0, -200, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 0, EFFVM_DEFENDER, 80, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 120, 0, 60, 20, 0, 0
    MoveSE 0, 0, 1, 125, 0, 60, 20, 0, 0
    SetYield TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 1, 2, 6
    McssPalFade EFFVM_DEFENDER, 0, 12, 1, 0x7fff
    WaitFrames 30
    McssPalFade EFFVM_DEFENDER, 12, 0, 1, 0x7fff
    WaitFrames 30
    PaletteFade 3, 0, 16, 1, 0
    WaitFrames 16
    SetVanish 0, 0
    SetVanish 1, 0
    PaletteFade 4, 16, 0, 0, 0
    SetVanish 2, 1
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
