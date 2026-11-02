// MOVE_FLAIL's animation: member 175 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Flail

Flail:
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 20, 24, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W175_01, 3, EFFVM_ATTACKER, 0, 0, 70, 0, 0, 0
    PlaySE SEQ_SE_W175_01, 3, EFFVM_ATTACKER, 40, 0, 70, 0, 0, 0
    SetYield TRUE
    ParticleLoad 346
    EmitterAdd 346, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssMoveXY EFFVM_ATTACKER, 2, 0x2000, 0, 4, 1, 4
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 16, -300, 120, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    CameraMovePreset 1, 11, 16, 0, 12
    WaitFrames 16
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 1, 1
    EmitterAdd 346, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
