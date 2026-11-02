// MOVE_TAILWIND's animation: member 366 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Tailwind

Tailwind:
    CameraMovePreset 1, 21, 16, 0, 12
    ParticleLoad 544
    SetYield FALSE
    PlaySE SEQ_SE_W239_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 110, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 0, 1000, 0, 110, 0, 0
    MoveSE 1, 0, 1, 115, 0, 90, 20, 0, 0
    SetYield TRUE
    EmitterAdd 544, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 544, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
