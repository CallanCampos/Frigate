// MOVE_STRUGGLE_BUG's animation: member 522 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant StruggleBug

StruggleBug:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W255_MOMOMO, 3, EFFVM_ATTACKER, 0, 0, 30, 0, 0, 0
    PlaySE SEQ_SE_W216_01, 1, EFFVM_ATTACKER, 30, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 50, 0, 0, 0, 0, 0
    SetYield TRUE
    McssMoveXY EFFVM_ATTACKER, 2, 0x4cd, 0, 2, 0, 6
    WaitFrames 10
    McssScale EFFVM_ATTACKER, 2, 0x333, -0x333, 8, 1, 1
    ParticleLoad 693
    EmitterAdd 693, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 693, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    CameraMovePreset 1, 9, 16, 0, 8
    WaitFrames 20
    CameraMovePreset 1, 11, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, TRUE
    ParticleLoad 425
    EmitterAdd 425, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 5, 0, 90, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 10, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_023, 1, EFFVM_DEFENDER, 0, 300, 127, 0, 0, 0
    SetYield TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 2, 0, 6
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 10, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
