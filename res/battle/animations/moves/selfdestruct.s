// MOVE_SELFDESTRUCT's animation: member 120 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Selfdestruct

Selfdestruct:
    CameraMovePreset 1, 9, 16, 0, 12
    PaletteFade 2, 0, 16, 1, 0
    Wait EFFVM_WAIT_ALL
    LoadBg 54
    BgMove 2, 0, 8, 1, 1, 24
    BgPalAnim 4, 5
    SetVanish 1, 1
    SetVanish 0, 1
    SetVanish 2, 0
    McssSetVanish EFFVM_ATTACKER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 2, EFFVM_ATTACKER, 10, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_ATTACKER, 20, 200, 90, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_ATTACKER, 30, 300, 95, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_ATTACKER, 40, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_ATTACKER, 50, 300, 90, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_ATTACKER, 60, -100, 95, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_ATTACKER, 70, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_ATTACKER, 70, 0, 127, 0, 0, 0
    SetYield TRUE
    ParticleLoad 289
    EmitterAdd 289, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 289, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 289, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_ATTACKER, 0, 12, 1, 0xc
    McssMoveXY EFFVM_ATTACKER, 2, 0x800, 0, 2, 0, 12
    WaitFrames 24
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 2, 0, 3
    McssPalFade EFFVM_ATTACKER, 12, 0, 1, 0xc
    Wait EFFVM_WAIT_ALL
    PaletteFade 3, 0, 16, 1, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 16, 0, 0, 0
    SetVanish 1, 0
    SetVanish 0, 0
    SetVanish 2, 1
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    PaletteFade 3, 16, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    End
