// MOVE_SWORDS_DANCE's animation: member 14 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant SwordsDance

SwordsDance:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W014_01, 2, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W014_02, 1, EFFVM_ATTACKER, 12, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W014_03, 2, EFFVM_ATTACKER, 23, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W014_03, 2, EFFVM_ATTACKER, 55, 0, 127, 0, 0, 0
    SetYield TRUE
    ParticleLoad 179
    EmitterAdd 179, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 179, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
