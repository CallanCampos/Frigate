// Effect BTLV_EFFECT_FIRST + 107 (0x29c): member 107 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant Effect107
    ScriptVariant Effect107_Variant1

Effect107:
    SetYield FALSE
    McssAlpha 9, 1, 1, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    McssAlpha 9, 1, 31, 10, 0, 0
    Wait EFFVM_WAIT_ALL
    End

Effect107_Variant1:
    SetYield FALSE
    DeleteMcss 8
    Wait EFFVM_WAIT_ALL
    McssSetMode 0, 4
    McssAlpha 0, 1, 31, 10, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
