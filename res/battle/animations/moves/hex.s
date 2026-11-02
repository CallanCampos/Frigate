// MOVE_HEX's animation: member 506 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Hex

Hex:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssMoveXY EFFVM_ATTACKER, 2, 0x1000, 0, 3, 0, 3
    SetYield FALSE
    PlaySE SEQ_SE_W048_01, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    MoveSE 1, 0, 1, 127, 0, 7, 10, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 20, 0, 127, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 16, 0, 8
    ParticleLoad 677
    EmitterAdd 677, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W028_01, 2, EFFVM_DEFENDER, 20, 0, 20, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 0, -200, 127, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x4cd, 2, 0, 10
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
