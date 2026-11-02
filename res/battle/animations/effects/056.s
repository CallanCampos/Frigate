// Effect BTLV_EFFECT_FIRST + 56 (0x269): member 56 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect056

Effect056:
    PaletteFade 2, 0, 10, 2, 0
    ParticleLoad 428
    EmitterAddOrtho 428, 2, 0, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 12
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_018, 1, EFFVM_PAN_CENTER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 428, 2, 0, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 428, 1, 0, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 428, 0, 0, 8, 0, 0x4000, 0, 0x800, 0x1000, 0x800, 0x1000
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 10, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    End
