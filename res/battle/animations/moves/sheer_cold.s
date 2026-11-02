// MOVE_SHEER_COLD's animation: member 329 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant SheerCold

SheerCold:
    McssSetValue EFFVM_ALL, 1
    PaletteFade 4, 0, 13, 0, 0
    SetVanish 0, 1
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W181_01, 1, EFFVM_DEFENDER, 0, 0, 70, 0, 0, 0
    PlaySE SEQ_SE_W181_01, 1, EFFVM_DEFENDER, 30, 30, 100, 0, 0, 0
    PlaySE SEQ_SE_W100_01, 2, EFFVM_DEFENDER, 20, 300, 127, 0, 0, 0
    PlaySE SEQ_SE_W043_01, 2, EFFVM_DEFENDER, 25, 0, 80, 0, 0, 0
    SetYield TRUE
    ParticleLoad 503
    EmitterAdd 503, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0xb33, 0x1000, 0x800, 0x1000
    EmitterAdd 503, 1, EFFVM_EMIT_DEFENDER, 8, 0x800, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    EmitterAdd 503, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0xb33, 0x1000, 0xccd, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 2, 0, 4
    McssPalFade EFFVM_DEFENDER, 0, 13, 0, 0x7fff
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    PaletteFade 4, 13, 0, 0, 0
    McssPalFade EFFVM_DEFENDER, 13, 0, 0, 0x7fff
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetValue EFFVM_ALL, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
