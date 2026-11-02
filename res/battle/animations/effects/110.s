// Effect BTLV_EFFECT_FIRST + 110 (0x29f): member 110 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant Effect110
    ScriptVariant Effect110_Variant1

Effect110:
    SetYield FALSE
    McssAlpha 0, 1, 1, 10, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetMode 0, 3
    Wait EFFVM_WAIT_ALL
    End

Effect110_Variant1:
    SetYield FALSE
    DeleteMcss 8
    Wait EFFVM_WAIT_ALL
    McssSetMode 0, 4
    McssAlpha 0, 1, 31, 10, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
