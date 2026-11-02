// MOVE_STRENGTH's animation: member 70 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Strength

Strength:
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssPalFade EFFVM_ATTACKER, 12, 0, 4, 0x1c
    SetYield FALSE
    PlaySE SEQ_SE_W116_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    MoveSE 1, 0, 0, 0, -2000, 0, 20, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 60, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 70, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 80, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 90, 0, 120, 0, 0, 0
    SetYield TRUE
    McssMoveXY EFFVM_ATTACKER, 2, 0, 0x333, 1, 2, 4
    McssScale EFFVM_ATTACKER, 1, 0xccd, 0xccd, 12, 1, 1
    WaitFrames 44
    McssScale EFFVM_ATTACKER, 1, 0x1000, 0x1000, 2, 1, 1
    McssMoveXY EFFVM_ATTACKER, 2, 0, 0x333, 1, 2, 2
    WaitFrames 4
    CameraMovePreset 1, 11, 16, 0, 12
    ParticleLoad 235
    EmitterAdd 235, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 235, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    McssMoveXY EFFVM_DEFENDER, 3, 0x1000, 0, 1, 2, 3
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
