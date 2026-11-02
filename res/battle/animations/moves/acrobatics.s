// MOVE_ACROBATICS's animation: member 512 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Acrobatics

Acrobatics:
    CameraMovePreset 1, 9, 16, 0, 12
    Wait EFFVM_WAIT_CAMERA
    SetYield FALSE
    PlaySE SEQ_SE_W048_01, 1, EFFVM_ATTACKER, 0, 200, 127, 0, 0, 0
    MoveSE 1, 0, 0, 200, -1000, 0, 25, 0, 0
    MoveSE 1, 0, 1, 127, 0, 0, 25, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 25, 0, 0, 0, 0, 0
    PlaySE 0, 2, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    McssMoveXY EFFVM_ATTACKER, 2, 0x2000, 0, 2, 0, 4
    WaitFrames 16
    McssSetMode EFFVM_ATTACKER, 3
    McssSetVanish EFFVM_DEFENDER, TRUE
    CameraMovePreset 1, 11, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 15, -400, 127, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 20, 0, 127, 0, 0, 0
    SetYield TRUE
    ParticleLoad 682
    EmitterAdd 682, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 12
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 4, 0, 1
    WaitFrames 20
    CameraMovePreset 1, 8, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_SYS_38, 1, EFFVM_ATTACKER, 15, 0, 127, 0, 0, 0
    SetYield FALSE
    WaitFrames 20
    McssMoveXY EFFVM_ATTACKER, 2, 0x2000, 0, 2, 0, 2
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetMode EFFVM_ATTACKER, 4
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
