// Effect BTLV_EFFECT_FIRST + 24 (0x249): member 24 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect024

Effect024:
    CameraMove 2, -0x2000, 0, 0, -0x2000, 0, 0, 10, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
