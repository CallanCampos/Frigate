// MOVE_ASSURANCE's animation: member 372 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Assurance

Assurance:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_DEFENDER, 0, -400, 110, 0, 0, 0
    PlaySE SEQ_SE_W003_01, 1, EFFVM_DEFENDER, 10, -100, 120, 0, 0, 0
    PlaySE SEQ_SE_W011_01, 2, EFFVM_DEFENDER, 10, 0, 70, 0, 0, 0
    PlaySE SEQ_SE_W003_01, 2, EFFVM_DEFENDER, 20, -100, 120, 0, 0, 0
    PlaySE SEQ_SE_W011_01, 1, EFFVM_DEFENDER, 20, 0, 70, 0, 0, 0
    SetYield TRUE
    ParticleLoad 551
    EmitterAdd 551, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 551, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 551, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 551, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 1, 1, 6
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
