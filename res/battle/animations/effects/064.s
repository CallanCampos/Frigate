// Effect BTLV_EFFECT_FIRST + 64 (0x271): member 64 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect064

Effect064:
    CameraMovePreset 1, 8, 16, 0, 0
    McssCameraMode 0, 0
    McssMoveXY 9, 1, -0x2000, 0, 16, 0, 0
    McssAlpha 9, 1, 8, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    DeleteMcss 9
    End
    .balign 4
