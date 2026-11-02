// Effect BTLV_EFFECT_FIRST + 87 (0x288): member 87 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect087

Effect087:
    SetYield FALSE
    CameraMove 1, 0x3b33, 0x6b33, 0x12800, -0x3000, 0x299a, 0x1333, 20, 0, 18
    McssCameraMode 0, 0
    Wait EFFVM_WAIT_ALL
    Return
    .balign 4
