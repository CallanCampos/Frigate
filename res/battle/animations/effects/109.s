// Effect BTLV_EFFECT_FIRST + 109 (0x29e): member 109 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant Effect109
    ScriptVariant Effect109_Variant1

Effect109:
    SetYield FALSE
    McssAlpha 1, 1, 1, 10, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetMode 1, 3
    Wait EFFVM_WAIT_ALL
    End

Effect109_Variant1:
    SetYield FALSE
    DeleteMcss 8
    Wait EFFVM_WAIT_ALL
    McssSetMode 0, 4
    McssAlpha 0, 1, 31, 10, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
