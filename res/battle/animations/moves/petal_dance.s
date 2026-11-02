// MOVE_PETAL_DANCE's animation: member 80 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant PetalDance

PetalDance:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 248
    McssCircle EFFVM_ATTACKER, 2, 3, 0x1000, 0x1000, 0xa000, 0x1000, 0x3000, 0
    SetYield FALSE
    PlaySE SEQ_SE_W027_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 60, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 115, 0, 40, 20, 0, 0
    SetYield TRUE
    EmitterAdd 248, 0, EFFVM_EMIT_ATTACKER, 8, 0x333, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 248, 1, EFFVM_EMIT_ATTACKER, 8, 0x333, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 32
    McssSetVanish EFFVM_DEFENDER, TRUE
    CameraMovePreset 1, 11, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W239_01, 2, EFFVM_DEFENDER, 10, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 80, 0, 0, 0, 0, 0
    MoveSE 2, 0, 0, -1000, 2500, 0, 80, 0, 0
    MoveSE 2, 0, 1, 120, 0, 60, 20, 0, 0
    SetYield TRUE
    EmitterAdd 248, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 248, 1, EFFVM_EMIT_DEFENDER, 8, 0x333, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 12
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
