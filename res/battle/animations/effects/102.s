// Effect BTLV_EFFECT_FIRST + 102 (0x297): member 102 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect102

Effect102:
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
