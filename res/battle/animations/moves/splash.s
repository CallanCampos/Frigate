// MOVE_SPLASH's animation: member 150 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Splash

Splash:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W260_01, 1, EFFVM_ATTACKER, 18, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W051_01, 2, EFFVM_ATTACKER, 18, 0, 90, 0, 0, 0
    SetYield TRUE
    ParticleLoad 318
    McssSetVanish EFFVM_ATTACKER, TRUE
    EmitterAdd 318, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 318, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 6, 1, 1
    Wait EFFVM_WAIT_PARTICLES
    SetYield FALSE
    PlaySE SEQ_SE_W260_01, 1, EFFVM_ATTACKER, 18, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W051_01, 2, EFFVM_ATTACKER, 18, 0, 90, 0, 0, 0
    SetYield TRUE
    EmitterAdd 318, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 318, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 6, 1, 1
    Wait EFFVM_WAIT_PARTICLES
    SetYield FALSE
    PlaySE SEQ_SE_W260_01, 1, EFFVM_ATTACKER, 18, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W051_01, 2, EFFVM_ATTACKER, 18, 0, 90, 0, 0, 0
    SetYield TRUE
    EmitterAdd 318, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 318, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 6, 1, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
