// Effect BTLV_EFFECT_FIRST + 103 (0x298): member 103 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant Effect103
    ScriptVariant Effect103_Variant1

Effect103:
    SetYield FALSE
    McssAlpha 1, 1, 1, 10, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetMode 1, 3
    McssScale 9, 1, 0x1000, 0x1000, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    End

Effect103_Variant1:
    SetYield FALSE
    DeleteMcss 8
    Wait EFFVM_WAIT_ALL
    McssSetMode 0, 4
    McssAlpha 0, 1, 31, 10, 0, 0
    Wait EFFVM_WAIT_ALL
    End
