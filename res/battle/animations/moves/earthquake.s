// MOVE_EARTHQUAKE's animation: member 89 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Earthquake

Earthquake:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 0, 0, 90, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 40, 0, 90, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 50, 0, 90, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 70, 0, 90, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 80, 0, 90, 0, 0, 0
    PlaySE SEQ_SE_W157_01, 1, EFFVM_DEFENDER, 0, 0, 125, 0, 0, 0
    PlaySE SEQ_SE_W157_03, 3, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 100, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 110, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 3, EFFVM_DEFENDER, 110, 0, 0, 0, 0, 0
    MoveSE 3, 0, 1, 100, 0, 80, 20, 0, 0
    MoveSE 2, 0, 1, 120, 0, 60, 50, 0, 0
    MoveSE 1, 0, 1, 120, 0, 60, 50, 0, 0
    McssSetVanish EFFVM_DEFENDER, TRUE
    ParticleLoad 260
    EmitterAdd 260, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 260, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    CameraShake 1, 0x333, 0, 2, 0, 12
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 2, 1, 9
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
