// Effect BTLV_EFFECT_FIRST + 55 (0x268): member 55 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect055

Effect055:
    PaletteFade 2, 0, 10, 2, 0
    ParticleLoad 411
    EmitterAddOrtho 411, 0, 0, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_017, 1, EFFVM_PAN_CENTER, 0, 0, 100, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 10, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    End
