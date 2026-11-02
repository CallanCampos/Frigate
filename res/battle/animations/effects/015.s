// Effect BTLV_EFFECT_FIRST + 15 (0x240): member 15 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect015

Effect015:
    SetYield FALSE
    CameraMove 1, 0x2b33, 0x5333, 0x114cd, -0x4000, 0x119a, 0, 7, 0, 0
    McssScale EFFVM_ALL, 1, 0x1000, 0x1000, 7, 0, 0
    McssCameraMode 0, 0
    Wait EFFVM_WAIT_ALL
    CameraSave
    End
    .balign 4
