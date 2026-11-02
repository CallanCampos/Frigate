// MOVE_TORMENT's animation: member 259 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Torment

Torment:
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W259_BOSO, 1, EFFVM_ATTACKER, 0, -1000, 50, 0, 0, 0
    PlaySE SEQ_SE_W259_BOSO, 2, EFFVM_ATTACKER, 15, -1000, 50, 0, 0, 0
    PlaySE SEQ_SE_W259_BOSO, 1, EFFVM_ATTACKER, 30, -1000, 50, 0, 0, 0
    PlaySE SEQ_SE_W259_BOSO, 2, EFFVM_ATTACKER, 45, -1000, 50, 0, 0, 0
    SetYield TRUE
    ParticleLoad 429
    EmitterAdd 429, 5, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 429, 6, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 429, 7, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 429, 8, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_ATTACKER, 2, 0, 0x4cd, 3, 1, 4
    WaitFrames 10
    EmitterAdd 429, 0, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 429, 1, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 429, 2, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 429, 3, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_PARTICLES
    CameraMovePreset 1, 11, 16, 0, 12
    WaitFrames 20
    SetYield FALSE
    PlaySE SEQ_SE_W259_PAKIP, 1, EFFVM_DEFENDER, 0, -150, 120, 0, 0, 0
    PlaySE SEQ_SE_W259_PAKIP, 1, EFFVM_DEFENDER, 18, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W259_PAKIP, 1, EFFVM_DEFENDER, 30, 100, 127, 0, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_DEFENDER, 0, 12, 1, 0x16
    EmitterAdd 429, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssPalFade EFFVM_DEFENDER, 12, 0, 1, 0x16
    McssMoveXY EFFVM_DEFENDER, 2, 0, 0x4cd, 1, 2, 3
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
