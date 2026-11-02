// Effect BTLV_EFFECT_FIRST + 112 (0x2a1): member 112 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect112

Effect112:
    SetYield FALSE
    McssSetMode 0, 4
    McssAlpha 0, 1, 31, 10, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
