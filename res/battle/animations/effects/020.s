// Effect BTLV_EFFECT_FIRST + 20 (0x245): member 20 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect020

Effect020:
    McssCameraMode 0, 0
    SetYield FALSE
    SetVar EFFVM_VAR_MCSS_SCALED, TRUE
    CameraMove 1, 0x8b33, 0x7b33, 0x1b4cd, 0x2000, 0x399a, 0xa000, 16, 0, 0
    McssScale EFFVM_ALL, 1, 0xc00, 0xc00, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
