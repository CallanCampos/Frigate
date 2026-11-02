// Effect BTLV_EFFECT_FIRST + 99 (0x294): member 99 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant Effect099
    ScriptVariant Effect099_Variant1

Effect099:
    SetYield FALSE
    McssAlpha 0, 1, 1, 10, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetMode 0, 3
    SetTrainer 0, 8, -18432, -20480, 0
    McssScale 8, 1, 0x1800, 0x1800, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    End

Effect099_Variant1:
    SetYield FALSE
    DeleteMcss 8
    Wait EFFVM_WAIT_ALL
    McssSetMode 0, 4
    McssAlpha 0, 1, 31, 10, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
