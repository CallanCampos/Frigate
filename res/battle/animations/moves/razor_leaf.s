// MOVE_RAZOR_LEAF's animation: member 75 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant RazorLeaf

RazorLeaf:
    CameraMovePreset 1, 9, 16, 0, 8
    McssSetValue EFFVM_ALL, 1
    Wait EFFVM_WAIT_PARTICLES
    ParticleLoad 242
    SetYield FALSE
    PlaySE SEQ_SE_W077_03, 1, EFFVM_ATTACKER, 0, 500, 115, 0, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W075_06, 1, EFFVM_DEFENDER, 70, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W075_06, 2, EFFVM_DEFENDER, 85, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W075_06, 1, EFFVM_DEFENDER, 90, 0, 100, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 65, 20, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 80, 20, 0, 0
    SetYield TRUE
    EmitterAdd 242, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    EmitterMove 242, 1, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    WaitFrames 90
    CameraMovePreset 1, 11, 10, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W010_01, 2, EFFVM_DEFENDER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W010_01, 2, EFFVM_DEFENDER, 10, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W010_01, 2, EFFVM_DEFENDER, 20, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W010_01, 2, EFFVM_DEFENDER, 30, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W075_02, 1, EFFVM_DEFENDER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 55, 0, 0, 0, 0, 0
    SetYield TRUE
    WaitFrames 6
    EmitterAdd 242, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 2, 1, 4
    McssScale EFFVM_DEFENDER, 2, 0x19a, -0x19a, 2, 1, 4
    Wait EFFVM_WAIT_ALL
    McssSetValue EFFVM_ALL, 0
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
