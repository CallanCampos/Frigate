// MOVE_SMOG's animation: member 123 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Smog

Smog:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    ParticleLoad 292
    EmitterAdd 292, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    SetYield FALSE
    PlaySE SEQ_SE_W123_01, 1, EFFVM_DEFENDER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W123_02, 2, EFFVM_DEFENDER, 0, 150, 80, 0, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_DEFENDER, 0, 10, 0, 0
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_DEFENDER, 10, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 0, 10, 0, 0
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_DEFENDER, 10, 0, 0, 0
    Wait EFFVM_WAIT_MCSS
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
