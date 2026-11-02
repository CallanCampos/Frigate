// Effect BTLV_EFFECT_FIRST + 4 (0x235): member 4 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect004

Effect004:
    McssCameraMode 0, 0
    IfVarConst EFFVM_VAR_MCSS_SCALED, EFFVM_CMP_EQ, FALSE, Effect004_00A8
    SetYield FALSE
    CameraMovePreset 1, 8, 16, 0, 0
    McssScale EFFVM_ALL, 1, 0x1000, 0x1000, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    WaitFrames 60
    SetVar EFFVM_VAR_MCSS_SCALED, FALSE

Effect004_00A8:
    McssCameraMode 0, 0
    CameraMove 1, 0xab33, 0x6b33, 0x114cd, 0x4000, 0x299a, 0, 90, 0, 82
    Wait EFFVM_WAIT_ALL
    CameraMove 1, 0xb33, 0x5b33, 0x144cd, -0x1000, 0x199a, 0x3000, 500, 0, 480
    Wait EFFVM_WAIT_ALL
    CameraMove 1, 0xb33, 0x5b33, 0x14800, -0x1000, 0x199a, 0x3333, 30, 0, 20
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 13, 50, 0, 40
    Wait EFFVM_WAIT_ALL
    End
