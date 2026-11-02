// MOVE_COPYCAT's animation: member 383 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Copycat

Copycat:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W295_01, 2, EFFVM_ATTACKER, 0, 0, 40, 0, 0, 0
    SetYield TRUE
    ParticleLoad 562
    EmitterAdd 562, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 562, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W215_02, 1, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    SetYield TRUE
    WaitFrames 30
    EmitterAdd 562, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    EmitterAdd 562, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_ATTACKER, 0, 6, 0, 0x3c80
    SetYield FALSE
    PlaySE SEQ_SE_W051_01, 2, EFFVM_ATTACKER, 0, 0, 75, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 6, 0, 0, 0x3d40
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
