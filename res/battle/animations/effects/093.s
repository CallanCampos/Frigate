// Effect BTLV_EFFECT_FIRST + 93 (0x28e): member 93 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect093

Effect093:
    CameraMove 2, 0x3000, 0, 0x6000, 0, 0, 0, 32, 0, 28
    PaletteFade 4, 0, 10, 1, 31
    McssSetVanish EFFVM_ATTACKER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_016, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_016, 2, EFFVM_ATTACKER, 20, 0, 127, 0, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_ATTACKER, 0, 10, 1, 0x1f
    ParticleLoad 160
    EmitterAdd 160, 0, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x3000, 0x1000, 0x1800, 0x1000
    EmitterAdd 160, 1, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x3000, 0x1000, 0x1800, 0x1000
    WaitFrames 40
    McssPalFade EFFVM_ATTACKER, 10, 0, 1, 0x1f
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    PaletteFade 4, 10, 0, 1, 31
    Wait EFFVM_WAIT_ALL
    End
