// MOVE_PSYCHO_BOOST's animation: member 354 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant PsychoBoost

PsychoBoost:
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W239_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W295_02, 1, EFFVM_ATTACKER, 130, 0, 115, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 1500, 0, 125, 0, 0
    PlaySE SEQ_SE_W319_01, 2, EFFVM_ATTACKER, 130, 0, 110, 0, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 140, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 155, 0, 60, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 170, 0, 30, 0, 0, 0
    SetYield TRUE
    ParticleLoad 531
    EmitterAdd 531, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 531, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 531, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 531, 5, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 110
    CameraMovePreset 1, 11, 16, 0, 12
    McssPalFade EFFVM_ATTACKER, 0, 12, 4, 0x7fff
    LoadBg 150
    BgPalAnim 24, 2
    SetVanish 2, 0
    PaletteFade 4, 0, 14, 0, 0
    WaitFrames 6
    SetVanish 0, 1
    WaitFrames 6
    SetVanish 1, 1
    PaletteFade 3, 14, 0, 0, 0
    McssPalFade EFFVM_ATTACKER, 12, 0, 1, 0x7fff
    EmitterAdd 531, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 531, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 6
    Wait EFFVM_WAIT_ALL
    SetVanish 1, 0
    SetVanish 0, 0
    SetVanish 2, 1
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 13, 0, 1, 0
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
