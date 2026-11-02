// MOVE_HEAT_WAVE's animation: member 257 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant HeatWave

HeatWave:
    CameraMovePreset 1, 14, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, TRUE
    PlaySE SEQ_SE_W239_01, 1, EFFVM_ATTACKER, 0, 0, 70, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 120, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 70, 0, 70, 50, 0, 0
    PlaySE SEQ_SE_W307_01, 2, EFFVM_ATTACKER, 0, 600, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 120, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 0, 2000, 0, 120, 0, 0
    MoveSE 2, 0, 1, 127, 0, 70, 50, 0, 0
    ParticleLoad 427
    EmitterAdd 427, 0, 0, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    PaletteFade 4, 0, 12, 1, 8470
    McssPalFade EFFVM_ATTACKER, 0, 12, 1, 0x2116
    McssPalFade EFFVM_DEFENDER, 0, 12, 1, 0x2116
    WaitFrames 10
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 3
    WaitFrames 12
    WaitFrames 80
    PaletteFade 4, 12, 0, 1, 8470
    McssPalFade EFFVM_ATTACKER, 12, 0, 1, 0x2116
    McssPalFade EFFVM_DEFENDER, 12, 0, 1, 0x2116
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
