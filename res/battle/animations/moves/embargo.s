// MOVE_EMBARGO's animation: member 373 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Embargo

Embargo:
    CameraMovePreset 1, 11, 16, 0, 12
    ParticleLoad 552
    SetYield FALSE
    PlaySE SEQ_SE_W124_01, 1, EFFVM_DEFENDER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 110, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 110, 0, 80, 30, 0, 0
    PlaySE SEQ_SE_W124_01, 2, EFFVM_DEFENDER, 30, -600, 110, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 110, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 110, 0, 80, 30, 0, 0
    SetYield TRUE
    EmitterAdd 552, 0, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 552, 1, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_DEFENDER, 0, 10, 1, 0
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 80
    McssPalFade EFFVM_DEFENDER, 10, 0, 1, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
