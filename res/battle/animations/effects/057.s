// Effect BTLV_EFFECT_FIRST + 57 (0x26a): member 57 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect057

Effect057:
    SetVar EFFVM_VAR_TARGET, 10
    PaletteFade 2, 0, 6, 6, 1023
    ParticleLoad 371
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_019, 1, EFFVM_PAN_CENTER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 371, 1, 0, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssPalFade EFFVM_ALL, 0, 6, 6, 0x3ff
    WaitFrames 12
    EmitterAddOrtho 371, 1, 0, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ALL, 6, 0, 6, 0x3ff
    PaletteFade 2, 6, 0, 6, 1023
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
