// MOVE_DIVE's animation: member 291 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant Dive
    ScriptVariant Dive_Variant1

Dive:
    CameraMovePreset 1, 9, 16, 0, 12
    WaitFrames 16
    SetYield FALSE
    PlaySE SEQ_SE_FLD_83, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    McssScale EFFVM_ATTACKER, 2, -0x666, 0x666, 8, 0, 1
    WaitFrames 4
    ParticleLoad 461
    McssMoveXY EFFVM_ATTACKER, 1, 0, -0x14000, 30, 1, 1
    EmitterAdd 461, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0xccd, 0x1000, 0x1000
    WaitFrames 8
    EmitterAdd 461, 2, EFFVM_EMIT_ATTACKER, 8, -0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 461, 2, EFFVM_EMIT_ATTACKER, 8, -0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 461, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 461, 0, EFFVM_EMIT_ATTACKER, 8, 0, 2048, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 461, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    McssSetMode EFFVM_ATTACKER, 3
    McssMoveXY EFFVM_ATTACKER, 5, 0, 0, 1, 0, 0
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End

Dive_Variant1:
    CameraMovePreset 1, 11, 12, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    McssMoveXY EFFVM_ATTACKER, 1, 0, -0x14000, 0, 0, 0
    PlaySE SEQ_SE_FLD_37, 1, EFFVM_DEFENDER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W055_01, 2, EFFVM_DEFENDER, 0, 0, 100, 0, 0, 0
    MoveSE 2, 0, 1, 100, 0, 50, 10, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 70, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 115, 0, 30, 30, 0, 0
    MoveSE 2, 0, 1, 100, 0, 30, 30, 0, 0
    SetYield TRUE
    ParticleLoad 462
    EmitterAddOrtho 462, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0x1800, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 462, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0x1800, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 20
    CameraMovePreset 1, 9, 16, 0, 8
    McssMoveXY EFFVM_ATTACKER, 1, 0, 0x14000, 30, 1, 0
    McssSetMode EFFVM_ATTACKER, 4
    Wait EFFVM_WAIT_ALL
    WaitFrames 20
    CameraMovePreset 1, 8, 10, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
