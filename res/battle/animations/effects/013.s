// Effect BTLV_EFFECT_FIRST + 13 (0x23e): member 13 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect013

Effect013:
    McssCameraMode 0, 0
    SetYield FALSE
    LoadVar EFFVM_VAR_TRAINER_TYPE
    SetTrainer -1, 8, 0xfffc0000, 0xe8000, 0
    TrainerMove 8, 1, 0x68000, -0x50000, 0, 32, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
