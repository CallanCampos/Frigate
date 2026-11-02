// MOVE_FIRE_PLEDGE's animation: member 519 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant FirePledge

FirePledge:
    McssSetValue EFFVM_ALL, 1
    ParticleLoad 689
    CameraMovePreset 1, 11, 10, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W216_01, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_032, 2, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W216_01, 1, EFFVM_DEFENDER, 28, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_032, 2, EFFVM_DEFENDER, 28, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W216_01, 1, EFFVM_DEFENDER, 55, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_032, 2, EFFVM_DEFENDER, 55, 0, 127, 0, 0, 0
    SetYield TRUE
    EmitterAdd 689, 4, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 689, 7, EFFVM_EMIT_DEFENDER, 8, 0x800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 689, 1, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    WaitFrames 15
    EmitterAdd 689, 5, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 689, 8, EFFVM_EMIT_DEFENDER, 8, 0x800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 689, 2, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    WaitFrames 15
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x19a, 2, 1, 6
    McssPalFade EFFVM_DEFENDER, 0, 10, 2, 0x1f
    EmitterAdd 689, 3, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 689, 6, EFFVM_EMIT_DEFENDER, 8, 0x800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 689, 0, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    WaitFrames 15
    EmitterAdd 689, 9, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    EmitterAdd 689, 9, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 10, 0, 2, 0x1f
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetValue EFFVM_ALL, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
