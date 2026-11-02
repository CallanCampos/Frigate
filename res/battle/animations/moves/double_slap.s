// MOVE_DOUBLE_SLAP's animation: member 3 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant DoubleSlap

DoubleSlap:
    Wait EFFVM_WAIT_ALL
    ParticleLoad 167
    EmitterAdd 167, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 167, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 167, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 167, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x19a, -0x19a, 2, 6, 2
    SetYield FALSE
    PlaySE SEQ_SE_W003_01, 1, EFFVM_DEFENDER, 0, 0, 125, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 0, 0, 35, 0, 0, 0
    PlaySE SEQ_SE_W003_01, 1, EFFVM_DEFENDER, 25, 0, 125, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 25, 0, 35, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
