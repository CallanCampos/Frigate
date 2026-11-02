// Effect BTLV_EFFECT_FIRST + 54 (0x267): member 54 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect054

Effect054:
    ParticleLoad 219
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_016, 1, EFFVM_PAN_CENTER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 219, 0, 0, 8, 0, 0x3000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 10
    EmitterAddOrtho 219, 0, 0, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 10
    EmitterAddOrtho 219, 0, 0, 8, 0, 0, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 10
    EmitterAddOrtho 219, 0, 0, 8, 0, 0x3000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 10
    EmitterAddOrtho 219, 0, 0, 8, 0, 0x1000, 0, 0x800, 0x1000, 0x800, 0x1000
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
