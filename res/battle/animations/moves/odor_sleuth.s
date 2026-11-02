// MOVE_ODOR_SLEUTH's animation: member 316 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant OdorSleuth

OdorSleuth:
    CameraMovePreset 1, 16, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_DEFENDER, 10, 700, 120, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 1, EFFVM_DEFENDER, 20, 700, 60, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssMoveXY EFFVM_DEFENDER, 3, 0x3000, 0, 1, 0, 16
    WaitFrames 40
    CameraMovePreset 1, 15, 24, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W051_01, 2, EFFVM_ATTACKER, 20, 0, 120, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    McssMoveXY EFFVM_ATTACKER, 3, 0x800, 0, 2, 1, 1
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
