// MOVE_BIND's animation: member 20 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Bind

Bind:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 4, 1, 3
    ParticleLoad 186
    McssSetVanish EFFVM_ATTACKER, TRUE
    EmitterAdd 186, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 186, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, -0x666, 0x800, 4, 1, 3
    SetYield FALSE
    PlaySE SEQ_SE_W020_01, 1, EFFVM_DEFENDER, 0, 0, 125, 0, 0, 0
    PlaySE SEQ_SE_W020_01, 2, EFFVM_DEFENDER, 15, 0, 125, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
