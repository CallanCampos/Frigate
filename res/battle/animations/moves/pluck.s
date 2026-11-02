// MOVE_PLUCK's animation: member 365 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Pluck

Pluck:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W157_04, 1, EFFVM_DEFENDER, 0, 400, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 90, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W331_01, 2, EFFVM_DEFENDER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W331_01, 2, EFFVM_DEFENDER, 5, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W331_01, 2, EFFVM_DEFENDER, 10, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W331_01, 2, EFFVM_DEFENDER, 15, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 15, -300, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 543
    EmitterAdd 543, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 543, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 543, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x333, 0, 1, 1, 6
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
