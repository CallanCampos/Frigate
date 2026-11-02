// MOVE_FLATTER's animation: member 260 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Flatter

Flatter:
    PlaySE SEQ_SE_W227_01, 1, EFFVM_ATTACKER, 0, 0, 80, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 130, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 80, 0, 100, 30, 0, 0
    CameraMovePreset 1, 16, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W260_01, 2, EFFVM_DEFENDER, 30, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W260_01, 2, EFFVM_DEFENDER, 45, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 430
    EmitterAddOrtho 430, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 430, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 20
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 4, 1, 2
    McssMoveXY EFFVM_DEFENDER, 2, 0, 0x800, 4, 1, 2
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
