// MOVE_WATER_PLEDGE's animation: member 518 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant WaterPledge

WaterPledge:
    McssSetValue EFFVM_ALL, 1
    ParticleLoad 688
    CameraMovePreset 1, 11, 10, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W216_01, 1, EFFVM_DEFENDER, 0, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W216_01, 1, EFFVM_DEFENDER, 40, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W216_01, 1, EFFVM_DEFENDER, 80, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W027_01, 2, EFFVM_DEFENDER, 0, -200, 127, 0, 0, 0
    PlaySE SEQ_SE_W027_01, 2, EFFVM_DEFENDER, 40, -200, 127, 0, 0, 0
    PlaySE SEQ_SE_W027_01, 2, EFFVM_DEFENDER, 80, -400, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 140, 0, 0, 0, 0, 0
    MoveSE 2, 0, 0, -400, 1000, 80, 60, 0, 0
    MoveSE 2, 0, 1, 127, 0, 70, 70, 0, 0
    SetYield TRUE
    EmitterAdd 688, 7, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 688, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x19a, 2, 1, 10
    EmitterAdd 688, 8, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 688, 2, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    EmitterAdd 688, 6, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 688, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetValue EFFVM_ALL, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
