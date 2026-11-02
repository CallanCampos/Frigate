// MOVE_RELIC_SONG's animation: member 547 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant RelicSong

RelicSong:
    CameraMovePreset 1, 9, 16, 0, 8
    PaletteFade 4, 0, 10, 2, 32767
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W547_INIMELO, 1, EFFVM_ATTACKER, 0, -500, 80, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 20, 240, 0, 0
    MoveSE 1, 0, 1, 80, 0, 100, 100, 0, 0
    SetYield TRUE
    ParticleLoad 720
    EmitterAdd 720, 5, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 6
    EmitterAdd 720, 6, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 6
    EmitterMove 720, 2, 6, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x96000, 0x800, 0x1000, 0x1000, 0x4000
    WaitFrames 6
    EmitterMove 720, 3, 6, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x96000, 0x800, 0x1000, 0x1000, 0x4000
    WaitFrames 6
    EmitterMove 720, 4, 6, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x96000, 0x800, 0x1000, 0x1000, 0x4000
    WaitFrames 6
    EmitterMove 720, 2, 6, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x96000, 0x800, 0x1000, 0x1000, 0x4000
    WaitFrames 6
    EmitterMove 720, 4, 6, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x96000, 0x800, 0x1000, 0x1000, 0x4000
    WaitFrames 6
    EmitterMove 720, 2, 6, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x96000, 0x800, 0x1000, 0x1000, 0x4000
    WaitFrames 6
    EmitterMove 720, 3, 6, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x96000, 0x800, 0x1000, 0x1000, 0x4000
    CameraMovePreset 1, 11, 24, 0, 8
    WaitFrames 6
    EmitterMove 720, 4, 6, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x96000, 0x800, 0x1000, 0x1000, 0x4000
    WaitFrames 6
    EmitterMove 720, 2, 6, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x96000, 0x800, 0x1000, 0x1000, 0x4000
    WaitFrames 6
    EmitterMove 720, 4, 6, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x96000, 0x800, 0x1000, 0x1000, 0x4000
    WaitFrames 10
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 40
    SetYield FALSE
    PlaySE SEQ_SE_W255_MOMOMO, 2, EFFVM_DEFENDER, 0, 1000, 30, 0, 0, 0
    PlaySE SEQ_SE_W255_MOMOMO, 2, EFFVM_DEFENDER, 30, 2000, 20, 0, 0, 0
    SetYield TRUE
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 6, 0, 6
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 2, 0, 20
    EmitterAdd 720, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    EmitterAdd 720, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 720, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    EmitterAdd 720, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    Wait EFFVM_WAIT_MCSS
    CameraMovePreset 1, 8, 10, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    PaletteFade 4, 10, 0, 2, 32767
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
