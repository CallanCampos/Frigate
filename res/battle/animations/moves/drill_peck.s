// MOVE_DRILL_PECK's animation: member 65 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant DrillPeck

DrillPeck:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssCircle EFFVM_ATTACKER, 2, 0, 0, 0x2000, 0xa000, 0x1000, 0x1000, 0
    SetYield FALSE
    PlaySE SEQ_SE_W216_01, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, TRUE
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    McssMoveXY EFFVM_ATTACKER, 2, 0x333, 0, 2, 1, 8
    ParticleLoad 230
    EmitterAdd 230, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 230, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 230, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x19a, -0x19a, 2, 1, 8
    SetYield FALSE
    PlaySE SEQ_SE_W065_DODODO, 2, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    MoveSE 2, 0, 1, 127, 0, 50, 10, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 60, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W221_NOISE, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    MoveSE 1, 0, 1, 127, 0, 50, 10, 0, 0
    MoveSE 1, 0, 0, 0, 1000, 0, 60, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 60, 0, 127, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
