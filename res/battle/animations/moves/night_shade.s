// MOVE_NIGHT_SHADE's animation: member 101 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant NightShade

NightShade:
    McssSetVanish EFFVM_ATTACKER, TRUE
    CameraMovePreset 1, 14, 24, 0, 0
    PaletteFade 4, 0, 10, 1, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W464_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W464_02, 2, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_ATTACKER, 0, 8, 1, 0x7fff
    McssSetValue EFFVM_ATTACKER, 1
    McssScale EFFVM_ATTACKER, 1, 0x1333, 0x1333, 4, 1, 1
    WaitFrames 22
    McssPalFade EFFVM_DEFENDER, 0, 14, 2, 0
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 1, 2, 5
    Wait EFFVM_WAIT_ALL
    McssSetValue EFFVM_ATTACKER, 0
    McssPalFade EFFVM_ATTACKER, 8, 0, 1, 0x7fff
    McssScale EFFVM_ATTACKER, 1, 0x1000, 0x1000, 4, 1, 1
    McssPalFade EFFVM_DEFENDER, 14, 0, 1, 0
    PaletteFade 4, 10, 0, 1, 0
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
