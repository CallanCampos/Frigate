// MOVE_TOXIC's animation: member 92 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Toxic

Toxic:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W124_01, 2, EFFVM_DEFENDER, 10, -500, 100, 0, 0, 0
    PlaySE SEQ_SE_W124_01, 1, EFFVM_DEFENDER, 40, -300, 50, 0, 0, 0
    MoveSE 2, 0, 1, 100, 0, 30, 40, 0, 0
    MoveSE 1, 0, 1, 50, 0, 30, 40, 0, 0
    SetYield TRUE
    ParticleLoad 265
    EmitterAdd 265, 0, EFFVM_EMIT_DEFENDER, 8, 0x333, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 265, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 265, 2, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    Wait EFFVM_WAIT_PARTICLES
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
