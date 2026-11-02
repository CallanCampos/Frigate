// Effect BTLV_EFFECT_FIRST + 6 (0x237): member 6 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect006

Effect006:
    SetYield FALSE
    IfPokeExists 9, FALSE, Effect006_0066
    McssPalFade 9, 16, 16, 0, 0

Effect006_0066:
    IfPokeExists 11, FALSE, Effect006_008A
    McssPalFade 11, 16, 16, 0, 0

Effect006_008A:
    IfPokeExists 13, FALSE, Effect006_00AE
    McssPalFade 13, 16, 16, 0, 0

Effect006_00AE:
    CameraMove 0, 0x4cd, 0x7800, -0xf000, 0x4cd, 0x3800, -0x19000, 0, 0, 0
    CameraRotate 0, -2000, -16000, 0, 0, 0
    CameraMovePreset 1, 1, 70, 0, 0
    WaitFrames 20
    CameraMovePreset 1, 8, 30, 0, 28
    WaitFrames 15
    IfPokeExists 9, FALSE, Effect006_014E
    McssPalFade 9, 16, 0, 1, 0

Effect006_014E:
    IfPokeExists 11, FALSE, Effect006_0172
    McssPalFade 11, 16, 0, 1, 0

Effect006_0172:
    IfPokeExists 13, FALSE, Effect006_0196
    McssPalFade 13, 16, 0, 1, 0

Effect006_0196:
    Wait EFFVM_WAIT_ALL
    IfPokeExists 9, FALSE, Effect006_01B4
    McssPlayAnim 9, 0

Effect006_01B4:
    IfPokeExists 11, FALSE, Effect006_01CC
    McssPlayAnim 11, 0

Effect006_01CC:
    IfPokeExists 13, FALSE, Effect006_01E4
    McssPlayAnim 13, 0

Effect006_01E4:
    End
    .balign 4
