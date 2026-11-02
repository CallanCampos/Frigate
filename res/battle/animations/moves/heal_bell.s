// MOVE_HEAL_BELL's animation: member 215 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant HealBell

HealBell:
    CameraMovePreset 1, 9, 16, 0, 12
    ParticleLoad 385
    EmitterAdd 385, 0, EFFVM_EMIT_ATTACKER, 8, 0x2800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 385, 1, EFFVM_EMIT_ATTACKER, 8, 0x2800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    PaletteFade 4, 0, 8, 1, 32767
    SetYield FALSE
    PlaySE SEQ_SE_W215_01, 1, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 130, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 110, 0, 90, 40, 0, 0
    PlaySE SEQ_SE_W215_02, 2, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 45, 0, 0, 0, 0, 0
    SetYield TRUE
    EmitterAdd 385, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 385, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_ATTACKER, 0, 8, 0, 0x7fff
    WaitFrames 16
    McssPalFade EFFVM_ATTACKER, 8, 0, 1, 0x7fff
    PaletteFade 4, 8, 0, 1, 32767
    WaitFrames 16
    PaletteFade 4, 0, 8, 1, 32767
    EmitterAdd 385, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 385, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_ATTACKER, 0, 8, 1, 0x7fff
    WaitFrames 16
    PaletteFade 4, 8, 0, 1, 32767
    McssPalFade EFFVM_ATTACKER, 8, 0, 1, 0x7fff
    WaitFrames 16
    PaletteFade 4, 0, 8, 1, 32767
    EmitterAdd 385, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 385, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_ATTACKER, 0, 8, 1, 0x7fff
    WaitFrames 16
    PaletteFade 4, 8, 0, 1, 32767
    McssPalFade EFFVM_ATTACKER, 8, 0, 1, 0x7fff
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
