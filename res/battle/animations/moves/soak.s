// MOVE_SOAK's animation: member 487 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Soak

Soak:
    McssSetValue EFFVM_ALL, 1
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W240_01, 1, EFFVM_DEFENDER, 0, -200, 127, 0, 0, 0
    MoveSE 1, 0, 1, 127, 0, 50, 10, 0, 0
    PlaySE SEQ_SE_W077_03, 2, EFFVM_DEFENDER, 40, -500, 127, 0, 0, 0
    SetYield TRUE
    ParticleLoad 662
    EmitterAdd 662, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 662, 0, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x800, 0x1000
    EmitterAdd 662, 1, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 662, 1, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x800, 0x1000
    EmitterAdd 662, 3, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x4cd, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    EmitterAdd 662, 2, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x800, 0x1000
    WaitFrames 10
    EmitterAdd 662, 3, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    WaitFrames 20
    EmitterAdd 662, 3, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x800, 0x1000
    McssPalFade EFFVM_DEFENDER, 0, 8, 2, 0x7d40
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 1, 4
    Wait EFFVM_WAIT_ALL
    McssSetValue EFFVM_ALL, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssPalFade EFFVM_DEFENDER, 8, 0, 2, 0x7d40
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
