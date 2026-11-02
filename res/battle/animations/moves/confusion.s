// MOVE_CONFUSION's animation: member 93 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Confusion

Confusion:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    PaletteFade 2, 0, 12, 1, 0
    SetYield FALSE
    PlaySE SEQ_SE_W048_01, 1, EFFVM_ATTACKER, 0, -500, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 30, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 100, 0, 20, 10, 0, 0
    SetYield TRUE
    McssMoveXY EFFVM_ATTACKER, 2, 0x4cd, 0, 2, 1, 3
    McssPalFade EFFVM_ATTACKER, 0, 8, 1, 0x7fff
    WaitFrames 10
    McssPalFade EFFVM_ATTACKER, 8, 0, 1, 0x7fff
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W095_01, 1, EFFVM_DEFENDER, 10, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W051_01, 2, EFFVM_DEFENDER, 15, 0, 120, 0, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_DEFENDER, 0, 12, 1, 0x58cc
    McssScale EFFVM_DEFENDER, 3, -0x333, 0, 2, 1, 3
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 1, 2, 6
    ParticleLoad 266
    EmitterAdd 266, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 22
    McssPalFade EFFVM_DEFENDER, 12, 0, 1, 0x58cc
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 12, 0, 1, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
