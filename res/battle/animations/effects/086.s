// Effect BTLV_EFFECT_FIRST + 86 (0x287): member 86 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect086

Effect086:
    SetYield FALSE
    CameraMove 1, 0x6b33, 0x6b33, 0x114cd, 0, 0x299a, 0, 20, 0, 18
    McssCameraMode 0, 0
    Wait EFFVM_WAIT_ALL
    Return
    .balign 4
