// MOVE_BLAST_BURN's animation: member 307 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant BlastBurn

BlastBurn:
    PaletteFade 4, 0, 12, 3, 20
    PlaySE SEQ_SE_W123_01, 1, EFFVM_ATTACKER, 0, 0, 70, 0, 0, 0
    PlaySE SEQ_SE_W055_01, 0, EFFVM_ATTACKER, 0, -2000, 80, 0, 0, 0
    MoveSE 0, 0, 0, -2000, 2000, 0, 200, 0, 0
    PanSE 0, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 200, 0, 0
    PlaySE SEQ_SE_DUMMY5, 0, EFFVM_DEFENDER, 200, 0, 0, 0, 0, 0
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    ParticleLoad 478
    EmitterAdd 478, 6, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 478, 0, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 478, 3, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W307_04, 1, EFFVM_ATTACKER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W307_04, 1, EFFVM_ATTACKER, 25, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W307_04, 1, EFFVM_ATTACKER, 50, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W063_01, 2, EFFVM_DEFENDER, 70, 0, 40, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 59, 40, 0, 0
    SetYield TRUE
    McssSetValue EFFVM_ALL, 1
    WaitFrames 20
    EmitterAdd 478, 7, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 478, 1, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 478, 4, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    EmitterAdd 478, 8, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 478, 2, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 478, 5, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 10, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W063_01, 1, EFFVM_DEFENDER, 0, 0, 40, 0, 0, 0
    PlaySE SEQ_SE_W307_EXP, 1, EFFVM_DEFENDER, 20, -500, 100, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_027, 2, EFFVM_DEFENDER, 25, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_027, 2, EFFVM_DEFENDER, 30, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W055_01, 2, EFFVM_DEFENDER, 50, 0, 90, 0, 0, 0
    MoveSE 1, 0, 0, 0, -1000, 120, 40, 0, 0
    MoveSE 2, 0, 1, 90, 0, 120, 40, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    Wait EFFVM_WAIT_ALL
    EmitterAdd 478, 0, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 478, 6, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    EmitterAdd 478, 9, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x99a, 0x1000, 0x1000
    WaitFrames 2
    McssPalFade EFFVM_DEFENDER, 0, 10, 2, 0x1f
    EmitterAdd 478, 10, EFFVM_EMIT_DEFENDER, 8, 0xccd, 0, 0, 0x1000, 0x99a, 0x99a, 0x1000
    EmitterAdd 478, 11, EFFVM_EMIT_DEFENDER, 8, 0xccd, 0, 0, 0x1000, 0x99a, 0x99a, 0x1000
    WaitFrames 10
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 1, 6
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 10, 0, 2, 0x14
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetValue EFFVM_ALL, 0
    CameraMovePreset 1, 8, 16, 0, 8
    PaletteFade 4, 12, 0, 3, 30
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
