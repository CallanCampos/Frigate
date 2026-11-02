// MOVE_CLEAR_SMOG's animation: member 499 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant ClearSmog

ClearSmog:
    McssSetValue EFFVM_ALL, 1
    CameraMovePreset 1, 9, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W464_01, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W018_01, 2, EFFVM_ATTACKER, 0, 0, 80, 0, 0, 0
    MoveSE 2, 0, 1, 80, 0, 70, 10, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 80, 0, 0
    SetYield TRUE
    ParticleLoad 671
    EmitterMove 671, 1, 6, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x3c000, 0x1000, 0x1000, 0x1000, 0x4000
    Wait EFFVM_WAIT_CAMERA
    McssSetVanish EFFVM_DEFENDER, TRUE
    CameraMovePreset 1, 11, 18, 0, 8
    McssScale EFFVM_DEFENDER, 2, 0x19a, -0x19a, 2, 0, 1
    WaitFrames 20
    EmitterAdd 671, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_FLD_127, 1, EFFVM_DEFENDER, 0, -500, 127, 0, 0, 0
    SetYield TRUE
    EmitterAdd 671, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    EmitterAdd 671, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    EmitterAdd 671, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetValue EFFVM_ALL, 0
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
