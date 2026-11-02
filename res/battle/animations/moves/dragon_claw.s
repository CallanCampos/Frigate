// MOVE_DRAGON_CLAW's animation: member 337 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant DragonClaw

DragonClaw:
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W307_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 80, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 3000, 0, 70, 0, 0
    MoveSE 1, 0, 1, 115, 0, 60, 20, 0, 0
    SetYield TRUE
    ParticleLoad 511
    EmitterAdd 511, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 511, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_ATTACKER, 0, 12, 1, 0x1f
    McssMoveXY EFFVM_ATTACKER, 2, 0, 0x333, 1, 1, 12
    WaitFrames 40
    McssPalFade EFFVM_ATTACKER, 12, 0, 1, 0x1f
    Wait EFFVM_WAIT_ALL
    ParticleDelete 511
    CameraMovePreset 1, 11, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W010_01, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W010_01, 2, EFFVM_DEFENDER, 7, 0, 50, 0, 0, 0
    PlaySE SEQ_SE_W010_01, 1, EFFVM_DEFENDER, 14, 0, 20, 0, 0, 0
    PlaySE SEQ_SE_W010_01, 2, EFFVM_DEFENDER, 30, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W010_01, 1, EFFVM_DEFENDER, 37, 0, 50, 0, 0, 0
    PlaySE SEQ_SE_W010_01, 2, EFFVM_DEFENDER, 44, 0, 20, 0, 0, 0
    PlaySE SEQ_SE_W010_01, 1, EFFVM_DEFENDER, 51, 0, 10, 0, 0, 0
    SetYield TRUE
    ParticleLoad 512
    ParticleLoad 513
    EmitterAdd 512, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 512, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 513, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 513, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 3
    WaitFrames 26
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 3
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
