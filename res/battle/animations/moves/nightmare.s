// MOVE_NIGHTMARE's animation: member 171 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Nightmare

Nightmare:
    CameraMovePreset 1, 16, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W379_POWER, 0, EFFVM_PAN_CENTER, 0, -200, 60, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 0, EFFVM_PAN_CENTER, 220, 0, 0, 0, 0, 0
    MoveSE 4, 0, 1, 60, 0, 200, 20, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    PaletteFade 4, 0, 16, 2, 0
    LoadBg 72
    SetVanish 2, 0
    Wait EFFVM_WAIT_ALL
    BgMove 1, 0, -48, 48, 2, 0
    BgWave 0, 0x1800, 16, 106, 0, 0
    WaitFrames 4
    PaletteFade 3, 16, 0, 2, 0
    SetVanish 1, 1
    SetVanish 0, 1
    McssSetValue EFFVM_DEFENDER, 1
    McssSetValue EFFVM_ATTACKER, 1
    McssAlpha EFFVM_DEFENDER, 1, 12, 12, 1, 1
    McssPalFade EFFVM_DEFENDER, 0, 8, 1, 0x7fff
    SetYield FALSE
    PlaySE SEQ_SE_W464_01, 1, EFFVM_DEFENDER, 0, 0, 115, 0, 0, 0
    SetYield TRUE
    WaitFrames 10
    McssScale EFFVM_DEFENDER, 2, 0, 0x4cd, 8, 1, 1
    McssPalFade EFFVM_DEFENDER, 8, 0, 1, 0x7fff
    McssAlpha EFFVM_DEFENDER, 1, 31, 12, 1, 1
    WaitFrames 20
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 2, 2, 4
    SetYield FALSE
    PlaySE SEQ_SE_W254_04, 2, EFFVM_DEFENDER, 0, -100, 115, 0, 0, 0
    PlaySE SEQ_SE_W254_04, 2, EFFVM_DEFENDER, 20, -100, 115, 0, 0, 0
    SetYield TRUE
    WaitFrames 50
    PaletteFade 3, 0, 16, 2, 0
    WaitFrames 2
    PaletteFade 4, 16, 0, 2, 0
    SetVanish 1, 0
    SetVanish 0, 0
    McssSetValue EFFVM_DEFENDER, 0
    McssSetValue EFFVM_ATTACKER, 0
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
