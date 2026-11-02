// Effect BTLV_EFFECT_FIRST + 26 (0x24b): member 26 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect026

Effect026:
    SetVar 55, 1
    SetYield FALSE
    CameraMovePreset 1, 8, 10, 0, 8
    Wait EFFVM_WAIT_ALL
    McssCameraMode 0, 0
    Gauge 0, 2
    SetYield FALSE
    CameraMove 1, 0x8333, 0x6b33, 0x124cd, 0x1000, 0x299a, 0x1000, 120, 0, 100
    McssCameraMode 0, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 0, 16, 2, 0
    McssPalFade EFFVM_ALL, 0, 16, 2, 0
    CameraMove 1, 0xd333, 0x6b33, 0x124cd, 0x6000, 0x299a, 0x1000, 10, 0, 8
    Wait EFFVM_WAIT_CAMERA
    Gauge 0, 2
    CameraMove 1, -0x6ccd, 0x6b33, 0x124cd, -0xe000, 0x299a, 0x1000, 30, 0, 0
    Wait EFFVM_WAIT_ALL
    CameraMove 1, 0x21333, 0x6b33, 0x124cd, 0x1a000, 0x299a, 0x1000, 20, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetMode EFFVM_ENEMY_SIDE, 3
    PaletteFade 2, 16, 0, 2, 0
    McssPalFade EFFVM_ALL, 16, 0, 2, 0
    SetVanish 4, 1
    CameraMove 1, 0x5ca6, 0x5f33, 0x13cc3, -0xe8d, 0x1d9a, 0x27f6, 20, 0, 0
    Wait EFFVM_WAIT_ALL
    Gauge 1, 0
    CameraMove 1, 0x74a6, 0x5f33, 0x14cc3, 0x173, 0x1d9a, 0x37f6, 400, 0, 380
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 0, 16, 2, 0
    McssPalFade EFFVM_ALL, 0, 16, 2, 0
    CameraMove 1, -0xcb5a, 0x5f33, 0x14cc3, -0x13e8d, 0x1d9a, 0x37f6, 30, 0, 0
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 10, 0, 8
    Wait EFFVM_WAIT_ALL
    McssCameraMode 0, 0
    McssSetMode EFFVM_ENEMY_SIDE, 4
    PaletteFade 2, 16, 0, 2, 0
    McssPalFade EFFVM_ALL, 16, 0, 2, 0
    SetVanish 4, 0
    Gauge 1, 1
    Wait EFFVM_WAIT_ALL
    End
