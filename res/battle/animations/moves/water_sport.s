// MOVE_WATER_SPORT's animation: member 346 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant WaterSport

WaterSport:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W051_01, 1, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W055_01, 2, EFFVM_ATTACKER, 10, 100, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 90, 100, 0, 0, 0, 0
    MoveSE 2, 0, 1, 115, 0, 70, 20, 0, 0
    SetYield TRUE
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 6, 1, 3
    ParticleLoad 523
    EmitterAddOrtho 523, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 523, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x800, 0x1000, 0x800, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 14, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W323_01, 1, EFFVM_DEFENDER, 20, -60, 120, 0, 0, 0
    PlaySE SEQ_SE_W323_01, 2, EFFVM_DEFENDER, 30, -60, 90, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 523, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 10
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 1, 4
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
