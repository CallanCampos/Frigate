// MOVE_POWER_WHIP's animation: member 438 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant PowerWhip

PowerWhip:
    LoadBg 174
    SetVanish 2, 0
    PaletteFade 4, 0, 13, 2, 0
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    PaletteFade 3, 13, 0, 0, 0
    SetVanish 0, 1
    SetVanish 1, 1
    SetYield FALSE
    PlaySE SEQ_SE_W027_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 40, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 0, 1000, 0, 40, 0, 0
    MoveSE 1, 0, 1, 120, 0, 30, 10, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 30, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 20, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 35, 0, 60, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 50, 0, 20, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 65, 0, 5, 0, 0, 0
    SetYield TRUE
    PaletteFade 3, 0, 0, 0, 0
    BgPalAnim 30, 1
    ParticleLoad 617
    EmitterAdd 617, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 617, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 617, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 1, 4
    Wait EFFVM_WAIT_ALL
    PaletteFade 3, 0, 13, 2, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 0
    SetVanish 1, 0
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    PaletteFade 4, 13, 0, 2, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
