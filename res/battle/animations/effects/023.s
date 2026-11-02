// Effect BTLV_EFFECT_FIRST + 23 (0x248): member 23 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect023

Effect023:
    CameraMove 2, 0x4000, 0, 0, 0x4000, 0, 0, 10, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
