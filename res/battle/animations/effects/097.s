// Effect BTLV_EFFECT_FIRST + 97 (0x292): member 97 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect097

Effect097:
    PlaySE SEQ_SE_RECOVERY, 5, EFFVM_PAN_CENTER, 0, 0, 127, 0, 0, 0
    Wait EFFVM_WAIT_SE
    End
    .balign 4
