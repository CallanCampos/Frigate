// MOVE_YAWN's animation: member 281 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Yawn

Yawn:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W281_AKUBI, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    SetYield TRUE
    McssScale EFFVM_ATTACKER, 2, -0x333, 0x333, 8, 1, 1
    ParticleLoad 452
    EmitterMove 452, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    McssSetVanish EFFVM_DEFENDER, TRUE
    Wait EFFVM_WAIT_PARTICLES
    CameraMovePreset 1, 11, 16, 0, 8
    EmitterAdd 452, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 3, 1, 1
    SetYield FALSE
    PlaySE SEQ_SE_W095_01, 2, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W095_01, 1, EFFVM_DEFENDER, 8, 0, 80, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
