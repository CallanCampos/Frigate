// MOVE_WAKE_UP_SLAP's animation: member 358 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant WakeUpSlap

WakeUpSlap:
    McssSetVanish EFFVM_DEFENDER, TRUE
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W003_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W112_02, 2, EFFVM_DEFENDER, 0, 200, 80, 0, 0, 0
    PlaySE SEQ_SE_W003_01, 1, EFFVM_DEFENDER, 30, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W112_02, 2, EFFVM_DEFENDER, 30, 200, 80, 0, 0, 0
    PlaySE SEQ_SE_W003_01, 1, EFFVM_DEFENDER, 60, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W112_02, 2, EFFVM_DEFENDER, 60, 200, 80, 0, 0, 0
    PlaySE SEQ_SE_W003_01, 1, EFFVM_DEFENDER, 85, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W112_02, 2, EFFVM_DEFENDER, 85, 200, 80, 0, 0, 0
    SetYield TRUE
    ParticleLoad 536
    EmitterAdd 536, 0, EFFVM_EMIT_DEFENDER, 8, 0x3333, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 536, 1, EFFVM_EMIT_DEFENDER, 8, 0x3333, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 536, 2, EFFVM_EMIT_DEFENDER, 8, 0x3333, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 536, 3, EFFVM_EMIT_DEFENDER, 8, 0x3333, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 4, 2, 4
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 4, 2, 4
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
