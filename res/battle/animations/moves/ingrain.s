// MOVE_INGRAIN's animation: member 275 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Ingrain

Ingrain:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W020_01, 1, EFFVM_ATTACKER, 10, -2000, 127, 0, 0, 0
    PlaySE SEQ_SE_W020_01, 2, EFFVM_ATTACKER, 20, -500, 120, 0, 0, 0
    PlaySE SEQ_SE_W020_01, 1, EFFVM_ATTACKER, 30, -100, 120, 0, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W275_PIRA, 1, EFFVM_ATTACKER, 60, -200, 80, 0, 0, 0
    PlaySE SEQ_SE_W275_PIRA, 2, EFFVM_ATTACKER, 80, 0, 50, 0, 0, 0
    PlaySE SEQ_SE_W275_PIRA, 1, EFFVM_ATTACKER, 102, 0, 8, 0, 0, 0
    SetYield TRUE
    ParticleLoad 444
    ParticleLoad 445
    EmitterAdd 444, 0, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 444, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 444, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 445, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 445, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
