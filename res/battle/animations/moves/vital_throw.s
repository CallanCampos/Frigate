// MOVE_VITAL_THROW's animation: member 233 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant VitalThrow

VitalThrow:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 0, -500, 120, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 35, -500, 120, 0, 0, 0
    SetYield TRUE
    McssCircle EFFVM_ATTACKER, 3, 3, 0x1000, 0x1000, 0x1e000, 0x1000, 0x1000, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    SetYield TRUE
    McssMoveXY EFFVM_ATTACKER, 2, 0x1000, 0, 3, 1, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 16, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 0, -200, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 10, 0, 120, 0, 0, 0
    PanSE 1, 0, EFFVM_DEFENDER, EFFVM_DEFENDER, 0, 40, 0, 0
    SetYield TRUE
    ParticleLoad 404
    EmitterAddOrtho 404, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x1000, 0x1000
    EmitterAddOrtho 404, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 2, 1, 1
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
