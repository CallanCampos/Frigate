// Effect BTLV_EFFECT_FIRST + 78 (0x27f): member 78 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect078

Effect078:
    CameraMovePreset 1, 9, 16, 0, 8
    PaletteFade 4, 0, 16, 3, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 1
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W129_04, 1, EFFVM_ATTACKER, 10, 0, 115, 0, 0, 0
    MoveSE 1, 0, 0, 2000, 0, 0, 50, 0, 0
    PlaySE SEQ_SE_DECIDE3, 2, EFFVM_ATTACKER, 70, 0, 115, 0, 0, 0
    SetYield TRUE
    ParticleLoad 1
    EmitterAdd 1, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 50
    EmitterAdd 1, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_ATTACKER, 0, 10, 0, 0x7fff
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 0
    PaletteFade 4, 16, 0, 3, 0
    CameraMovePreset 1, 8, 16, 0, 8
    McssPalFade EFFVM_ATTACKER, 10, 0, 0, 0x7fff
    Wait EFFVM_WAIT_ALL
    End
