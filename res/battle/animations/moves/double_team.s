// MOVE_DOUBLE_TEAM's animation: member 104 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant DoubleTeam

DoubleTeam:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W055_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 10, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W055_01, 1, EFFVM_ATTACKER, 25, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 35, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W055_01, 1, EFFVM_ATTACKER, 50, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 60, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W055_01, 1, EFFVM_ATTACKER, 70, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 80, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W055_01, 1, EFFVM_ATTACKER, 90, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 100, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W055_01, 1, EFFVM_ATTACKER, 110, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 120, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W055_01, 1, EFFVM_ATTACKER, 125, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 200, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 0, 1500, 0, 180, 0, 0
    MoveSE 1, 0, 1, 120, 0, 180, 20, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssMoveXY EFFVM_ATTACKER, 3, 0x800, 0, 2, 1, 2
    Wait EFFVM_WAIT_ALL
    McssMoveXY EFFVM_ATTACKER, 3, 0x1000, 0, 2, 1, 2
    Wait EFFVM_WAIT_ALL
    McssMoveXY EFFVM_ATTACKER, 3, 0x2000, 0, 2, 1, 4
    Wait EFFVM_WAIT_ALL
    McssMoveXY EFFVM_ATTACKER, 3, 0x3000, 0, 1, 0, 16
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
