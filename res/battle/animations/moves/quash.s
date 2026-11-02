// MOVE_QUASH's animation: member 511 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Quash

Quash:
    SetYield FALSE
    PlaySE SEQ_SE_W307_03, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W054_01, 2, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    MoveSE 2, 0, 1, 127, 0, 20, 10, 0, 0
    PlaySE SEQ_SE_W020_01, 2, EFFVM_DEFENDER, 50, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W020_01, 1, EFFVM_DEFENDER, 60, 0, 127, 0, 0, 0
    SetYield TRUE
    ParticleLoad 681
    CameraMovePreset 1, 11, 20, 0, 10
    WaitFrames 10
    EmitterAdd 681, 0, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 681, 1, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x666, -0x666, 20, 0, 1
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 10
    Wait EFFVM_WAIT_ALL
    End
