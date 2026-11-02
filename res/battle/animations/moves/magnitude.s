// MOVE_MAGNITUDE's animation: member 222 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Magnitude

Magnitude:
    CameraMovePreset 1, 14, 24, 0, 0
    CameraShake 1, 0x4cd, 4096, 1, 1, 10
    PlaySE SEQ_SE_W157_02, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W157_03, 2, EFFVM_DEFENDER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 110, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 110, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 120, 0, 60, 50, 0, 0
    MoveSE 2, 0, 1, 110, 0, 60, 50, 0, 0
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0x4cd, 1, 2, 12
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
