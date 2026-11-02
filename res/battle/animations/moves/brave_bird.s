// MOVE_BRAVE_BIRD's animation: member 413 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant BraveBird

BraveBird:
    CameraMovePreset 1, 9, 10, 0, 6
    LoadBg 159
    SetYield FALSE
    PlaySE SEQ_SE_FLD_57, 2, EFFVM_ATTACKER, 0, 2000, 30, 0, 0, 0
    MoveSE 2, 0, 1, 30, 0, 80, 10, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 90, 0, 0, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 90, 0, 0
    SetYield TRUE
    SetVanish 2, 0
    PaletteFade 4, 0, 13, 0, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 3, 13, 0, 1, 0
    BgMove 4, 36, 0, 120, 0, 0
    SetVanish 0, 1
    SetVanish 1, 1
    McssSetValue EFFVM_ATTACKER, 1
    McssSetValue EFFVM_DEFENDER, 1
    SetYield FALSE
    PlaySE SEQ_SE_W255_BU, 1, EFFVM_DEFENDER, 0, 0, 70, 0, 0, 0
    PlaySE SEQ_SE_W250_UZUSHIO, 1, EFFVM_DEFENDER, 0, 800, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 65, 0, 0, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 30, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W011_01, 1, EFFVM_DEFENDER, 30, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W011_01, 1, EFFVM_DEFENDER, 45, 0, 30, 0, 0, 0
    SetYield TRUE
    ParticleLoad 592
    McssSetMode EFFVM_ATTACKER, 3
    EmitterAdd 592, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 592, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 592, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 592, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 592, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 592, 5, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 592, 7, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 592, 8, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 6
    CameraMovePreset 1, 11, 10, 0, 6
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssPalFade EFFVM_DEFENDER, 0, 12, 2, 0x1e
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 2, 1, 8
    EmitterAdd 592, 6, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_PARTICLES
    PaletteFade 3, 0, 13, 1, 0
    Wait EFFVM_WAIT_PARTICLES
    McssSetMode EFFVM_ATTACKER, 4
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssPalFade EFFVM_DEFENDER, 12, 0, 2, 0x1e
    McssSetValue EFFVM_ATTACKER, 0
    McssSetValue EFFVM_DEFENDER, 0
    PaletteFade 2, 13, 0, 0, 0
    SetVanish 0, 0
    SetVanish 1, 0
    CameraMovePreset 1, 8, 10, 0, 6
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
