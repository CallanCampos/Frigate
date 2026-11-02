// MOVE_TICKLE's animation: member 321 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Tickle

Tickle:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_DECIDE3, 1, EFFVM_ATTACKER, 10, 0, 115, 0, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W321_01, 2, EFFVM_DEFENDER, 45, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 90, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 115, 0, 70, 20, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_ATTACKER, 3, 0x99a, 0, 4, 1, 1
    McssPalFade EFFVM_ATTACKER, 0, 12, 1, 0
    ParticleLoad 495
    EmitterAddOrtho 495, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x3000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 20
    McssPalFade EFFVM_ATTACKER, 12, 0, 1, 0
    WaitFrames 10
    CameraMovePreset 1, 16, 24, 0, 0
    McssRotate EFFVM_DEFENDER, 3, 0xe38d55, 3, 1, 2
    McssMoveXY EFFVM_DEFENDER, 2, 0x1000, 0, 5, 1, 2
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
