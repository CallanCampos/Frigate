// Effect BTLV_EFFECT_FIRST + 114 (0x2a3): member 114 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect114

Effect114:
    ParticleLoad 163
    IfVarConst EFFVM_VAR_ATTACKER_SIDE, EFFVM_CMP_EQ, 0, Effect114_01AA
    PaletteFade 1, 0, 12, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_SW_STARPOKE, 5, EFFVM_PAN_RIGHT, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 163, 0, 13, 8, 0, 0, 0, 0x800, 0, 0x800, 0
    EmitterAddOrtho 163, 1, 13, 8, 0, 0, 0, 0x800, 0, 0x800, 0
    EmitterAddOrtho 163, 2, 13, 8, 0, 0, 0, 0x800, 0, 0x800, 0
    EmitterAddOrtho 163, 3, 13, 8, 0, 0, 0, 0x800, 0, 0x800, 0
    SetYield FALSE
    SetYield TRUE
    McssPalFade EFFVM_ATTACKER, 0, 14, 4, 0x6680
    McssPalFade EFFVM_ATTACKER, 14, 0, 4, 0x6680
    WaitFrames 65
    PaletteFade 1, 6, 0, 0, 0
    Return

Effect114_01AA:
    PaletteFade 1, 0, 12, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_SW_STARPOKE, 5, EFFVM_PAN_LEFT, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 163, 0, 13, 8, 0x1000, 0x800, 0x1000, 0x800, 0, 0x800, 0
    EmitterAddOrtho 163, 1, 13, 8, 0x1000, 0x800, 0x1000, 0x800, 0, 0x800, 0
    EmitterAddOrtho 163, 2, 13, 8, 0x1000, 0x800, 0x1000, 0x800, 0, 0x800, 0
    EmitterAddOrtho 163, 3, 13, 8, 0x1000, 0x800, 0x1000, 0x800, 0, 0x800, 0
    SetYield FALSE
    SetYield TRUE
    McssPalFade EFFVM_ATTACKER, 0, 14, 4, 0x6680
    McssPalFade EFFVM_ATTACKER, 14, 0, 4, 0x6680
    WaitFrames 65
    PaletteFade 1, 12, 0, 0, 0
    Return
