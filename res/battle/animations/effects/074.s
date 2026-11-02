// Effect BTLV_EFFECT_FIRST + 74 (0x27b): member 74 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect074

Effect074:
    IfVarConst EFFVM_VAR_ATTACKER_VANISH, EFFVM_CMP_EQ, TRUE, Effect074_0290
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 0, 12, 0, 0
    LoadBg 144
    Wait EFFVM_WAIT_ALL
    PaletteFade 3, 12, 0, 3, 0
    SetVanish 2, 0
    SetVanish 0, 1
    SetVanish 1, 1
    Wait EFFVM_WAIT_BG
    BgMove 1, 60, 0, 120, 0, 0
    ParticleLoad 420
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_037, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 420, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 420, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 420, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 420, 3, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssPalFade EFFVM_ATTACKER, 0, 12, 2, 0x514a
    McssCircle EFFVM_ATTACKER, 2, 0, 0x333, 0x333, 0xa000, 0, 0x8000, 0
    WaitFrames 20
    McssPalFade EFFVM_ATTACKER, 12, 0, 2, 0x514a
    Wait EFFVM_WAIT_PARTICLES
    PaletteFade 3, 0, 12, 2, 0
    SetVanish 0, 0
    SetVanish 1, 0
    PaletteFade 2, 12, 0, 2, 0
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL

Effect074_0290:
    End
    .balign 4
