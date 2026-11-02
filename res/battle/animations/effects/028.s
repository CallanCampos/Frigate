// Effect BTLV_EFFECT_FIRST + 28 (0x24d): member 28 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant Effect028
    ScriptVariant Effect028_Variant1

Effect028:
    McssCameraMode 0, 0
    IfVarConst EFFVM_VAR_MCSS_SCALED, EFFVM_CMP_EQ, FALSE, Effect028_00E0
    SetYield FALSE
    CameraMovePreset 1, 8, 16, 0, 0
    McssScale EFFVM_ALL, 1, 0x1000, 0x1000, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    WaitFrames 100
    SetVar EFFVM_VAR_MCSS_SCALED, FALSE

Effect028_00E0:
    McssCameraMode 0, 0
    CameraMove 1, 0x1b33, 0x5333, 0x114cd, -0x5000, 0x119a, 0, 100, 0, 90
    Wait EFFVM_WAIT_ALL
    CameraMove 1, 0x8b33, 0x4333, 0x114cd, 0x2000, 0x19a, 0, 400, 0, 380
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 13, 50, 0, 40
    Wait EFFVM_WAIT_ALL
    End

Effect028_Variant1:
    CameraMove 2, -0x5000, -0x1800, 0, -0x5000, -0x1800, 0, 100, 0, 90
    WaitFrames 60
    Wait EFFVM_WAIT_ALL
    CameraMove 2, 0x7000, -0x1000, 0, 0x7000, -0x1000, 0, 400, 0, 380
    WaitFrames 60
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
