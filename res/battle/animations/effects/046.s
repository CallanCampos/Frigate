// Effect BTLV_EFFECT_FIRST + 46 (0x25f): member 46 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect046

Effect046:
    IfVarConst EFFVM_VAR_ATTACKER_VANISH, EFFVM_CMP_EQ, TRUE, Effect046_01B4
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_10, 1, EFFVM_ATTACKER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_10, 1, EFFVM_ATTACKER, 26, 0, 127, 0, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_ATTACKER, 0, 12, 0, 0x7fff
    McssScale EFFVM_ATTACKER, 2, -0x4cd, 0x4cd, 12, 0, 2
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 12, 0, 0, 0x7fff
    ParticleLoad 153
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_09, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 153, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL

Effect046_01B4:
    End
    .balign 4
