// MOVE_BEAT_UP's animation: member 251 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant BeatUp

BeatUp:
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 0, -100, 115, 0, 0, 0
    PlaySE SEQ_SE_W011_01, 2, EFFVM_DEFENDER, 5, 60, 100, 0, 0, 0
    SetYield TRUE
    ParticleLoad 421
    EmitterAdd 421, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 3, 1, 1
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
