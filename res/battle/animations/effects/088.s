// Effect BTLV_EFFECT_FIRST + 88 (0x289): member 88 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect088

Effect088:
    SetYield FALSE
    CameraMove 1, 0xb33, 0x6b33, 0x1399a, -0x6000, 0x299a, 0x24cd, 20, 0, 18
    McssCameraMode 0, 0
    Wait EFFVM_WAIT_ALL
    Return
    .balign 4
