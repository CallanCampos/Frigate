// MOVE_SKETCH's animation: member 166 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Sketch

Sketch:
    CameraMovePreset 1, 11, 16, 0, 12
    McssSetValue EFFVM_DEFENDER, 1
    McssSetVanish EFFVM_ATTACKER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W075_03, 1, EFFVM_DEFENDER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY, 1, EFFVM_DEFENDER, 120, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W215_02, 2, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W215_02, 2, EFFVM_DEFENDER, 40, 200, 127, 0, 0, 0
    PlaySE SEQ_SE_W215_02, 2, EFFVM_DEFENDER, 80, 400, 127, 0, 0, 0
    MoveSE 2, 0, 1, 127, 0, 80, 50, 0, 0
    SetYield TRUE
    ParticleLoad 336
    EmitterAdd 336, 0, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 336, 1, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssAlpha EFFVM_DEFENDER, 0, 2, 1, 1, 0
    WaitFrames 50
    McssAlpha EFFVM_DEFENDER, 1, 31, 50, 1, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetValue EFFVM_DEFENDER, 0
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
