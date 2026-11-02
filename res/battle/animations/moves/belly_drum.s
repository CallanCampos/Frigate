// MOVE_BELLY_DRUM's animation: member 187 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant BellyDrum

BellyDrum:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W187_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W187_01, 2, EFFVM_ATTACKER, 10, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W187_01, 1, EFFVM_ATTACKER, 20, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W187_01, 2, EFFVM_ATTACKER, 30, 0, 115, 0, 0, 0
    SetYield TRUE
    ParticleLoad 357
    EmitterAdd 357, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 357, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 357, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssScale EFFVM_ATTACKER, 2, 0x333, -0x333, 3, 1, 3
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
