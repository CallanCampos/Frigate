// Effect BTLV_EFFECT_FIRST + 27 (0x24c): member 27 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant Effect027
    ScriptVariant Effect027_Variant1

Effect027:
    McssCameraMode 0, 0
    IfVarConst EFFVM_VAR_MCSS_SCALED, EFFVM_CMP_EQ, FALSE, Effect027_00E0
    SetYield FALSE
    CameraMovePreset 1, 8, 16, 0, 0
    McssScale EFFVM_ALL, 1, 0x1000, 0x1000, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    WaitFrames 60
    SetVar EFFVM_VAR_MCSS_SCALED, FALSE

Effect027_00E0:
    McssCameraMode 0, 0
    CameraMove 1, 0x8b33, 0x6b33, 0x114cd, 0x4000, 0x299a, 0, 40, 0, 30
    WaitFrames 65
    Wait EFFVM_WAIT_ALL
    CameraMove 1, 0xb33, 0x5b33, 0x114cd, -0x4000, 0x199a, 0, 450, 0, 430
    WaitFrames 480
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 13, 50, 0, 40
    Wait EFFVM_WAIT_ALL
    End

Effect027_Variant1:
    CameraMove 2, 0x2000, 0, 0, 0x4000, 0, 0, 40, 0, 30
    WaitFrames 60
    Wait EFFVM_WAIT_ALL
    CameraMove 2, -0x8000, -0x1000, 0, -0x8000, -0x1000, 0, 320, 0, 290
    WaitFrames 60
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
