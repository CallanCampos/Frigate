// MOVE_BUG_BITE's animation: member 450 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant BugBite

BugBite:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W044_02, 1, EFFVM_DEFENDER, 0, -30, 127, 0, 0, 0
    PlaySE SEQ_SE_W044_02, 1, EFFVM_DEFENDER, 5, 10, 127, 0, 0, 0
    PlaySE SEQ_SE_W044_02, 1, EFFVM_DEFENDER, 10, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W044_02, 1, EFFVM_DEFENDER, 15, -10, 127, 0, 0, 0
    PlaySE SEQ_SE_W044_02, 1, EFFVM_DEFENDER, 20, 10, 127, 0, 0, 0
    PlaySE SEQ_SE_W044_02, 1, EFFVM_DEFENDER, 25, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W044_02, 1, EFFVM_DEFENDER, 30, -20, 127, 0, 0, 0
    PlaySE SEQ_SE_W044_02, 1, EFFVM_DEFENDER, 35, -10, 127, 0, 0, 0
    PlaySE SEQ_SE_W075_02, 2, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 40, 0, 0, 0, 0, 0
    SetYield TRUE
    ParticleLoad 629
    EmitterAdd 629, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 629, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 2, 1, 5
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
