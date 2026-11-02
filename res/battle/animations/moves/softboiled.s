// MOVE_SOFTBOILED's animation: member 135 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Softboiled

Softboiled:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 6, 1, 1
    SetYield FALSE
    PlaySE SEQ_SE_W260_01, 1, EFFVM_ATTACKER, 15, -400, 120, 0, 0, 0
    PlaySE SEQ_SE_DECIDE3, 2, EFFVM_ATTACKER, 55, 0, 110, 0, 0, 0
    SetYield TRUE
    ParticleLoad 304
    EmitterAdd 304, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 304, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 2
    EmitterAdd 304, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 304, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 304, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 6
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 10, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
