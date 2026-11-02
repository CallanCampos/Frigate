// Effect BTLV_EFFECT_FIRST + 47 (0x260): member 47 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect047

Effect047:
    ParticleLoad 154
    IfVarConst EFFVM_VAR_ATTACKER_SIDE, EFFVM_CMP_EQ, 0, Effect047_014A
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_11, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 154, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x800, 0x800, 0x800
    EmitterAddOrtho 154, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x800, 0x800, 0x800
    ParticleLoad 426
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_09, 1, EFFVM_ATTACKER, 42, 0, 110, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 426, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x800, 0x800, 0x800
    Return

Effect047_014A:
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_11, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 154, 0, 13, 8, 0, 0x2000, 0, 0x800, 0x800, 0x800, 0x800
    EmitterAddOrtho 154, 1, 13, 8, 0, 0x2000, 0, 0x800, 0x800, 0x800, 0x800
    ParticleLoad 426
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_09, 1, EFFVM_ATTACKER, 42, 0, 110, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 426, 2, 13, 8, 0, 0x2000, 0, 0x800, 0x800, 0x800, 0x800
    Return
