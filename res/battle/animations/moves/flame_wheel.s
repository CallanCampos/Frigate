// MOVE_FLAME_WHEEL's animation: member 172 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant FlameWheel

FlameWheel:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    ParticleLoad 342
    EmitterAdd 342, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W307_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W464_01, 2, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 55, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W464_01, 2, EFFVM_ATTACKER, 30, -200, 127, 0, 0, 0
    MoveSE 1, 0, 1, 115, 0, 35, 20, 0, 0
    MoveSE 2, 0, 1, 90, 0, 35, 20, 0, 0
    SetYield TRUE
    WaitFrames 40
    McssMoveXY EFFVM_ATTACKER, 2, 0x2000, 0x1000, 4, 2, 1
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 12
    SetYield FALSE
    PlaySE SEQ_SE_W307_02, 2, EFFVM_DEFENDER, 0, 0, 80, 0, 0, 0
    PlaySE SEQ_SE_W307_EXP, 1, EFFVM_DEFENDER, 0, -200, 127, 0, 0, 0
    SetYield TRUE
    EmitterAdd 342, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 342, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 1, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
