// Effect BTLV_EFFECT_FIRST + 77 (0x27e): member 77 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect077

Effect077:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetVar EFFVM_VAR_CAMERA_LOCKED, TRUE
    End
