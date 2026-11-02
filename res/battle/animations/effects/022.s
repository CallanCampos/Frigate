// Effect BTLV_EFFECT_FIRST + 22 (0x247): member 22 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect022

Effect022:
    McssCameraMode 0, 0
    SetYield FALSE
    CameraMove 1, 0xab33, 0x7b33, 0x1b4cd, 0x4000, 0x399a, 0xa000, 16, 0, 0
    McssScale EFFVM_ALL, 1, 0xc00, 0xc00, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    End
