// MOVE_LAST_RESORT's animation: member 387 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant LastResort

LastResort:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_CAMERA
    SetYield FALSE
    PlaySE SEQ_SE_W112_02, 1, EFFVM_DEFENDER, 0, 0, 110, 0, 0, 0
    MoveSE 1, 0, 0, -4000, 3000, 0, 80, 0, 0
    PlaySE SEQ_SE_W011_01, 2, EFFVM_DEFENDER, 40, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W111_02, 1, EFFVM_DEFENDER, 40, 0, 100, 0, 0, 0
    SetYield TRUE
    ParticleLoad 566
    EmitterAdd 566, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 566, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 566, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 30
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 1, 3
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
