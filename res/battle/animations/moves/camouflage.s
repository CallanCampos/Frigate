// MOVE_CAMOUFLAGE's animation: member 293 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Camouflage

Camouflage:
    CameraMovePreset 1, 9, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W295_01, 1, EFFVM_ATTACKER, 10, -300, 120, 0, 0, 0
    MoveSE 1, 0, 0, -300, 0, 0, 40, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssSetValue EFFVM_ATTACKER, 1
    McssAlpha EFFVM_ATTACKER, 1, 14, 8, 1, 1
    McssPalFade EFFVM_ATTACKER, 0, 12, 1, 0x7fff
    Wait EFFVM_WAIT_ALL
    McssMoveXY EFFVM_ATTACKER, 3, 0x333, 0, 3, 1, 3
    SetYield FALSE
    PlaySE SEQ_SE_W146_01, 2, EFFVM_ATTACKER, 10, -200, 80, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 70, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 100, 0, 40, 30, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 12, 0, 1, 0x7fff
    McssAlpha EFFVM_ATTACKER, 1, 31, 8, 1, 1
    Wait EFFVM_WAIT_ALL
    McssSetValue EFFVM_ATTACKER, 0
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
