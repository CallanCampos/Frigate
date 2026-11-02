// MOVE_LAVA_PLUME's animation: member 436 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant LavaPlume

LavaPlume:
    CameraMovePreset 1, 14, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 0, 200, 120, 0, 0, 0
    PlaySE SEQ_SE_W307_01, 2, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    MoveSE 2, 0, 1, 120, 0, 20, 50, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_PAN_CENTER, 40, 0, 0, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 60, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 60, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W157_04, 3, EFFVM_DEFENDER, 45, 0, 50, 0, 0, 0
    PlaySE SEQ_SE_W464_01, 1, EFFVM_DEFENDER, 45, 0, 127, 0, 0, 0
    SetYield TRUE
    ParticleLoad 615
    EmitterAddOrtho 615, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 615, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 615, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 40
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 2, 1, 4
    McssPalFade EFFVM_DEFENDER, 0, 13, 2, 0x1f
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 13, 0, 2, 0x1f
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
