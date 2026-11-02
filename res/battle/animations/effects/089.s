// Effect BTLV_EFFECT_FIRST + 89 (0x28a): member 89 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect089

Effect089:
    SetYield FALSE
    CameraMove 1, 0x9b33, 0x6b33, 0x1019a, 0x3000, 0x299a, -0x1333, 20, 0, 18
    McssCameraMode 0, 0
    Wait EFFVM_WAIT_ALL
    Return
    .balign 4
