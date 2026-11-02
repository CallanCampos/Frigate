// MOVE_MAGICAL_LEAF's animation: member 345 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant MagicalLeaf

MagicalLeaf:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W075_04, 1, EFFVM_ATTACKER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W075_03, 2, EFFVM_ATTACKER, 10, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W075_05, 1, EFFVM_ATTACKER, 20, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 70, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 100, 0, 30, 40, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W075_06, 1, EFFVM_DEFENDER, 70, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W075_06, 2, EFFVM_DEFENDER, 85, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W075_06, 1, EFFVM_DEFENDER, 90, 0, 100, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 65, 10, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 65, 10, 0, 0
    SetYield TRUE
    ParticleLoad 522
    EmitterMove 522, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    EmitterAdd 522, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 80
    CameraMovePreset 1, 11, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 5, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 10, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 15, 0, 110, 0, 0, 0
    SetYield TRUE
    EmitterAdd 522, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 2, 1, 4
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
