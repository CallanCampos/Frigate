// Effect BTLV_EFFECT_FIRST + 19 (0x244): member 19 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect019

Effect019:
    SetYield FALSE
    CameraMove 1, 0x8b33, 0x7b33, 0x17ccd, 0x2000, 0x399a, 0x6800, 16, 0, 0
    McssCameraMode 0, 0
    McssScale EFFVM_PLAYER_SIDE, 1, 0xa66, 0xa66, 16, 0, 0
    McssScale EFFVM_ENEMY_SIDE, 1, 0xb9a, 0xb9a, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    SetVar EFFVM_VAR_MCSS_SCALED, TRUE
    SetVar EFFVM_VAR_CAMERA_SAVED, FALSE
    End
    .balign 4
