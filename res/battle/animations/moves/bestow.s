// MOVE_BESTOW's animation: member 516 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Bestow

Bestow:
    CameraMovePreset 1, 9, 16, 0, 8
    WaitFrames 30
    SetYield FALSE
    PlaySE SEQ_SE_FLD_16, 1, EFFVM_ATTACKER, 0, -300, 127, 0, 0, 0
    MoveSE 1, 0, 1, 127, 0, 40, 30, 0, 0
    MoveSE 1, 0, 0, -300, -500, 20, 40, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 60, 0, 0
    SetYield TRUE
    McssScale EFFVM_ATTACKER, 2, 0x333, -0x333, 6, 0, 1
    WaitFrames 10
    ParticleLoad 686
    EmitterMove 686, 0, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x2e000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 20
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 20
    SetYield FALSE
    PlaySE SEQ_SE_W111_01, 2, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 6, 0, 1
    EmitterAdd 686, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    WaitFrames 20
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
