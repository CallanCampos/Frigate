// MOVE_HEADBUTT's animation: member 29 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Headbutt

Headbutt:
    McssCircle EFFVM_ATTACKER, 2, 0, 0, 0x3000, 0x10000, 0, 0x1000, 0
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 0, -200, 115, 0, 0, 0
    SetYield TRUE
    WaitFrames 4
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 6
    ParticleLoad 195
    EmitterAdd 195, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 195, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 0, 0, 90, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 30, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 90, 0, 20, 10, 0, 0
    PlaySE SEQ_SE_W023_01, 2, EFFVM_DEFENDER, 0, -300, 120, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
