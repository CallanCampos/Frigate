// MOVE_CALM_MIND's animation: member 347 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant CalmMind

CalmMind:
    PaletteFade 4, 0, 13, 0, 0
    SetVanish 0, 1
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W319_01, 1, EFFVM_ATTACKER, 0, 0, 70, 0, 0, 0
    PlaySE SEQ_SE_W014_01, 2, EFFVM_ATTACKER, 0, -500, 127, 0, 0, 0
    SetYield TRUE
    ParticleLoad 524
    EmitterAdd 524, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 524, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 524, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    SetVanish 0, 0
    PaletteFade 4, 13, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    End
