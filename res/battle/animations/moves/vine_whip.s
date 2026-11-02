// MOVE_VINE_WHIP's animation: member 22 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant VineWhip

VineWhip:
    McssCircle EFFVM_ATTACKER, 3, 1, 0, 0x3000, 0xa000, 0x2000, 0x1000, 0
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 10, -100, 115, 0, 0, 0
    SetYield TRUE
    WaitFrames 10
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 10
    ParticleLoad 188
    EmitterAdd 188, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 188, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    EmitterAdd 188, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 4, 2, 1
    SetYield FALSE
    PlaySE SEQ_SE_W022_02, 1, EFFVM_DEFENDER, 0, -400, 120, 0, 0, 0
    PlaySE SEQ_SE_W022_01, 2, EFFVM_DEFENDER, 7, -200, 120, 0, 0, 0
    SetYield TRUE
    WaitFrames 20
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
