// MOVE_STOCKPILE's animation: member 254 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Stockpile

Stockpile:
    McssSetValue EFFVM_ALL, 1
    CameraMovePreset 1, 9, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, TRUE
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 0, 12, 3, 0x7fff
    ParticleLoad 424
    EmitterAdd 424, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x800, 0x1000
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 8, 1, 2
    SetYield FALSE
    PlaySE SEQ_SE_W112_01, 1, EFFVM_ATTACKER, 0, -100, 30, 0, 0, 0
    PlaySE SEQ_SE_W254_04, 2, EFFVM_ATTACKER, 0, -50, 120, 0, 0, 0
    PlaySE SEQ_SE_W254_04, 2, EFFVM_ATTACKER, 20, 0, 120, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 12, 0, 8
    McssPalFade EFFVM_ATTACKER, 12, 0, 3, 0x7fff
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetValue EFFVM_ALL, 0
    Wait EFFVM_WAIT_ALL
    End
