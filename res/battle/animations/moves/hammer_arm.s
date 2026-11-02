// MOVE_HAMMER_ARM's animation: member 359 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant HammerArm

HammerArm:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_DEFENDER, 0, -400, 110, 0, 0, 0
    PlaySE SEQ_SE_W011_01, 2, EFFVM_DEFENDER, 15, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 15, 0, 100, 0, 0, 0
    SetYield TRUE
    ParticleLoad 537
    EmitterAdd 537, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 537, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 537, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 2, 1, 4
    CameraShake 0, 0x99a, 0, 1, 0, 6
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
