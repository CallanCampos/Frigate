// Effect BTLV_EFFECT_FIRST + 17 (0x242): member 17 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect017

Effect017:
    SetYield FALSE
    CameraMove 1, 0xb33, 0x5333, 0x114cd, -0x6000, 0x119a, 0, 7, 0, 0
    McssScale EFFVM_ALL, 1, 0x1000, 0x1000, 7, 0, 0
    McssCameraMode 0, 0
    Wait EFFVM_WAIT_ALL
    CameraSave
    End
    .balign 4
