// Effect BTLV_EFFECT_FIRST + 21 (0x246): member 21 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect021

Effect021:
    McssCameraMode 0, 0
    SetYield FALSE
    CameraMove 1, 0x3b33, 0x7b33, 0x1b4cd, -0x3000, 0x399a, 0xa000, 16, 0, 0
    McssScale EFFVM_ALL, 1, 0xc00, 0xc00, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    End
