// MOVE_ROCK_POLISH's animation: member 397 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant RockPolish

RockPolish:
    CameraMovePreset 1, 9, 16, 0, 8
    PaletteFade 4, 0, 12, 0, 0
    PlaySE SEQ_SE_W075_02, 2, EFFVM_ATTACKER, 15, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DECIDE3, 2, EFFVM_ATTACKER, 55, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W043_01, 1, EFFVM_ATTACKER, 55, -700, 127, 0, 0, 0
    ParticleLoad 576
    EmitterAdd 576, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 576, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    PaletteFade 4, 12, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
