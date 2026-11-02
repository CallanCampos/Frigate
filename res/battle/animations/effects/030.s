// Effect BTLV_EFFECT_FIRST + 30 (0x24f): member 30 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect030

Effect030:
    McssCameraMode 0, 0
    IfVarConst EFFVM_VAR_MCSS_SCALED, EFFVM_CMP_EQ, FALSE, Effect030_00A8
    SetYield FALSE
    CameraMovePreset 1, 8, 16, 0, 0
    McssScale EFFVM_ALL, 1, 0x1000, 0x1000, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    WaitFrames 60
    SetVar EFFVM_VAR_MCSS_SCALED, FALSE

Effect030_00A8:
    McssCameraMode 0, 0
    CameraMove 1, 0x4b33, 0x4333, 0x134cd, -0x2000, 0x19a, 0x2000, 30, 0, 20
    Wait EFFVM_WAIT_ALL
    CameraMove 1, 0xab33, 0x8333, 0x134cd, 0x4000, 0x419a, 0x2000, 80, 0, 70
    Wait EFFVM_WAIT_ALL
    CameraMove 1, 0xbb33, 0x9333, 0x134cd, 0x5000, 0x519a, 0x2000, 150, 0, 140
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 13, 50, 0, 40
    Wait EFFVM_WAIT_ALL
    End
