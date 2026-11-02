// Effect BTLV_EFFECT_FIRST + 2 (0x233): member 2 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect002

Effect002:
    McssCameraMode 0, 0
    SetYield FALSE
    CameraMove 2, 0xa000, 0x1000, 0x10000, 0x6000, 0x2000, 0x10000, 32, 0, 32
    LoadVar EFFVM_VAR_TRAINER_TYPE
    SetTrainer -1, 10, -61440, -45056, 0
    LoadVar EFFVM_VAR_TRAINER_TYPE + 2
    SetTrainer -1, 12, -36864, -45056, -4096
    IfPokeExists 1, FALSE, Effect002_00DA
    McssScale 1, 1, 0x800, 0x800, 12, 2, 0

Effect002_00DA:
    IfPokeExists 3, FALSE, Effect002_0106
    McssScale 3, 1, 0x800, 0x800, 12, 2, 0

Effect002_0106:
    IfPokeExists 5, FALSE, Effect002_0132
    McssScale 5, 1, 0x800, 0x800, 12, 2, 0

Effect002_0132:
    IfPokeExists 7, FALSE, Effect002_015E
    McssScale 7, 1, 0x800, 0x800, 12, 2, 0

Effect002_015E:
    IfPokeExists 9, FALSE, Effect002_018A
    McssScale 9, 1, 0x800, 0x800, 12, 2, 0

Effect002_018A:
    IfPokeExists 11, FALSE, Effect002_01B6
    McssScale 11, 1, 0x800, 0x800, 12, 2, 0

Effect002_01B6:
    IfPokeExists 13, FALSE, Effect002_01E2
    McssScale 13, 1, 0x800, 0x800, 12, 2, 0

Effect002_01E2:
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
