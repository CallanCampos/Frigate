// MOVE_MIST_BALL's animation: member 296 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant MistBall

MistBall:
    McssSetValue EFFVM_ALL, 1
    CameraMovePreset 1, 9, 12, 0, 8
    McssSetVanish EFFVM_ATTACKER, TRUE
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W124_01, 1, EFFVM_ATTACKER, 35, 0, 110, 0, 0, 0
    MoveSE 1, 0, 1, 110, 0, 50, 13, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 68, 0, 0, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 50, 18, 0, 0
    SetYield TRUE
    LoadBg 18
    SetVanish 2, 0
    PaletteFade 4, 0, 15, 1, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 1
    SetVanish 1, 1
    PaletteFade 4, 15, 0, 1, 0
    BgMove 4, -20, 0, 150, 1, 0
    ParticleLoad 466
    EmitterMove 466, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x1e000, 0, 0x1000, 0x1000, 0
    CameraMovePreset 1, 11, 10, 0, 6
    WaitFrames 20
    SetYield FALSE
    PlaySE SEQ_SE_W051_01, 1, EFFVM_DEFENDER, 5, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W017_01, 2, EFFVM_DEFENDER, 15, 0, 115, 0, 0, 0
    SetYield TRUE
    EmitterAdd 466, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x666, 0x1000
    EmitterAdd 466, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x666, 0x1000
    EmitterAdd 466, 1, 0, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x14cd, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 3
    WaitFrames 60
    PaletteFade 4, 0, 15, 1, 0
    WaitFrames 30
    SetVanish 1, 0
    SetVanish 0, 0
    WaitFrames 30
    PaletteFade 4, 15, 0, 1, 0
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    McssSetValue EFFVM_ALL, 0
    Wait EFFVM_WAIT_ALL
    End
