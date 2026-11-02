// MOVE_HEAT_CRASH's animation: member 535 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant HeatCrash

HeatCrash:
    McssSetValue EFFVM_ALL, 1
    ParticleLoad 704
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W307_03, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W310_01, 2, EFFVM_DEFENDER, 15, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W464_01, 0, EFFVM_DEFENDER, 30, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W157_01, 1, EFFVM_DEFENDER, 30, 0, 127, 0, 0, 0
    MoveSE 1, 0, 1, 127, 0, 80, 20, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 100, 0, 0, 0, 0, 0
    SetYield TRUE
    EmitterAdd 704, 1, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 704, 2, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x99a, 0x1000
    EmitterAdd 704, 3, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 704, 4, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    McssScale EFFVM_DEFENDER, 2, 0x800, -0x800, 4, 1, 1
    EmitterAdd 704, 0, EFFVM_EMIT_DEFENDER, 8, 0x1800, 0, 0, 0x1000, 0x800, 0x1000, 0x1000
    CameraShake 0, 0x333, 0, 2, 0, 8
    McssPalFade EFFVM_DEFENDER, 0, 10, 2, 0x1f
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 4, 0, 8
    Wait EFFVM_WAIT_ALL
    McssSetValue EFFVM_ALL, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssPalFade EFFVM_DEFENDER, 10, 0, 2, 0x1f
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
