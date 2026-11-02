// MOVE_RETALIATE's animation: member 514 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Retaliate

Retaliate:
    SetYield FALSE
    PlaySE SEQ_SE_W307_02, 1, EFFVM_ATTACKER, 0, -500, 80, 0, 0, 0
    PlaySE SEQ_SE_W307_02, 1, EFFVM_ATTACKER, 40, -1000, 40, 0, 0, 0
    SetYield TRUE
    CameraMovePreset 1, 9, 12, 0, 8
    ParticleLoad 684
    Wait EFFVM_WAIT_ALL
    EmitterAdd 684, 0, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W026_01, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssCircle EFFVM_ATTACKER, 0, 0, 0x4000, 0, 0xa000, 0, 0x1000, 0
    EmitterAdd 684, 1, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 16
    CameraMovePreset 1, 11, 10, 0, 6
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 4, 0, 6
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 10, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 20, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 30, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 40, 0, 127, 0, 0, 0
    SetYield TRUE
    WaitFrames 10
    EmitterAdd 684, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    CameraMovePreset 1, 8, 12, 0, 8
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    WaitFrames 20
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
