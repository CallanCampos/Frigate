// MOVE_TRIPLE_KICK's animation: member 167 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant TripleKick

TripleKick:
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_015, 0, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 337
    EmitterAdd 337, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 12288, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 2
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 0, 200, 100, 0, 0, 0
    SetYield TRUE
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 2, 2, 1
    McssSetVanish EFFVM_ATTACKER, TRUE
    EmitterAdd 337, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 337, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 12288, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
