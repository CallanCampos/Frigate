// Effect BTLV_EFFECT_FIRST + 31 (0x250): member 31 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect031

Effect031:
    McssCameraMode 0, 0
    IfVarConst EFFVM_VAR_MCSS_SCALED, EFFVM_CMP_EQ, FALSE, Effect031_00A8
    SetYield FALSE
    CameraMovePreset 1, 8, 16, 0, 0
    McssScale EFFVM_ALL, 1, 0x1000, 0x1000, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    WaitFrames 60
    SetVar EFFVM_VAR_MCSS_SCALED, FALSE

Effect031_00A8:
    McssCameraMode 0, 0
    CameraMove 1, 0xab33, 0x8b33, 0x104cd, 0x4000, 0x499a, -0x1000, 100, 0, 90
    Wait EFFVM_WAIT_ALL
    WaitFrames 50
    CameraMove 1, 0x8b33, 0x6333, 0x11ccd, 0x2000, 0x219a, 0x800, 50, 0, 42
    Wait EFFVM_WAIT_ALL
    WaitFrames 50
    CameraMove 1, 0x2b33, 0x6333, 0x11ccd, -0x3000, 0x219a, 0x800, 120, 0, 110
    Wait EFFVM_WAIT_ALL
    WaitFrames 120
    CameraMove 1, 0x6b33, 0x5999, 0x11ccd, 0x1000, 0x1800, 0x800, 150, 0, 100
    Wait EFFVM_WAIT_ALL
    CameraMove 1, 0x7333, 0x54cc, 0x11ccd, 0x1800, 0x1333, 0x800, 100, 0, 0
    Wait EFFVM_WAIT_ALL
    WaitFrames 50
    CameraMovePreset 1, 13, 100, 0, 90
    Wait EFFVM_WAIT_ALL
    End
