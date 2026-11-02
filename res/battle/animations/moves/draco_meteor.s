// MOVE_DRACO_METEOR's animation: member 434 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant DracoMeteor

DracoMeteor:
    CameraMovePreset 1, 11, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 613
    EmitterAdd 613, 0, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 613, 1, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 613, 2, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 613, 3, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    PaletteFade 4, 0, 12, 1, 12288
    PlaySE SEQ_SE_W319_01, 1, EFFVM_DEFENDER, 15, 0, 110, 0, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W157_03, 3, EFFVM_DEFENDER, 70, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 3, EFFVM_DEFENDER, 180, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W102_02, 1, EFFVM_DEFENDER, 80, -400, 110, 0, 0, 0
    PlaySE SEQ_SE_W102_02, 1, EFFVM_DEFENDER, 90, -400, 110, 0, 0, 0
    PlaySE SEQ_SE_W102_02, 1, EFFVM_DEFENDER, 100, -400, 110, 0, 0, 0
    PlaySE SEQ_SE_W102_02, 1, EFFVM_DEFENDER, 110, -400, 110, 0, 0, 0
    PlaySE SEQ_SE_W102_02, 1, EFFVM_DEFENDER, 120, -400, 110, 0, 0, 0
    PlaySE SEQ_SE_W102_02, 1, EFFVM_DEFENDER, 130, -400, 110, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 150, -300, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 170, -300, 50, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 190, -300, 10, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 80, 300, 95, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 87, 350, 100, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 96, 320, 105, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 109, 300, 90, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 117, 320, 95, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 130, 310, 85, 0, 0, 0
    SetYield TRUE
    WaitFrames 10
    CameraMove 2, 0, 0, 0, -0x8000, 0x8000, 0, 22, 0, 18
    WaitFrames 50
    CameraMovePreset 1, 11, 32, 0, 28
    WaitFrames 10
    McssScale EFFVM_DEFENDER, 2, 0, -0x333, 6, 0, 6
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 1, 12
    CameraShake 0, 0x4cd, 0, 1, 1, 14
    McssSetVanish EFFVM_DEFENDER, TRUE
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 12, 0, 1, 12288
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
