// Effect BTLV_EFFECT_FIRST + 106 (0x29b): member 106 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect106

Effect106:
    SetYield FALSE
    McssAlpha 8, 1, 1, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetMode 8, 3
    DeleteMcss 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
