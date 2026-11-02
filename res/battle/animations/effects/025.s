// Effect BTLV_EFFECT_FIRST + 25 (0x24a): member 25 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect025

Effect025:
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
    CameraMove 1, 0x3333, 0x6b33, 0x124cd, -0x4000, 0x299a, 0x1000, 10, 0, 8
    Wait EFFVM_WAIT_CAMERA
    Gauge 0, 2
    CameraMove 1, 0x17333, 0x6b33, 0x124cd, 0x10000, 0x299a, 0x1000, 30, 0, 0
    Wait EFFVM_WAIT_ALL
    McssCameraMode 0, 0
    CameraMove 1, -0x10ccd, 0x6b33, 0x124cd, -0x18000, 0x299a, 0x1000, 20, 0, 0
    Wait EFFVM_WAIT_ALL
    McssCameraMode 0, 0
    McssSetMode EFFVM_PLAYER_SIDE, 3
    SetVanish 3, 1
    PaletteFade 2, 16, 0, 2, 0
    McssPalFade EFFVM_ALL, 16, 0, 2, 0
    CameraMove 1, 0x8333, 0x9b33, 0xd4cd, 0x1000, 0x299a, -0x4000, 20, 0, 0
    Wait EFFVM_WAIT_ALL
    Gauge 1, 1
    CameraMove 1, 0x9b33, 0xcb33, 0xe4cd, 0x3000, 0x599a, -0x3000, 400, 0, 380
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 0, 16, 2, 0
    McssPalFade EFFVM_ALL, 0, 16, 2, 0
    CameraMove 1, -0xa4cd, 0xcb33, 0xe4cd, -0x11000, 0x599a, -0x3000, 30, 0, 0
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 10, 0, 8
    Wait EFFVM_WAIT_ALL
    McssCameraMode 0, 0
    McssSetMode EFFVM_PLAYER_SIDE, 4
    SetVanish 3, 0
    PaletteFade 2, 16, 0, 2, 0
    McssPalFade EFFVM_ALL, 16, 0, 2, 0
    Gauge 1, 0
    Wait EFFVM_WAIT_ALL
    End
