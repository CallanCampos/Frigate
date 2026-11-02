// Effect BTLV_EFFECT_FIRST + 92 (0x28d): member 92 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect092

Effect092:
    ParticleLoad 159
    SetYield FALSE
    PlaySE SEQ_SE_W377_01, 1, EFFVM_PAN_CENTER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W113_02, 2, EFFVM_PAN_CENTER, 40, 0, 127, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 159, 0, 0, 8, 0x1000, 0x3000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 159, 1, 0, 8, 0x1000, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 159, 2, 0, 8, 0x1000, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 159, 3, 0, 8, 0x1000, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
