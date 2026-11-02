// MOVE_BUBBLE's animation: member 145 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Bubble

Bubble:
    CameraMovePreset 1, 9, 12, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W124_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W124_01, 2, EFFVM_DEFENDER, 50, -300, 60, 0, 0, 0
    MoveSE 1, 0, 1, 120, 0, 60, 30, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 80, 0, 0
    MoveSE 2, 0, 1, 60, 0, 110, 30, 0, 0
    SetYield TRUE
    ParticleLoad 314
    EmitterAdd 314, 0, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    CameraMovePreset 1, 11, 32, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W051_01, 2, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W051_01, 1, EFFVM_DEFENDER, 10, 0, 90, 0, 0, 0
    PlaySE SEQ_SE_W051_01, 2, EFFVM_DEFENDER, 30, 0, 60, 0, 0, 0
    SetYield TRUE
    EmitterAdd 314, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0xcd, -0xcd, 2, 1, 8
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
