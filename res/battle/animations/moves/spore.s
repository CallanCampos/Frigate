// MOVE_SPORE's animation: member 147 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Spore

Spore:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    ParticleLoad 316
    EmitterAdd 316, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 316, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_DEFENDER, 0, 13, 1, 0x3de
    SetYield FALSE
    PlaySE SEQ_SE_W077_03, 1, EFFVM_DEFENDER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W077_03, 2, EFFVM_DEFENDER, 30, 0, 40, 0, 0, 0
    PlaySE SEQ_SE_W099_01, 1, EFFVM_DEFENDER, 50, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 180, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 2000, -1000, 50, 130, 0, 0
    MoveSE 1, 0, 1, 120, 0, 130, 50, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_DEFENDER, 13, 0, 1, 0x3de
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_DEFENDER, 0, 13, 1, 0x3de
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_DEFENDER, 13, 0, 1, 0x3de
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_DEFENDER, 0, 13, 1, 0x3de
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_DEFENDER, 13, 0, 1, 0x3de
    Wait EFFVM_WAIT_MCSS
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0xcd, -0xcd, 2, 1, 4
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
