// MOVE_HEAL_BLOCK's animation: member 377 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant HealBlock

HealBlock:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W377_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W377_02, 2, EFFVM_DEFENDER, 45, 100, 90, 0, 0, 0
    SetYield TRUE
    ParticleLoad 556
    EmitterAdd 556, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    PaletteFade 4, 0, 12, 0, 15855
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssPalFade EFFVM_DEFENDER, 0, 10, 0, 0
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 10, 0, 0, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    PaletteFade 4, 12, 0, 0, 15855
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
