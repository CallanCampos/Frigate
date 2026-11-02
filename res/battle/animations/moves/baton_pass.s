// MOVE_BATON_PASS's animation: member 226 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant BatonPass

BatonPass:
    CameraMovePreset 1, 9, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W075_01, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    MoveSE 1, 0, 0, 0, 500, 0, 39, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_ATTACKER, 45, 500, 127, 0, 0, 0
    SetYield TRUE
    McssScale EFFVM_ATTACKER, 2, -0x333, 0x333, 10, 0, 1
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 396
    EmitterAdd 396, 1, EFFVM_EMIT_ATTACKER, 8, 0x2800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 396, 0, EFFVM_EMIT_ATTACKER, 8, 0x2800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 396, 2, EFFVM_EMIT_ATTACKER, 8, 0x2800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 396, 4, EFFVM_EMIT_ATTACKER, 8, 0x2800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    McssScale EFFVM_ATTACKER, 2, 0x333, -0x333, 10, 0, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
