// Effect BTLV_EFFECT_FIRST + 98 (0x293): member 98 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect098

Effect098:
    ParticleLoad 146
    IfVarConst EFFVM_VAR_ATTACKER_SIDE, EFFVM_CMP_EQ, 0, Effect098_0154
    PaletteFade 1, 0, 6, 0, 0
    WaitFrames 15
    SetYield FALSE
    PlaySE SEQ_SE_SW_NPOKE, 5, EFFVM_PAN_RIGHT, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 146, 1, 13, 8, 0, -0x1000, 0, 0x800, 0, 0x800, 0
    EmitterAddOrtho 146, 2, 13, 8, 0, -0x1000, 0, 0x800, 0, 0x800, 0
    SetYield FALSE
    SetYield TRUE
    McssPalFade EFFVM_ATTACKER, 0, 14, 4, 0x2a8a
    McssPalFade EFFVM_ATTACKER, 14, 0, 4, 0x2bca
    WaitFrames 60
    PaletteFade 1, 6, 0, 0, 0
    Return

Effect098_0154:
    PaletteFade 1, 0, 12, 0, 0
    WaitFrames 15
    SetYield FALSE
    PlaySE SEQ_SE_SW_NPOKE, 5, EFFVM_PAN_LEFT, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 146, 1, 13, 8, 0, -0x1000, 0, 0x800, 0, 0x800, 0
    EmitterAddOrtho 146, 2, 13, 8, 0, -0x1000, 0, 0x800, 0, 0x800, 0
    SetYield FALSE
    SetYield TRUE
    McssPalFade EFFVM_ATTACKER, 0, 14, 4, 0x2a8a
    McssPalFade EFFVM_ATTACKER, 14, 0, 4, 0x2a8a
    WaitFrames 60
    PaletteFade 1, 12, 0, 0, 0
    Return
