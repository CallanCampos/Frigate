// MOVE_SHARPEN's animation: member 159 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Sharpen

Sharpen:
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W095_01, 1, EFFVM_ATTACKER, 0, 0, 80, 0, 0, 0
    PlaySE SEQ_SE_W095_01, 2, EFFVM_ATTACKER, 30, 100, 80, 0, 0, 0
    PlaySE SEQ_SE_W095_01, 1, EFFVM_ATTACKER, 60, 200, 80, 0, 0, 0
    PlaySE SEQ_SE_W100_01, 2, EFFVM_ATTACKER, 85, 100, 110, 0, 0, 0
    PlaySE SEQ_SE_W100_01, 1, EFFVM_ATTACKER, 95, 100, 60, 0, 0, 0
    SetYield TRUE
    ParticleLoad 327
    EmitterAdd 327, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 327, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_PARTICLES
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
