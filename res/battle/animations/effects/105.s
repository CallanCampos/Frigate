// Effect BTLV_EFFECT_FIRST + 105 (0x29a): member 105 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect105

Effect105:
    SetYield FALSE
    McssAlpha 9, 1, 1, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetMode 9, 3
    DeleteMcss 9
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
