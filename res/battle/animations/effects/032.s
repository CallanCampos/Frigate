// Effect BTLV_EFFECT_FIRST + 32 (0x251): member 32 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect032

Effect032:
    IfVarConst 55, EFFVM_CMP_EQ, 0, Effect032_011A
    SetYield FALSE
    McssCameraMode 0, 0
    PaletteFade 2, 0, 16, 1, 0
    McssPalFade EFFVM_ALL, 0, 16, 1, 0
    Wait EFFVM_WAIT_MCSS
    Wait EFFVM_WAIT_STAGE_FIELD_FADE
    SetYield FALSE
    CameraMovePreset 1, 8, 1, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    McssSetMode EFFVM_ALL, 5
    SetVanish 0, 0
    PaletteFade 2, 16, 0, 1, 0
    McssPalFade EFFVM_ALL, 16, 0, 1, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    Gauge 1, 2
    Wait EFFVM_WAIT_ALL

Effect032_011A:
    SetVar 55, 0
    End
    .balign 4
