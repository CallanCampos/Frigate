// MOVE_SNARL's animation: member 555 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Snarl

Snarl:
    McssSetValue EFFVM_ALL, 1
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W103_01, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W054_01, 2, EFFVM_ATTACKER, 0, 1000, 127, 0, 0, 0
    MoveSE 2, 0, 0, 1000, 1200, 0, 80, 0, 0
    MoveSE 2, 0, 1, 127, 0, 60, 20, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 80, 0, 0, 0, 0, 0
    SetYield TRUE
    ParticleLoad 732
    EmitterAdd 732, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x800, 0x1000, 0x1000, 0x1000
    EmitterAdd 732, 5, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x800, 0x1000
    EmitterAdd 732, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0x800, 0x1000
    EmitterAdd 732, 3, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0x800, 0x1000
    EmitterAdd 732, 0, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0x800, 0x1000
    EmitterAdd 732, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_ATTACKER, 2, 0x333, -0x333, 20, 1, 1
    McssMoveXY EFFVM_ATTACKER, 2, 0x4cd, 0, 3, 0, 10
    WaitFrames 40
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 2, 1, 4
    CameraMovePreset 1, 11, 12, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 10, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 15, -500, 100, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 20, -200, 127, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 12, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetValue EFFVM_ALL, 0
    Wait EFFVM_WAIT_ALL
    End
