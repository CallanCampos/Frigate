// Effect BTLV_EFFECT_FIRST + 101 (0x296): member 101 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant Effect101
    ScriptVariant Effect101_Variant1

Effect101:
    SetYield FALSE
    McssAlpha 1, 1, 1, 10, 0, 0
    McssAlpha 0, 1, 1, 10, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetMode 1, 3
    McssSetMode 0, 3
    SetTrainer 0, 8, 0, 0, 0
    SetTrainer 0, 9, 0, 0, 0
    McssScale 8, 1, 0x1800, 0x1800, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    End

Effect101_Variant1:
    SetYield FALSE
    DeleteMcss 8
    DeleteMcss 9
    Wait EFFVM_WAIT_ALL
    McssSetMode 0, 4
    McssSetMode 1, 4
    McssAlpha 0, 1, 31, 10, 0, 0
    McssAlpha 1, 1, 31, 10, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
