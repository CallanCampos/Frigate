// MOVE_REST's animation: member 156 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Rest

Rest:
    CameraMovePreset 1, 9, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W156_BABBLE2, 1, EFFVM_ATTACKER, 20, -1000, 127, 0, 0, 0
    PlaySE SEQ_SE_W156_BABBLE2, 1, EFFVM_ATTACKER, 70, -1000, 127, 0, 0, 0
    PlaySE SEQ_SE_W156_BABBLE2, 1, EFFVM_ATTACKER, 100, -1000, 127, 0, 0, 0
    PlaySE SEQ_SE_W095_02, 2, EFFVM_ATTACKER, 20, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W095_02, 2, EFFVM_ATTACKER, 85, 0, 110, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    ParticleLoad 324
    EmitterAdd 324, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 324, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    EmitterAdd 324, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    EmitterAdd 324, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 324, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_ATTACKER, TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
