// MOVE_COUNTER's animation: member 68 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Counter

Counter:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssCircle EFFVM_ATTACKER, 2, 0, 0x1000, 0x1000, 0xa000, 0x1000, 0x1000, 0
    SetYield FALSE
    PlaySE SEQ_SE_W015_01, 1, EFFVM_ATTACKER, 0, -1000, 120, 0, 0, 0
    SetYield TRUE
    WaitFrames 30
    CameraMovePreset 1, 11, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W011_01, 1, EFFVM_DEFENDER, 15, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 15, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 233
    EmitterAdd 233, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 233, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 233, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 233, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 3
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 2, 1, 6
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
