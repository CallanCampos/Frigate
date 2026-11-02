// MOVE_MIMIC's animation: member 102 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Mimic

Mimic:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    ParticleLoad 273
    EmitterMove 273, 1, 1, EFFVM_EMIT_DEFENDER, EFFVM_EMIT_ATTACKER, 0x2000, 0x28000, 0, 0x1000, 0x1000, 0
    SetYield FALSE
    PlaySE SEQ_SE_W102_02, 1, EFFVM_ATTACKER, 0, 250, 115, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 25, 0, 0
    SetYield TRUE
    WaitFrames 16
    CameraMovePreset 1, 9, 20, 0, 8
    McssSetVanish EFFVM_DEFENDER, TRUE
    Wait EFFVM_WAIT_ALL
    EmitterAdd 273, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W102_01, 2, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
