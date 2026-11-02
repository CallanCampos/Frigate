// MOVE_OVERHEAT's animation: member 315 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Overheat

Overheat:
    CameraMovePreset 1, 9, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W157_03, 3, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 3, EFFVM_ATTACKER, 70, 0, 0, 0, 0, 0
    MoveSE 3, 0, 0, 0, 3000, 0, 50, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_ATTACKER, 0, 12, 2, 0x1f
    LoadBg 123
    SetVanish 2, 0
    PaletteFade 4, 0, 15, 1, 0
    Wait EFFVM_WAIT_ALL
    BgPalAnim 18, 0
    BgWave 1, 0x119a, 24, 80, 0, 0
    SetVanish 0, 1
    SetVanish 1, 1
    PaletteFade 3, 15, 0, 1, 0
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W011_01, 2, EFFVM_ATTACKER, 10, 0, 120, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssMoveXY EFFVM_ATTACKER, 2, 0x333, 0, 2, 1, 6
    ParticleLoad 490
    EmitterAdd 490, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 490, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 0, 15, 0, 0
    WaitFrames 15
    SetVanish 0, 0
    SetVanish 1, 0
    PaletteFade 4, 13, 0, 1, 0
    McssPalFade EFFVM_ATTACKER, 12, 0, 2, 0x1f
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W018_02, 1, EFFVM_ATTACKER, 10, -400, 110, 0, 0, 0
    PlaySE SEQ_SE_W018_02, 2, EFFVM_DEFENDER, 10, -400, 110, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 490, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0x1000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 490, 2, EFFVM_EMIT_DEFENDER, 8, 0, 0x1000, 0, 0x800, 0x1000, 0x800, 0x1000
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, TRUE
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
