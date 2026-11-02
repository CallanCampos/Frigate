// Effect BTLV_EFFECT_FIRST + 5 (0x236): member 5 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect005

Effect005:
    SetYield FALSE
    SetVar EFFVM_VAR_CAMERA_LOCKED, FALSE
    CameraMovePreset 1, 8, 16, 0, 0
    McssScale EFFVM_ALL, 1, 0x1000, 0x1000, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    Gauge 1, 2
    SetVar EFFVM_VAR_CAMERA_SAVED, FALSE
    End
