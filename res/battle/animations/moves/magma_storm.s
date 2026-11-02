// MOVE_MAGMA_STORM's animation: member 463 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant MagmaStorm

MagmaStorm:
    McssSetValue EFFVM_ALL, 1
    SetYield FALSE
    PlaySE SEQ_SE_SDEMO_03, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    LoadBg 189
    SetVanish 2, 0
    PaletteFade 4, 0, 13, 1, 0
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 16, 0, 8
    SetVanish 0, 1
    SetVanish 1, 1
    PaletteFade 3, 13, 0, 1, 0
    BgMove 4, 0, 24, 180, 0, 0
    McssSetValue EFFVM_ATTACKER, 1
    McssSetValue EFFVM_DEFENDER, 1
    SetYield FALSE
    PlaySE SEQ_SE_W307_01, 1, EFFVM_DEFENDER, 20, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 160, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 3000, 0, 140, 0, 0
    MoveSE 1, 0, 1, 100, 0, 120, 40, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 40, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 50, -500, 125, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 60, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 70, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 80, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 90, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 100, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 110, -500, 60, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 120, -500, 40, 0, 0, 0
    SetYield TRUE
    ParticleLoad 643
    OrbitEmitters 643, 1, 3, 0x1800, 0x1800, 0, 60, 0, 3, 0
    OrbitEmitters 643, 2, 3, 0x1800, 0x1800, 0, 60, 0, 3, 0
    OrbitEmitters 643, 0, 3, 0x1800, 0x1800, 0, 60, 0, 3, 0
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 4, 2, 6
    WaitFrames 16
    OrbitEmitters 643, 1, 3, 0x1800, 0x1800, 0, 60, 0, 3, 0
    OrbitEmitters 643, 2, 3, 0x1800, 0x1800, 0, 60, 0, 3, 0
    OrbitEmitters 643, 0, 3, 0x1800, 0x1800, 0x1000, 60, 0, 3, 0
    WaitFrames 16
    OrbitEmitters 643, 1, 3, 0x1800, 0x1800, 0, 60, 0, 3, 0
    OrbitEmitters 643, 2, 3, 0x1800, 0x1800, 0, 60, 0, 3, 0
    OrbitEmitters 643, 0, 3, 0x1800, 0x1800, 0, 60, 0, 3, 0
    McssPalFade EFFVM_DEFENDER, 0, 10, 1, 0x1f
    WaitFrames 16
    OrbitEmitters 643, 1, 3, 0x1800, 0x1800, 0, 60, 0, 3, 0
    OrbitEmitters 643, 2, 3, 0x1800, 0x1800, 0, 60, 0, 3, 0
    OrbitEmitters 643, 0, 3, 0x1800, 0x1800, 0x1000, 60, 0, 3, 0
    McssPalFade EFFVM_DEFENDER, 10, 0, 1, 0x1f
    WaitFrames 16
    McssPalFade EFFVM_DEFENDER, 0, 10, 1, 0x1f
    OrbitEmitters 643, 1, 3, 0x1800, 0x1800, 0, 60, 0, 3, 0
    OrbitEmitters 643, 2, 3, 0x1800, 0x1800, 0, 60, 0, 3, 0
    OrbitEmitters 643, 0, 3, 0x1800, 0x1800, 0x1000, 60, 0, 3, 0
    Wait EFFVM_WAIT_PARTICLES
    PaletteFade 3, 0, 15, 1, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 0
    SetVanish 1, 0
    PaletteFade 2, 15, 0, 1, 0
    McssPalFade EFFVM_DEFENDER, 10, 0, 1, 0x1f
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetValue EFFVM_ATTACKER, 0
    McssSetValue EFFVM_DEFENDER, 0
    CameraMovePreset 1, 8, 16, 0, 8
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 0, 0, 0, 0, 0, 0
    McssSetValue EFFVM_ALL, 0
    Wait EFFVM_WAIT_ALL
    End
