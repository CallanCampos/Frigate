// Effect BTLV_EFFECT_FIRST + 68 (0x275): member 68 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect068

Effect068:
    IfVarConst EFFVM_VAR_ATTACKER_VANISH, EFFVM_CMP_EQ, TRUE, Effect068_02CA
    CameraMovePreset 1, 14, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 0, 6, 6, 0
    ParticleLoad 236
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_029, 1, EFFVM_DEFENDER, 0, -800, 100, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_029, 2, EFFVM_DEFENDER, 10, -600, 80, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_029, 1, EFFVM_DEFENDER, 20, -400, 70, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_029, 2, EFFVM_DEFENDER, 30, -200, 60, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_029, 1, EFFVM_DEFENDER, 40, -100, 50, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_029, 2, EFFVM_DEFENDER, 45, 0, 40, 0, 0, 0
    PanSE 1, 0, EFFVM_DEFENDER, EFFVM_ATTACKER, 0, 49, 0, 0
    PanSE 2, 0, EFFVM_DEFENDER, EFFVM_ATTACKER, 0, 49, 0, 0
    SetYield TRUE
    IfVarVar EFFVM_VAR_ATTACKER, EFFVM_CMP_EQ, EFFVM_VAR_DEFENDER, Effect068_01FA
    EmitterAddOrtho 236, 0, EFFVM_EMIT_DEFENDER, EFFVM_EMIT_ATTACKER, 0, 0xc00, 0, 0x800, 0x1000, 0x800, 0x1000

Effect068_01FA:
    WaitFrames 40
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_030, 1, EFFVM_ATTACKER, 5, 0, 127, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 236, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssPalFade EFFVM_ATTACKER, 0, 10, 2, 0x7fff
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 10, 0, 2, 0x7fff
    PaletteFade 2, 6, 0, 6, 1023
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL

Effect068_02CA:
    End
