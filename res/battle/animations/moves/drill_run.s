// MOVE_DRILL_RUN's animation: member 529 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant DrillRun

DrillRun:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W216_01, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 30, 0, 0, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 30, 0, 0
    SetYield FALSE
    CameraMovePreset 1, 11, 26, 0, 15
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 698
    EmitterMove 698, 2, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0, 0x1000, 0x1000, 0
    EmitterAdd 698, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x99a, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 8
    WaitFrames 15
    SetYield FALSE
    PlaySE SEQ_SE_FLD_03, 2, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_ROTATION_B, 1, EFFVM_DEFENDER, 20, -500, 127, 0, 0, 0
    SetYield TRUE
    EmitterAdd 698, 5, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0xb33, 0x1000, 0x1000
    EmitterAdd 698, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0xb33, 0x1000, 0x1000
    EmitterAdd 698, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0xb33, 0x1000, 0x1000
    WaitFrames 14
    McssScale EFFVM_DEFENDER, 2, 0x666, -0x666, 4, 0, 1
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
