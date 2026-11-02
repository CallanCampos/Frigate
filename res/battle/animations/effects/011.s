// Effect BTLV_EFFECT_FIRST + 11 (0x23c): member 11 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect011

Effect011:
    McssCameraMode 0, 0
    SetYield FALSE
    CameraMove 2, 0xa000, 0x1000, 0x10000, 0x6000, 0x2000, 0x10000, 32, 0, 32
    SetTrainer 38, 8, -61440, -45056, 0
    IfPokeExists 1, FALSE, Effect011_00B8
    McssScale 1, 1, 0x800, 0x800, 12, 2, 0

Effect011_00B8:
    IfPokeExists 3, FALSE, Effect011_00E4
    McssScale 3, 1, 0x800, 0x800, 12, 2, 0

Effect011_00E4:
    IfPokeExists 5, FALSE, Effect011_0110
    McssScale 5, 1, 0x800, 0x800, 12, 2, 0

Effect011_0110:
    IfPokeExists 7, FALSE, Effect011_013C
    McssScale 7, 1, 0x800, 0x800, 12, 2, 0

Effect011_013C:
    Wait EFFVM_WAIT_ALL
    End
