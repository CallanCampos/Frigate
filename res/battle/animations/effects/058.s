// Effect BTLV_EFFECT_FIRST + 58 (0x26b): member 58 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect058

Effect058:
    SetVar EFFVM_VAR_TARGET, 10
    PaletteFade 2, 0, 6, 6, 1023
    ParticleLoad 412
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_020, 1, EFFVM_PAN_CENTER, 0, 0, 100, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 412, 0, 0, 8, 0x3000, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssPalFade EFFVM_ALL, 0, 6, 6, 0x3ff
    WaitFrames 12
    EmitterAddOrtho 412, 1, 0, 8, 0x3000, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 412, 2, 0, 8, 0x3000, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ALL, 6, 0, 6, 0x3ff
    PaletteFade 2, 6, 0, 6, 1023
    Wait EFFVM_WAIT_ALL
    End
