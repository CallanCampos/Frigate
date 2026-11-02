// MOVE_LOW_SWEEP's animation: member 490 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant LowSweep

LowSweep:
    CameraMovePreset 1, 9, 16, 0, 8
    WaitFrames 20
    Wait EFFVM_WAIT_MCSS
    SetYield FALSE
    PlaySE SEQ_SE_W075_06, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    MoveSE 1, 0, 1, 127, 0, 15, 8, 0, 0
    SetYield TRUE
    McssCircle EFFVM_ATTACKER, 2, 1, 0x1000, 0x1000, 0x6000, 0, 0x1000, 0
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 20
    ParticleLoad 189
    EmitterAdd 189, 0, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 189, 1, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x99a, 0x1000
    EmitterAdd 189, 2, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_FLD_04, 2, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 6, 0, 1
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
