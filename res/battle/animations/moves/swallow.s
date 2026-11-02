// MOVE_SWALLOW's animation: member 256 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Swallow

Swallow:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W254_04, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W307_03, 1, EFFVM_ATTACKER, 80, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W113_02, 2, EFFVM_ATTACKER, 90, 0, 40, 0, 0, 0
    PlaySE SEQ_SE_W113_02, 1, EFFVM_ATTACKER, 107, 0, 20, 0, 0, 0
    SetYield TRUE
    McssScale EFFVM_ATTACKER, 2, -0x4cd, 0x4cd, 6, 2, 1
    ParticleLoad 426
    EmitterAdd 426, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 40
    EmitterAdd 426, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 426, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 40
    McssPalFade EFFVM_ATTACKER, 0, 12, 2, 0x7fff
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 12, 0, 2, 0x7fff
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
