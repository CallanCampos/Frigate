// MOVE_ENTRAINMENT's animation: member 494 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Entrainment

Entrainment:
    SetYield FALSE
    PlaySE SEQ_SE_FLD_52, 1, EFFVM_ATTACKER, 10, -500, 70, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_06, 2, EFFVM_ATTACKER, 10, -200, 50, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_06, 2, EFFVM_ATTACKER, 22, -200, 50, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_06, 2, EFFVM_ATTACKER, 34, -200, 50, 0, 0, 0
    SetYield TRUE
    CameraMovePreset 1, 9, 16, 0, 8
    McssSetVanish EFFVM_ATTACKER, TRUE
    Wait EFFVM_WAIT_ALL
    McssScale EFFVM_ATTACKER, 2, -0x19a, 0x19a, 15, 0, 2
    WaitFrames 4
    McssRotate EFFVM_ATTACKER, 2, 0xff555600, 15, 0, 1
    WaitFrames 30
    McssRotate EFFVM_ATTACKER, 2, 0xaaaa00, 15, 0, 1
    WaitFrames 4
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    WaitFrames 10
    CameraMovePreset 1, 11, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, TRUE
    Wait EFFVM_WAIT_ALL
    McssScale EFFVM_DEFENDER, 2, -0x19a, 0x19a, 10, 0, 2
    WaitFrames 4
    McssRotate EFFVM_DEFENDER, 2, 0xff555600, 15, 0, 1
    SetYield FALSE
    PlaySE SEQ_SE_FLD_52, 1, EFFVM_DEFENDER, 0, -500, 70, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_06, 2, EFFVM_DEFENDER, 0, -200, 50, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_06, 2, EFFVM_DEFENDER, 12, -200, 50, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_06, 2, EFFVM_DEFENDER, 24, -200, 50, 0, 0, 0
    SetYield TRUE
    WaitFrames 30
    McssRotate EFFVM_DEFENDER, 2, 0xaaaa00, 15, 0, 1
    WaitFrames 10
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
