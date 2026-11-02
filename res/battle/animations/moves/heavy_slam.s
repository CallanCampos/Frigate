// MOVE_HEAVY_SLAM's animation: member 484 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant HeavySlam

HeavySlam:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_015, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_ATTACKER, 10, -500, 100, 0, 0, 0
    PlaySE SEQ_SE_FLD_03, 2, EFFVM_ATTACKER, 30, 0, 127, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 31, 15, 0, 0
    SetYield TRUE
    McssCircle EFFVM_ATTACKER, 3, 2, 0, 0x2000, 0x10000, 0, 0x1000, 0
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    ParticleLoad 659
    EmitterAdd 659, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 659, 1, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 659, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    CameraShake 0, 0x4cd, 0, 2, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W075_04, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W307_EXP, 2, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 4, 1, 1
    McssCircle EFFVM_DEFENDER, 3, 2, 0, 0x1000, 0x10000, 0, 0x1000, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
