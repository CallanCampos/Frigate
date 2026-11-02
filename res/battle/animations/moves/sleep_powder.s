// MOVE_SLEEP_POWDER's animation: member 79 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant SleepPowder

SleepPowder:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    ParticleLoad 247
    EmitterAdd 247, 0, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 247, 1, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W077_03, 2, EFFVM_DEFENDER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W099_01, 1, EFFVM_DEFENDER, 40, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 130, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 2000, -3000, 40, 90, 0, 0
    MoveSE 1, 0, 1, 0, 100, 35, 20, 0, 0
    MoveSE 1, 0, 1, 100, 0, 90, 40, 0, 0
    SetYield TRUE
    WaitFrames 30
    McssPalFade EFFVM_DEFENDER, 0, 8, 12, 0x53c0
    McssPalFade EFFVM_DEFENDER, 8, 0, 12, 0x53c0
    McssScale EFFVM_DEFENDER, 2, 0x52, -0x52, 1, 2, 6
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
