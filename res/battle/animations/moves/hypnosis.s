// MOVE_HYPNOSIS's animation: member 95 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Hypnosis

Hypnosis:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    ParticleLoad 268
    EmitterMove 268, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    SetYield FALSE
    PlaySE SEQ_SE_W054_01, 1, EFFVM_DEFENDER, 0, 500, 120, 0, 0, 0
    PlaySE SEQ_SE_W048_01, 2, EFFVM_DEFENDER, 0, 0, 20, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 30, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 30, 0, 0
    SetYield TRUE
    WaitFrames 30
    McssSetVanish EFFVM_DEFENDER, TRUE
    CameraMovePreset 1, 11, 16, 0, 8
    McssScale EFFVM_DEFENDER, 2, 0x52, -0x52, 1, 1, 6
    EmitterAdd 268, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 268, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 268, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W095_01, 2, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W095_02, 1, EFFVM_DEFENDER, 10, -600, 110, 0, 0, 0
    MoveSE 1, 0, 0, 0, -500, 5, 20, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
