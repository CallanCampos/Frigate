// MOVE_CRUSH_CLAW's animation: member 306 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant CrushClaw

CrushClaw:
    CameraMovePreset 1, 11, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W010_01, 1, EFFVM_DEFENDER, 5, 200, 120, 0, 0, 0
    PlaySE SEQ_SE_W010_01, 2, EFFVM_DEFENDER, 32, 200, 120, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 476
    ParticleLoad 477
    EmitterAdd 476, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 476, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 477, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 477, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 3
    WaitFrames 26
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 3
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
