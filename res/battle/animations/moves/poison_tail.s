// MOVE_POISON_TAIL's animation: member 342 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant PoisonTail

PoisonTail:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_DEFENDER, 15, 200, 110, 0, 0, 0
    PlaySE SEQ_SE_W017_01, 2, EFFVM_DEFENDER, 20, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W054_01, 2, EFFVM_DEFENDER, 20, 100, 80, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 90, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 110, 0, 40, 40, 0, 0
    PlaySE SEQ_SE_W022_01, 1, EFFVM_DEFENDER, 20, 0, 127, 0, 0, 0
    SetYield TRUE
    ParticleLoad 518
    EmitterAdd 518, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 518, 1, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 518, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 518, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0, -0x333, 2, 2, 1
    McssPalFade EFFVM_DEFENDER, 0, 13, 0, 0x7814
    WaitFrames 6
    CameraShake 0, 0x4cd, 0, 2, 0, 6
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    McssPalFade EFFVM_DEFENDER, 13, 0, 0, 0x7814
    Wait EFFVM_WAIT_ALL
    End
