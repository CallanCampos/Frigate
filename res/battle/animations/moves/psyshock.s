// MOVE_PSYSHOCK's animation: member 473 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Psyshock

Psyshock:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_CANCEL3, 1, EFFVM_ATTACKER, 0, 0, 80, 0, 0, 0
    PlaySE SEQ_SE_CANCEL3, 1, EFFVM_ATTACKER, 6, 0, 80, 0, 0, 0
    PlaySE SEQ_SE_CANCEL3, 1, EFFVM_ATTACKER, 12, 0, 80, 0, 0, 0
    PlaySE SEQ_SE_CANCEL3, 1, EFFVM_ATTACKER, 18, 0, 80, 0, 0, 0
    SetYield TRUE
    McssMoveXY EFFVM_ATTACKER, 2, 0x4cd, 0, 2, 0, 10
    WaitFrames 20
    CameraMovePreset 1, 11, 16, 0, 8
    SetYield FALSE
    SetYield FALSE
    PlaySE SEQ_SE_FLD_127, 1, EFFVM_DEFENDER, 0, 500, 80, 0, 0, 0
    PlaySE SEQ_SE_SHINKA_W025, 1, EFFVM_DEFENDER, 25, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_SYS_24, 2, EFFVM_DEFENDER, 37, 0, 127, 0, 0, 0
    MoveSE 2, 0, 0, 0, -200, 42, 20, 0, 0
    PlaySE SEQ_SE_FLD_03, 1, EFFVM_DEFENDER, 75, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 90, 0, 127, 0, 0, 0
    SetYield TRUE
    ParticleLoad 650
    EmitterAdd 650, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    EmitterAdd 650, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    WaitFrames 10
    McssSetValue EFFVM_ALL, 1
    EmitterAdd 650, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0xccd, 0x1000, 0x800, 0x1000
    WaitFrames 45
    EmitterAdd 650, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 1, 5
    WaitFrames 60
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetValue EFFVM_ALL, 0
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
