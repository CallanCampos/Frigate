// Effect BTLV_EFFECT_FIRST + 79 (0x280): member 79 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect079

Effect079:
    CameraMovePreset 1, 9, 16, 0, 8
    ParticleLoad 2
    SetYield FALSE
    PlaySE SEQ_SE_W112_02, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DECIDE3, 2, EFFVM_ATTACKER, 10, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DECIDE3, 2, EFFVM_ATTACKER, 30, 0, 59, 0, 0, 0
    SetYield TRUE
    EmitterAdd 2, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 2, 1, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
