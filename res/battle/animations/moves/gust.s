// MOVE_GUST's animation: member 16 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Gust

Gust:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W239_01, 1, EFFVM_DEFENDER, 10, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 120, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -500, 500, 10, 80, 0, 0
    MoveSE 1, 0, 1, 70, 0, 60, 60, 0, 0
    PlaySE SEQ_SE_W059_01, 2, EFFVM_DEFENDER, 10, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 120, 0, 0, 0, 0, 0
    MoveSE 2, 0, 0, 0, 1300, 0, 40, 0, 0
    MoveSE 2, 0, 0, 1300, 0, 60, 40, 0, 0
    MoveSE 2, 0, 1, 0, 100, 0, 30, 0, 0
    MoveSE 2, 0, 1, 100, 0, 60, 60, 0, 0
    SetYield TRUE
    ParticleLoad 181
    EmitterAdd 181, 0, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 181, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    EmitterAdd 181, 1, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 181, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x1000, 0, 3, 1, 6
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
