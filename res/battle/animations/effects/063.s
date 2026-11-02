// Effect BTLV_EFFECT_FIRST + 63 (0x270): member 63 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect063

Effect063:
    SetYield FALSE
    McssPlayAnim 9, 2
    IfVarConst EFFVM_VAR_BATTLE_STYLE, EFFVM_CMP_NE, 0, Effect063_0086
    TrainerMove 9, 0, 0xbcd, 0x666, -0x9000, 0, 0, 0
    Jump Effect063_00A8

Effect063_0086:
    TrainerMove 9, 0, 0xbcd, 0x666, -0x9000, 0, 0, 0

Effect063_00A8:
    TrainerMove 9, 1, -0x5800, 0, 0, 1, 0, 0
    McssAlpha 9, 0, 1, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    CameraMove 2, 0x1000, 0, 0, 0x1000, 0, 0, 16, 0, 0
    McssCameraMode 0, 0
    TrainerMove 9, 1, 0x2000, 0, 0, 16, 0, 0
    McssAlpha 9, 1, 31, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
