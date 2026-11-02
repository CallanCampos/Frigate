// MOVE_SIGNAL_BEAM's animation: member 324 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant SignalBeam

SignalBeam:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    ParticleLoad 498
    SetYield FALSE
    PlaySE SEQ_SE_W054_01, 1, EFFVM_ATTACKER, 0, 0, 70, 0, 0, 0
    PlaySE SEQ_SE_W048_01, 2, EFFVM_ATTACKER, 0, 0, 70, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 100, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 110, 0, 70, 30, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 40, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 40, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_FLD_73, 1, EFFVM_DEFENDER, 40, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_73, 1, EFFVM_DEFENDER, 60, 0, 127, 0, 0, 0
    SetYield TRUE
    EmitterMove 498, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x733, 0x1000, 0
    CameraMovePreset 1, 11, 16, 0, 6
    WaitFrames 20
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 2, 0, 16
    McssPalFade EFFVM_DEFENDER, 0, 12, 0, 0x1e
    WaitFrames 10
    McssPalFade EFFVM_DEFENDER, 12, 0, 0, 0x1e
    WaitFrames 10
    McssPalFade EFFVM_DEFENDER, 0, 12, 0, 0x3c0
    WaitFrames 10
    McssPalFade EFFVM_DEFENDER, 12, 0, 0, 0x3c0
    WaitFrames 10
    McssPalFade EFFVM_DEFENDER, 0, 12, 0, 0x3de
    WaitFrames 10
    McssPalFade EFFVM_DEFENDER, 12, 0, 0, 0x3de
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 10, 0, 6
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
