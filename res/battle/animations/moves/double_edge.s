// MOVE_DOUBLE_EDGE's animation: member 38 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant DoubleEdge

DoubleEdge:
    CameraMovePreset 1, 20, 24, 0, 0
    McssCircle EFFVM_ATTACKER, 3, 0, 0x1000, 0x1000, 0xa000, 0x1000, 0x4000, 0
    SetYield FALSE
    PlaySE SEQ_SE_W027_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 100, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 3000, 0, 100, 0, 0
    MoveSE 1, 0, 1, 120, 0, 60, 40, 0, 0
    SetYield TRUE
    WaitFrames 20
    CameraMovePreset 1, 11, 16, 0, 8
    PaletteFade 4, 0, 16, 1, 0
    Wait EFFVM_WAIT_ALL
    LoadBg 15
    PaletteFade 3, 16, 0, 1, 0
    SetVanish 0, 1
    SetVanish 1, 1
    McssCircle EFFVM_ATTACKER, 3, 0, 0, 0x8000, 0x6000, 0x1000, 0x1000, 0
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W465_01, 2, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 204
    EmitterAdd 204, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 204, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 204, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 204, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 204, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x1000, 0, 2, 1, 6
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 6
    McssSetValue EFFVM_DEFENDER, 1
    McssPalFade EFFVM_DEFENDER, 0, 10, 0, 0x3ff
    WaitFrames 2
    SetVanish 2, 0
    BgPalAnim 2, 3
    WaitFrames 24
    McssPalFade EFFVM_DEFENDER, 10, 0, 0, 0x3ff
    PaletteFade 3, 0, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 10, 0, 0, 0
    McssSetValue EFFVM_DEFENDER, 0
    SetVanish 0, 0
    SetVanish 1, 0
    SetVanish 2, 1
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    PaletteFade 3, 16, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
