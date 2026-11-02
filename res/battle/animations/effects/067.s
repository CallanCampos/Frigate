// Effect BTLV_EFFECT_FIRST + 67 (0x274): member 67 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect067

Effect067:
    IfVarConst EFFVM_VAR_ATTACKER_VANISH, EFFVM_CMP_EQ, TRUE, Effect067_019C
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 0, 12, 3, 0
    Wait EFFVM_WAIT_ALL
    LoadBg 69
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_028, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    SetVanish 0, 1
    SetVanish 1, 1
    SetVanish 2, 0
    BgMove 2, 4, 4, 16, 0, 4
    WaitFrames 30
    PaletteFade 3, 0, 12, 3, 0
    McssPalFade EFFVM_ATTACKER, 0, 10, 2, 0x7fff
    WaitFrames 30
    McssPalFade EFFVM_ATTACKER, 10, 0, 2, 0x7fff
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 0
    SetVanish 1, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 12, 0, 3, 0
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL

Effect067_019C:
    End
    .balign 4
