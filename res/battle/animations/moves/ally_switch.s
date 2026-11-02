// MOVE_ALLY_SWITCH's animation: member 502 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant AllySwitch

AllySwitch:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W015_01, 5, EFFVM_PAN_CENTER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W015_01, 5, EFFVM_PAN_CENTER, 16, 0, 127, 0, 0, 0
    McssMoveXY EFFVM_ATTACKER, 3, 0x800, 0, 4, 0, 2
    WaitFrames 32
    SetYield FALSE
    McssSetValue EFFVM_ATTACKER, 1
    McssAlpha EFFVM_ATTACKER, 1, 8, 82, 0, 0
    PlaySE SEQ_SE_W015_01, 5, EFFVM_PAN_CENTER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W015_01, 5, EFFVM_PAN_CENTER, 12, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W015_01, 5, EFFVM_PAN_CENTER, 24, 0, 127, 0, 0, 0
    McssMoveXY EFFVM_ATTACKER, 3, 0x1000, 0, 3, 0, 3
    WaitFrames 36
    SetYield FALSE
    PlaySE SEQ_SE_W015_01, 5, EFFVM_PAN_CENTER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W015_01, 5, EFFVM_PAN_CENTER, 8, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W015_01, 5, EFFVM_PAN_CENTER, 16, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W015_01, 5, EFFVM_PAN_CENTER, 24, 0, 127, 0, 0, 0
    McssMoveXY EFFVM_ATTACKER, 3, 0x1800, 0, 2, 0, 4
    WaitFrames 32
    SetYield FALSE
    PlaySE SEQ_SE_W015_01, 5, EFFVM_PAN_CENTER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W015_01, 5, EFFVM_PAN_CENTER, 4, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W015_01, 5, EFFVM_PAN_CENTER, 8, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W015_01, 5, EFFVM_PAN_CENTER, 12, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W015_01, 5, EFFVM_PAN_CENTER, 16, 0, 127, 0, 0, 0
    McssMoveXY EFFVM_ATTACKER, 3, 0x2000, 0, 1, 0, 5
    WaitFrames 20
    McssMoveXY EFFVM_ATTACKER, 1, 0, -0x20000, 0, 0, 0
    McssAlpha EFFVM_ATTACKER, 0, 31, 0, 0, 0
    McssSetValue EFFVM_ATTACKER, 0
    CameraMovePreset 1, 8, 10, 0, 8
    Wait EFFVM_WAIT_ALL
    End
