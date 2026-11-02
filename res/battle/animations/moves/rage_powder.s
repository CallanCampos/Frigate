// MOVE_RAGE_POWDER's animation: member 476 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant RagePowder

RagePowder:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    ParticleLoad 653
    EmitterAdd 653, 0, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 653, 1, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W077_03, 2, EFFVM_DEFENDER, 0, 100, 115, 0, 0, 0
    PlaySE SEQ_SE_W077_03, 1, EFFVM_DEFENDER, 20, 100, 30, 0, 0, 0
    PlaySE SEQ_SE_W051_01, 2, EFFVM_DEFENDER, 40, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W051_01, 1, EFFVM_DEFENDER, 50, 0, 40, 0, 0, 0
    SetYield TRUE
    WaitFrames 30
    McssPalFade EFFVM_DEFENDER, 0, 8, 12, 0x1e
    McssPalFade EFFVM_DEFENDER, 8, 0, 12, 0x1e
    McssScale EFFVM_DEFENDER, 2, 0x52, -0x52, 1, 2, 6
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
