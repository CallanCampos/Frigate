// MOVE_FISSURE's animation: member 90 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Fissure

Fissure:
    CameraMovePreset 1, 16, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, TRUE
    PaletteFade 2, 0, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 3, 16, 0, 1, 0
    LoadBg 42
    BgMove 2, 8, 0, 2, 1, 16
    SetVanish 0, 1
    SetVanish 1, 1
    SetVanish 2, 0
    McssCameraMode 0, 0
    PlaySE SEQ_SE_W157_02, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 140, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W157_03, 3, EFFVM_DEFENDER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 3, EFFVM_DEFENDER, 140, 0, 0, 0, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W181_01, 2, EFFVM_DEFENDER, 40, -1000, 120, 0, 0, 0
    PlaySE SEQ_SE_W181_01, 2, EFFVM_DEFENDER, 60, -1000, 120, 0, 0, 0
    PlaySE SEQ_SE_W181_01, 2, EFFVM_DEFENDER, 80, -1000, 120, 0, 0, 0
    SetYield TRUE
    CameraShake 0, 0x333, 0, 1, 1, 16
    WaitFrames 20
    ParticleLoad 261
    ParticleLoad 262
    EmitterAdd 262, 0, EFFVM_EMIT_DEFENDER, 8, -0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    EmitterAdd 261, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 14
    Wait EFFVM_WAIT_ALL
    PaletteFade 3, 0, 16, 1, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 16, 0, 0, 0
    SetVanish 1, 0
    SetVanish 0, 0
    SetVanish 2, 1
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
