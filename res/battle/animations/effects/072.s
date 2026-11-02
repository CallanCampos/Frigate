// Effect BTLV_EFFECT_FIRST + 72 (0x279): member 72 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect072

Effect072:
    IfVarConst EFFVM_VAR_ATTACKER_VANISH, EFFVM_CMP_EQ, TRUE, Effect072_04B0
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    LoadBg 189
    SetVanish 2, 0
    PaletteFade 4, 0, 16, 1, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 1
    SetVanish 1, 1
    PaletteFade 3, 13, 0, 1, 0
    BgMove 4, 0, 24, 180, 0, 0
    McssSetValue EFFVM_ALL, 1
    ParticleLoad 643
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_033, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    OrbitEmittersAlt 643, 1, 1, 0x2000, 0x2000, 0, 60, 0, 3, 0
    OrbitEmittersAlt 643, 2, 1, 0x2000, 0x2000, 0, 60, 0, 3, 0
    OrbitEmittersAlt 643, 0, 1, 0x2000, 0x2000, 0, 60, 0, 3, 0
    McssSetVanish EFFVM_ATTACKER, FALSE
    WaitFrames 16
    McssScale EFFVM_ATTACKER, 2, -0x19a, 0x19a, 4, 2, 6
    OrbitEmittersAlt 643, 1, 1, 0x2000, 0x2000, 0, 60, 0, 3, 0
    OrbitEmittersAlt 643, 2, 1, 0x2000, 0x2000, 0, 60, 0, 3, 0
    OrbitEmittersAlt 643, 0, 1, 0x2000, 0x2000, 0, 60, 0, 3, 0
    WaitFrames 16
    OrbitEmittersAlt 643, 1, 1, 0x2000, 0x2000, 0, 60, 0, 3, 0
    OrbitEmittersAlt 643, 2, 1, 0x2000, 0x2000, 0, 60, 0, 3, 0
    OrbitEmittersAlt 643, 0, 1, 0x2000, 0x2000, 0, 60, 0, 3, 0
    McssPalFade EFFVM_ATTACKER, 0, 10, 1, 0x1f
    WaitFrames 16
    OrbitEmittersAlt 643, 1, 1, 0x2000, 0x2000, 0, 60, 0, 3, 0
    OrbitEmittersAlt 643, 2, 1, 0x2000, 0x2000, 0, 60, 0, 3, 0
    OrbitEmittersAlt 643, 0, 1, 0x2000, 0x2000, 0, 60, 0, 3, 0
    McssPalFade EFFVM_ATTACKER, 10, 0, 1, 0x1f
    WaitFrames 16
    McssPalFade EFFVM_ATTACKER, 0, 10, 1, 0x1f
    OrbitEmittersAlt 643, 1, 1, 0x2000, 0x2000, 0, 60, 0, 3, 0
    OrbitEmittersAlt 643, 2, 1, 0x2000, 0x2000, 0, 60, 0, 3, 0
    OrbitEmittersAlt 643, 0, 1, 0x2000, 0x2000, 0, 60, 0, 3, 0
    Wait EFFVM_WAIT_PARTICLES
    PaletteFade 3, 0, 16, 1, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 16, 0, 0, 0
    SetVanish 0, 0
    SetVanish 1, 0
    SetVanish 2, 1
    McssPalFade EFFVM_ATTACKER, 10, 0, 1, 0x1f
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetValue EFFVM_ALL, 0
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL

Effect072_04B0:
    End
    .balign 4
