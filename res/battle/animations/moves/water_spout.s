// MOVE_WATER_SPOUT's animation: member 323 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant WaterSpout

WaterSpout:
    CameraMovePreset 1, 9, 16, 0, 12
    WaitFrames 16
    SetYield FALSE
    PlaySE SEQ_SE_W051_01, 1, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W055_01, 2, EFFVM_ATTACKER, 10, 100, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 90, 100, 0, 0, 0, 0
    MoveSE 2, 0, 1, 115, 0, 70, 20, 0, 0
    SetYield TRUE
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 6, 1, 1
    CameraShake 0, 0x333, 0, 1, 1, 6
    ParticleLoad 497
    EmitterAdd 497, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 497, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1800, 0x1000
    WaitFrames 5
    CameraMove 2, 0, 0, 0, 0, 0x3000, 0, 16, 0, 12
    WaitFrames 55
    CameraMovePreset 1, 11, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W323_01, 1, EFFVM_DEFENDER, 20, -60, 120, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    EmitterAdd 497, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 497, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 497, 2, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    CameraShake 0, 0x333, 0, 1, 1, 6
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 1, 4
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
