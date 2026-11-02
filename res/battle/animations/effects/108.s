// Effect BTLV_EFFECT_FIRST + 108 (0x29d): member 108 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant Effect108
    ScriptVariant Effect108_Variant1

Effect108:
    SetYield FALSE
    SetTrainer 0, 8, 0, 0, 0
    McssAlpha 8, 1, 1, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    McssAlpha 8, 1, 31, 10, 0, 0
    McssScale 8, 1, 0x1800, 0x1800, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    End

Effect108_Variant1:
    SetYield FALSE
    DeleteMcss 8
    Wait EFFVM_WAIT_ALL
    McssSetMode 0, 4
    McssAlpha 0, 1, 31, 10, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
