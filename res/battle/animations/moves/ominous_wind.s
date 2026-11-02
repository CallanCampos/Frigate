// MOVE_OMINOUS_WIND's animation: member 466 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant OminousWind

OminousWind:
    CameraMovePreset 1, 14, 24, 0, 0
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 646
    PlaySE SEQ_SE_W239_01, 1, EFFVM_ATTACKER, 0, -1000, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 110, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 115, 0, 70, 40, 0, 0
    PlaySE SEQ_SE_W307_01, 2, EFFVM_ATTACKER, 0, -1000, 90, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 110, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 115, 0, 70, 40, 0, 0
    EmitterAdd 646, 0, 0, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    PaletteFade 4, 0, 12, 1, 6150
    McssPalFade EFFVM_DEFENDER, 0, 12, 1, 0x300c
    McssPalFade EFFVM_ATTACKER, 0, 12, 1, 0x300c
    WaitFrames 20
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 13
    WaitFrames 60
    McssPalFade EFFVM_DEFENDER, 12, 0, 1, 0x300c
    McssPalFade EFFVM_ATTACKER, 12, 0, 1, 0x300c
    PaletteFade 4, 12, 0, 1, 6150
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 17, 24, 0, 0
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
