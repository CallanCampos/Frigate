// MOVE_SLUDGE_BOMB's animation: member 188 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant SludgeBomb

SludgeBomb:
    McssSetValue EFFVM_ALL, 1
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W188_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 30, 0, 0
    SetYield TRUE
    ParticleLoad 358
    EmitterMove 358, 2, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x1e000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 10
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W051_01, 2, EFFVM_DEFENDER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W124_01, 1, EFFVM_DEFENDER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 50, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 110, 0, 20, 30, 0, 0
    SetYield TRUE
    EmitterAdd 358, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    EmitterAdd 358, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 4, 1, 1
    McssPalFade EFFVM_DEFENDER, 0, 10, 2, 0x7c1f
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssPalFade EFFVM_DEFENDER, 10, 0, 2, 0x7c1f
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetValue EFFVM_ALL, 0
    Wait EFFVM_WAIT_ALL
    End
