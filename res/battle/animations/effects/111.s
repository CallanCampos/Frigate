// Effect BTLV_EFFECT_FIRST + 111 (0x2a0): member 111 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect111

Effect111:
    SetYield FALSE
    McssSetMode 1, 4
    McssAlpha 1, 1, 31, 10, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
