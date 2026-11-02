// MOVE_SPARK's animation: member 209 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Spark

Spark:
    CameraMovePreset 1, 9, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W077_02, 1, EFFVM_ATTACKER, 20, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W129_04, 2, EFFVM_ATTACKER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 90, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 140, 0, 115, 0, 0, 0
    MoveSE 2, 0, 0, -1000, 2000, 0, 90, 0, 0
    SetYield TRUE
    SetYield FALSE
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    PaletteFade 2, 0, 8, 1, 13311
    McssPalFade EFFVM_ATTACKER, 0, 14, 1, 0x32d6
    ParticleLoad 379
    EmitterAdd 379, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 379, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 60
    PaletteFade 2, 8, 0, 1, 13311
    McssPalFade EFFVM_ATTACKER, 14, 0, 1, 0x32d6
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 0, 8, 0, 13311
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 8, 0, 0, 13311
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 0, 8, 0, 13311
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 8, 0, 0, 13311
    McssMoveXY EFFVM_ATTACKER, 2, 0x1000, 0, 2, 1, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W295_02, 1, EFFVM_DEFENDER, 20, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W077_02, 2, EFFVM_DEFENDER, 20, 0, 120, 0, 0, 0
    SetYield TRUE
    WaitFrames 12
    EmitterAdd 379, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 379, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 379, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 379, 5, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 3
    Wait EFFVM_WAIT_PARTICLES
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
