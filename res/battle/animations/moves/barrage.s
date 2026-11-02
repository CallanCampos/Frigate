// MOVE_BARRAGE's animation: member 140 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Barrage

Barrage:
    ParticleLoad 309
    EmitterMove 309, 1, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0, 0x24000, 0x5000, 0x1000, 0x1000, 0
    WaitFrames 10
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 0, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W125_01, 2, EFFVM_DEFENDER, 20, 200, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 20, 0, 50, 0, 0, 0
    SetYield TRUE
    EmitterAdd 309, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 309, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 309, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 16
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
