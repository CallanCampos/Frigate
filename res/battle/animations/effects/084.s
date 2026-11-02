// Effect BTLV_EFFECT_FIRST + 84 (0x285): member 84 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect084

Effect084:
    McssSetMode EFFVM_ATTACKER, 3
    McssEffect 0, EFFVM_ATTACKER, 0
    End
    .balign 4
