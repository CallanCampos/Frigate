// MOVE_OUTRAGE's animation: member 200 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Outrage

Outrage:
    CameraMovePreset 1, 14, 24, 0, 0
    PaletteFade 4, 0, 16, 1, 0
    Wait EFFVM_WAIT_ALL
    McssSetValue EFFVM_ALL, 1
    CameraShake 0, 0x333, 0, 2, 0, 13
    SetYield FALSE
    PlaySE SEQ_SE_W307_02, 1, EFFVM_ATTACKER, 20, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_ATTACKER, 50, -400, 110, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_ATTACKER, 55, -800, 127, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_ATTACKER, 60, -300, 110, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_ATTACKER, 65, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W239_01, 2, EFFVM_DEFENDER, 0, -1000, 80, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 110, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 120, 0, 70, 40, 0, 0
    MoveSE 2, 0, 0, -1000, 2000, 0, 100, 0, 0
    SetYield TRUE
    LoadBg 93
    BgPalAnim 12, 12
    BgWave 0, 0x4000, 16, 120, 0, 0
    SetVanish 2, 0
    WaitFrames 6
    PaletteFade 3, 16, 0, 2, 0
    SetVanish 1, 1
    SetVanish 0, 1
    BgMove 1, 320, 0, 120, 0, 0
    ParticleLoad 370
    EmitterAddOrtho 370, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 370, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 370, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssPalFade EFFVM_DEFENDER, 0, 13, 0, 0x1f
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 2, 1, 10
    Wait EFFVM_WAIT_PARTICLES
    PaletteFade 3, 0, 16, 1, 0
    McssPalFade EFFVM_DEFENDER, 13, 0, 0, 0x1f
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 16, 0, 0, 0
    SetVanish 1, 0
    SetVanish 0, 0
    SetVanish 2, 1
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetValue EFFVM_ALL, 0
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
