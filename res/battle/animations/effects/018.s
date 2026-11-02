// Effect BTLV_EFFECT_FIRST + 18 (0x243): member 18 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect018

Effect018:
    SetYield FALSE
    CameraMove 1, 0x9b33, 0x5333, 0x114cd, 0x3000, 0x119a, 0, 7, 0, 0
    McssScale EFFVM_ALL, 1, 0x1000, 0x1000, 7, 0, 0
    McssCameraMode 0, 0
    Wait EFFVM_WAIT_ALL
    CameraSave
    End
    .balign 4
