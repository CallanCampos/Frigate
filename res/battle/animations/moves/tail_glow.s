// MOVE_TAIL_GLOW's animation: member 294 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant TailGlow

TailGlow:
    PaletteFade 4, 0, 12, 3, 0
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W095_01, 1, EFFVM_ATTACKER, 0, 0, 80, 10, 0, 0
    PlaySE SEQ_SE_W095_01, 2, EFFVM_ATTACKER, 5, -20, 40, 10, 0, 0
    PlaySE SEQ_SE_W095_01, 2, EFFVM_ATTACKER, 35, 0, 30, 10, 0, 0
    PlaySE SEQ_SE_W095_01, 1, EFFVM_ATTACKER, 40, -20, 15, 10, 0, 0
    PlaySE SEQ_SE_W095_01, 1, EFFVM_ATTACKER, 63, 0, 10, 10, 0, 0
    PlaySE SEQ_SE_W095_01, 2, EFFVM_ATTACKER, 68, -20, 5, 10, 0, 0
    SetYield TRUE
    ParticleLoad 464
    EmitterAdd 464, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 12, 0, 3, 0
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
