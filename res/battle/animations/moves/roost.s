// MOVE_ROOST's animation: member 355 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Roost

Roost:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W017_01, 1, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W017_01, 2, EFFVM_ATTACKER, 5, 0, 70, 0, 0, 0
    PlaySE SEQ_SE_W017_01, 1, EFFVM_ATTACKER, 10, 0, 40, 0, 0, 0
    ParticleLoad 532
    EmitterAdd 532, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    PlaySE SEQ_SE_W129_04, 2, EFFVM_ATTACKER, 0, 0, 90, 0, 0, 0
    McssPalFade EFFVM_ATTACKER, 0, 13, 2, 0x7fff
    WaitFrames 30
    McssPalFade EFFVM_ATTACKER, 13, 0, 2, 0x7fff
    EmitterAdd 532, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    PlaySE SEQ_SE_DECIDE3, 2, EFFVM_ATTACKER, 60, 0, 115, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
