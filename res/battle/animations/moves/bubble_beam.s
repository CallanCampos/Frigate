// MOVE_BUBBLE_BEAM's animation: member 61 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant BubbleBeam

BubbleBeam:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W124_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 50, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W061_01, 1, EFFVM_DEFENDER, 60, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W061_01, 2, EFFVM_DEFENDER, 70, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W061_01, 1, EFFVM_DEFENDER, 80, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W061_01, 2, EFFVM_DEFENDER, 90, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W061_01, 1, EFFVM_DEFENDER, 100, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W061_01, 2, EFFVM_DEFENDER, 110, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 226
    EmitterAdd 226, 0, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 226, 1, EFFVM_EMIT_DEFENDER, 8, 0x2800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    CameraMovePreset 1, 11, 16, 0, 12
    WaitFrames 40
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 4
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
