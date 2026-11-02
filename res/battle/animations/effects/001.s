// Effect BTLV_EFFECT_FIRST + 1 (0x232): member 1 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect001

Effect001:
    SetYield FALSE
    McssCameraMode 0, 0
    CameraMove 2, 0xa000, 0x1000, 0x10000, 0x6000, 0x2000, 0x10000, 32, 0, 32
    LoadVar EFFVM_VAR_TRAINER_TYPE
    SetTrainer -1, 8, -61440, -45056, 0
    IfPokeExists 1, FALSE, Effect001_00BE
    McssScale 1, 1, 0x800, 0x800, 12, 2, 0

Effect001_00BE:
    IfPokeExists 3, FALSE, Effect001_00EA
    McssScale 3, 1, 0x800, 0x800, 12, 2, 0

Effect001_00EA:
    IfPokeExists 5, FALSE, Effect001_0116
    McssScale 5, 1, 0x800, 0x800, 12, 2, 0

Effect001_0116:
    IfPokeExists 7, FALSE, Effect001_0142
    McssScale 7, 1, 0x800, 0x800, 12, 2, 0

Effect001_0142:
    IfPokeExists 9, FALSE, Effect001_016E
    McssScale 9, 1, 0x800, 0x800, 12, 2, 0

Effect001_016E:
    IfPokeExists 11, FALSE, Effect001_019A
    McssScale 11, 1, 0x800, 0x800, 12, 2, 0

Effect001_019A:
    IfPokeExists 13, FALSE, Effect001_01C6
    McssScale 13, 1, 0x800, 0x800, 12, 2, 0

Effect001_01C6:
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
