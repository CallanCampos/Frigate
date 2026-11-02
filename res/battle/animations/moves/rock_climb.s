// MOVE_ROCK_CLIMB's animation: member 431 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant RockClimb

RockClimb:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W157_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 40, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 120, 0, 10, 30, 0, 0
    SetYield TRUE
    ParticleLoad 611
    EmitterAddOrtho 611, 3, EFFVM_EMIT_ATTACKER, 8, 0, 0x1000, 0, 0x800, 0xc00, 0x800, 0xc00
    EmitterAddOrtho 611, 4, EFFVM_EMIT_ATTACKER, 8, 0, 0x1000, 0, 0x800, 0xc00, 0x800, 0xc00
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 2, 1, 1
    Wait EFFVM_WAIT_ALL
    Wait EFFVM_WAIT_SE
    SetYield FALSE
    PlaySE SEQ_SE_W157_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 40, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 120, 0, 10, 30, 0, 0
    SetYield TRUE
    EmitterAddOrtho 611, 3, EFFVM_EMIT_ATTACKER, 8, 0, 0x1000, 0, 0x800, 0xc00, 0x800, 0xc00
    EmitterAddOrtho 611, 4, EFFVM_EMIT_ATTACKER, 8, 0, 0x1000, 0, 0x800, 0xc00, 0x800, 0xc00
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 2, 1, 1
    Wait EFFVM_WAIT_ALL
    Wait EFFVM_WAIT_SE
    SetYield FALSE
    PlaySE SEQ_SE_W157_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 40, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 120, 0, 10, 30, 0, 0
    SetYield TRUE
    EmitterAddOrtho 611, 3, EFFVM_EMIT_ATTACKER, 8, 0, 0x1000, 0, 0x800, 0xc00, 0x800, 0xc00
    EmitterAddOrtho 611, 4, EFFVM_EMIT_ATTACKER, 8, 0, 0x1000, 0, 0x800, 0xc00, 0x800, 0xc00
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 2, 1, 1
    Wait EFFVM_WAIT_ALL
    Wait EFFVM_WAIT_SE
    McssMoveXY EFFVM_ATTACKER, 2, 0x1000, 0, 2, 2, 1
    SetYield FALSE
    PlaySE SEQ_SE_W015_01, 1, EFFVM_ATTACKER, 0, -1000, 120, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 20, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 16, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 1, 1, 4
    EmitterAddOrtho 611, 2, EFFVM_EMIT_DEFENDER, 8, 0, 0x1000, 0, 0x800, 0x800, 0x800, 0x1000
    EmitterAddOrtho 611, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x800, 0x800, 0x1000
    EmitterAddOrtho 611, 2, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x800, 0x800, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 0, -1000, 110, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    Wait EFFVM_WAIT_SE
    CameraMovePreset 1, 17, 24, 0, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
