// MOVE_FRUSTRATION's animation: member 218 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Frustration

Frustration:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 0, 12, 2, 0x1f
    SetYield FALSE
    PlaySE SEQ_SE_W099_01, 1, EFFVM_ATTACKER, 0, 2000, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 10, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 4000, 0, 0, 10, 0, 0
    PlaySE SEQ_SE_W099_01, 1, EFFVM_ATTACKER, 40, 2000, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 50, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 4000, 0, 40, 10, 0, 0
    PlaySE SEQ_SE_W018_02, 0, EFFVM_ATTACKER, 0, 200, 110, 0, 0, 0
    PlaySE SEQ_SE_W018_02, 0, EFFVM_ATTACKER, 40, 200, 110, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_ATTACKER, 0, 135, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 12, 0, 2, 0x1f
    CameraMovePreset 1, 16, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssMoveXY EFFVM_ATTACKER, 2, 0x1000, 0x800, 2, 1, 4
    McssScale EFFVM_ATTACKER, 2, 0x333, -0x333, 4, 2, 2
    SetYield FALSE
    PlaySE SEQ_SE_W216_01, 1, EFFVM_DEFENDER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W216_01, 2, EFFVM_DEFENDER, 10, 0, 115, 0, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W129_03, 1, EFFVM_DEFENDER, 10, -80, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 389
    EmitterAddOrtho 389, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 389, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 4, 2, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
