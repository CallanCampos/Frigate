// MOVE_AIR_SLASH's animation: member 403 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant AirSlash

AirSlash:
    CameraMovePreset 1, 11, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W464_01, 2, EFFVM_DEFENDER, 0, 500, 100, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 30, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W010_01, 2, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 10, 300, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 582
    EmitterAdd 582, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 582, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 582, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 2, 1, 6
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
