// MOVE_SUPERSONIC's animation: member 48 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Supersonic

Supersonic:
    CameraMovePreset 1, 20, 24, 0, 0
    PaletteFade 4, 0, 12, 1, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W048_01, 1, EFFVM_DEFENDER, 0, 50, 110, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 70, 0, 0, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 60, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W048_02, 2, EFFVM_DEFENDER, 70, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W048_02, 1, EFFVM_DEFENDER, 90, 0, 60, 0, 0, 0
    SetYield TRUE
    ParticleLoad 213
    EmitterMove 213, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0, 0x1000, 0x1000, 0
    WaitFrames 2
    EmitterMove 213, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0, 0x1000, 0x1000, 0
    WaitFrames 2
    EmitterMove 213, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0, 0x1000, 0x1000, 0
    WaitFrames 10
    CameraMovePreset 1, 11, 10, 0, 8
    McssSetVanish EFFVM_DEFENDER, TRUE
    EmitterAdd 213, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 2, 0, 3
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    PaletteFade 4, 12, 0, 1, 0
    Wait EFFVM_WAIT_ALL
    End
