// MOVE_BULLDOZE's animation: member 523 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Bulldoze

Bulldoze:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssScale EFFVM_ATTACKER, 2, 0x333, -0x333, 2, 0, 20
    SetYield FALSE
    PlaySE SEQ_SE_W360_MOMOMON, 1, EFFVM_ATTACKER, 0, 0, 70, 0, 0, 0
    PlaySE SEQ_SE_FLD_03, 2, EFFVM_ATTACKER, 35, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_03, 2, EFFVM_ATTACKER, 45, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_03, 2, EFFVM_ATTACKER, 55, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_69, 3, EFFVM_ATTACKER, 0, -200, 60, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 3, EFFVM_ATTACKER, 80, 0, 0, 0, 0, 0
    SetYield TRUE
    WaitFrames 20
    CameraMovePreset 1, 17, 24, 0, 0
    CameraShake 0, 0x4cd, 0, 1, 0, 16
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x19a, -0x19a, 2, 0, 1
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
