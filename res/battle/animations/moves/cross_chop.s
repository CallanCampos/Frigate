// MOVE_CROSS_CHOP's animation: member 238 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant CrossChop

CrossChop:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W015_01, 1, EFFVM_DEFENDER, 0, 0, 50, 0, 0, 0
    PlaySE SEQ_SE_W015_01, 2, EFFVM_DEFENDER, 5, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W015_01, 1, EFFVM_DEFENDER, 50, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 63, -60, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 409
    EmitterAdd 409, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 409, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 409, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 409, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 50
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 6, 0, 1
    McssSetVanish EFFVM_DEFENDER, TRUE
    EmitterAdd 409, 5, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 409, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
