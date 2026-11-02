// Effect BTLV_EFFECT_FIRST + 113 (0x2a2): member 113 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect113

Effect113:
    SetYield FALSE
    PlaySE SEQ_SE_W003_01, 5, EFFVM_PAN_CENTER, 16, 0, 127, 0, 0, 0
    ParticleLoad 147
    EmitterAddOrtho 147, 0, 1, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddOrtho 147, 1, 1, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddOrtho 147, 2, 1, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddOrtho 147, 3, 1, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddOrtho 147, 4, 1, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddOrtho 147, 5, 1, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddOrtho 147, 6, 1, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddOrtho 147, 7, 1, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddOrtho 147, 8, 1, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddOrtho 147, 9, 1, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddOrtho 147, 10, 1, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddOrtho 147, 11, 1, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddOrtho 147, 12, 1, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddOrtho 147, 13, 1, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
