// MOVE_LEAF_BLADE's animation: member 348 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant LeafBlade

LeafBlade:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W010_01, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W010_01, 1, EFFVM_DEFENDER, 22, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W010_01, 2, EFFVM_DEFENDER, 40, -200, 80, 0, 0, 0
    PlaySE SEQ_SE_W017_01, 1, EFFVM_DEFENDER, 40, 200, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 525
    EmitterAdd 525, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 525, 1, EFFVM_EMIT_DEFENDER, 8, 0x3800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 525, 2, EFFVM_EMIT_DEFENDER, 8, 0x4000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 525, 3, EFFVM_EMIT_DEFENDER, 8, 0x4000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 525, 4, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 6, 0, 1
    WaitFrames 20
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 6, 0, 1
    WaitFrames 20
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 6, 0, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
