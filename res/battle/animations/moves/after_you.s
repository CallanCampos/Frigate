// MOVE_AFTER_YOU's animation: member 495 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant AfterYou

AfterYou:
    CameraMovePreset 1, 9, 16, 0, 8
    McssSetVanish EFFVM_ATTACKER, TRUE
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_FLD_52, 1, EFFVM_ATTACKER, 0, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_52, 1, EFFVM_ATTACKER, 60, -500, 127, 0, 0, 0
    SetYield TRUE
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 4, 0, 1
    WaitFrames 4
    McssSetValue EFFVM_ATTACKER, 1
    McssMoveXY EFFVM_ATTACKER, 2, 0, 0x3000, 16, 0, 1
    McssRotate EFFVM_ATTACKER, 2, 0x71c6ab, 8, 0, 2
    WaitFrames 30
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 4, 0, 1
    McssSetValue EFFVM_ATTACKER, 0
    Wait EFFVM_WAIT_ALL
    WaitFrames 6
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 4, 0, 1
    WaitFrames 4
    McssSetValue EFFVM_ATTACKER, 1
    McssMoveXY EFFVM_ATTACKER, 2, 0, 0x4000, 20, 0, 1
    McssRotate EFFVM_ATTACKER, 2, 0x71c6ab, 8, 0, 2
    WaitFrames 40
    McssSetValue EFFVM_ATTACKER, 0
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 4, 0, 1
    Wait EFFVM_WAIT_ALL
    WaitFrames 10
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
