// MOVE_AERIAL_ACE's animation: member 332 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant AerialAce

AerialAce:
    CameraMovePreset 1, 14, 24, 0, 0
    LoadBg 135
    PaletteFade 4, 0, 15, 1, 0
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 2, EFFVM_DEFENDER, 10, 600, 120, 0, 0, 0
    PlaySE SEQ_SE_W010_01, 2, EFFVM_DEFENDER, 20, 800, 120, 0, 0, 0
    PlaySE SEQ_SE_W010_01, 2, EFFVM_DEFENDER, 40, 300, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 40, 200, 80, 0, 0, 0
    SetYield TRUE
    McssCircle EFFVM_ATTACKER, 2, 0, 0x1000, 0x1000, 0xa000, 0, 0x1000, 0
    McssPalFade EFFVM_DEFENDER, 0, 12, 2, 0
    SetVanish 2, 0
    WaitFrames 6
    SetVanish 0, 1
    SetVanish 1, 1
    ParticleLoad 506
    EmitterAddOrtho 506, 3, EFFVM_EMIT_DEFENDER, 8, 0, 0x3000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 506, 4, EFFVM_EMIT_DEFENDER, 8, 0, 0x3000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 506, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0x3000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 506, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0x3000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 506, 2, EFFVM_EMIT_DEFENDER, 8, 0, 0x3000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 6
    BgPalAnim 20, 2
    PaletteFade 3, 15, 0, 1, 0
    WaitFrames 10
    McssMoveXY EFFVM_DEFENDER, 2, -0x1000, 0, 2, 1, 2
    WaitFrames 16
    PaletteFade 3, 0, 15, 0, 0
    WaitFrames 16
    SetVanish 0, 0
    SetVanish 1, 0
    McssPalFade EFFVM_DEFENDER, 15, 0, 2, 0
    PaletteFade 4, 15, 0, 0, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
