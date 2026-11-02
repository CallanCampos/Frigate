// Effect BTLV_EFFECT_FIRST + 29 (0x24e): member 29 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect029

Effect029:
    McssCameraMode 0, 0
    IfVarConst EFFVM_VAR_MCSS_SCALED, EFFVM_CMP_EQ, FALSE, Effect029_00A8
    SetYield FALSE
    CameraMovePreset 1, 8, 16, 0, 0
    McssScale EFFVM_ALL, 1, 0x1000, 0x1000, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    WaitFrames 60
    SetVar EFFVM_VAR_MCSS_SCALED, FALSE

Effect029_00A8:
    McssCameraMode 0, 0
    CameraMove 1, 0x7b33, 0x7b33, 0x114cd, 0x1000, 0x499a, 0, 45, 0, 35
    Wait EFFVM_WAIT_ALL
    CameraMove 1, 0x7b33, 0x5333, 0x12ccd, 0x1000, 0x219a, 0x1800, 120, 0, 100
    Wait EFFVM_WAIT_ALL
    WaitFrames 100
    CameraMovePreset 1, 13, 50, 0, 40
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
