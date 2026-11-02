// MOVE_TWISTER's animation: member 239 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Twister

Twister:
    CameraMovePreset 1, 11, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W059_01, 1, EFFVM_DEFENDER, 0, 0, 100, 0, 0, 0
    MoveSE 1, 0, 0, 0, 2000, 10, 100, 0, 0
    MoveSE 1, 0, 1, 70, 0, 50, 100, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 150, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W239_01, 2, EFFVM_DEFENDER, 0, 300, 60, 0, 0, 0
    MoveSE 2, 0, 0, 0, 2000, 10, 100, 0, 0
    MoveSE 2, 0, 1, 60, 0, 50, 100, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 150, 0, 0, 0, 0, 0
    SetYield TRUE
    ParticleLoad 410
    EmitterAdd 410, 0, EFFVM_EMIT_DEFENDER, 8, 0x800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 410, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 40
    McssMoveXY EFFVM_DEFENDER, 2, 0x1000, 0, 3, 1, 4
    McssSetVanish EFFVM_DEFENDER, TRUE
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
