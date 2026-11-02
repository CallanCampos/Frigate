// MOVE_ROCK_TOMB's animation: member 317 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant RockTomb

RockTomb:
    CameraMovePreset 1, 16, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W157_01, 1, EFFVM_ATTACKER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 100, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W157_03, 3, EFFVM_ATTACKER, 0, 0, 90, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 3, EFFVM_ATTACKER, 100, 0, 0, 0, 0, 0
    SetYield TRUE
    ParticleLoad 491
    EmitterAddOrtho 491, 1, 0, 8, 0, 0, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 20
    EmitterAddOrtho 491, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0x1000, 0, 0x800, 0x1000, 0x800, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_FLD_50, 2, EFFVM_DEFENDER, 0, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_50, 2, EFFVM_DEFENDER, 10, -500, 100, 0, 0, 0
    SetYield TRUE
    CameraShake 0, 0x4cd, 0, 2, 0, 1
    WaitFrames 8
    CameraShake 0, 0x4cd, 0, 2, 0, 1
    WaitFrames 8
    CameraShake 0, 0x4cd, 0, 2, 0, 1
    WaitFrames 8
    CameraShake 0, 0x4cd, 0, 2, 0, 1
    WaitFrames 8
    CameraShake 0, 0x4cd, 0, 2, 0, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
