// Effect BTLV_EFFECT_FIRST + 80 (0x281): member 80 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect080

Effect080:
    LoadBg 228
    SetYield FALSE
    QueueCommands 0, 255, 20560, 5919, 14, 0, 1
    SetVanish 2, 0
    BgSetPriority 0
    Wait EFFVM_WAIT_WINDOW
    BgMove 1, 256, 0, 32, 0, 0
    Wait EFFVM_WAIT_BG
    WaitFrames 30
    QueueCommands 1, 255, 16990, 5919, 14, 0, 1
    Wait EFFVM_WAIT_WINDOW
    SetVanish 2, 1
    QueueCommands 1, 255, 20305, 5919, 1, 0, 0
    Wait EFFVM_WAIT_WINDOW
    End
    .balign 4
