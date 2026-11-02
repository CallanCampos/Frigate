// MOVE_SANDSTORM's animation: member 201 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Sandstorm

Sandstorm:
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    PaletteFade 4, 0, 14, 1, 268
    SetYield FALSE
    PlaySE SEQ_SE_W239_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 100, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 2000, 0, 100, 0, 0
    PlaySE SEQ_SE_W157_03, 3, EFFVM_ATTACKER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 3, EFFVM_ATTACKER, 100, 0, 0, 0, 0, 0
    MoveSE 3, 0, 1, 100, 0, 60, 40, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_ATTACKER, 0, 10, 1, 0x10c
    McssPalFade EFFVM_DEFENDER, 0, 10, 1, 0x10c
    ParticleLoad 371
    EmitterAdd 371, 0, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 371, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 371, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 44
    PaletteFade 4, 14, 0, 1, 268
    McssPalFade EFFVM_ATTACKER, 10, 0, 1, 0x10c
    McssPalFade EFFVM_DEFENDER, 10, 0, 1, 0x10c
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
