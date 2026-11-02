// MOVE_KARATE_CHOP's animation: member 2 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant KarateChop

KarateChop:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    ParticleLoad 166
    EmitterAdd 166, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 166, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_DEFENDER, 0, 0, 125, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 10, 0, 110, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
