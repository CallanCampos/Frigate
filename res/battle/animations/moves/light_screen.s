// MOVE_LIGHT_SCREEN's animation: member 113 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant LightScreen

LightScreen:
    CameraMovePreset 1, 21, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    ParticleLoad 282
    ParticleLoad 283
    EmitterAdd 282, 1, EFFVM_EMIT_ATTACKER, 8, 0x1800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 282, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W113_01, 1, EFFVM_ATTACKER, 0, -50, 127, 0, 0, 0
    SetYield TRUE
    WaitFrames 10
    EmitterAdd 283, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W112_01, 1, EFFVM_ATTACKER, 50, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W112_02, 2, EFFVM_ATTACKER, 70, 0, 90, 0, 0, 0
    SetYield TRUE
    WaitFrames 60
    EmitterAdd 282, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
