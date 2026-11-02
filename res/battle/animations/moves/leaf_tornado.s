// MOVE_LEAF_TORNADO's animation: member 536 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant LeafTornado

LeafTornado:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W027_01, 1, EFFVM_ATTACKER, 0, -500, 127, 0, 0, 0
    MoveSE 1, 0, 0, -500, 1000, 0, 60, 0, 0
    MoveSE 1, 0, 1, 127, 0, 50, 10, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 60, 0, 0, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 705
    McssCircle EFFVM_ATTACKER, 2, 3, 0x1000, 0x1000, 0xa000, 0x1000, 0x3000, 0
    EmitterAdd 705, 1, EFFVM_EMIT_ATTACKER, 8, 0x333, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 705, 1, EFFVM_EMIT_ATTACKER, 8, 0x333, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 32
    McssSetValue EFFVM_ALL, 1
    McssSetVanish EFFVM_DEFENDER, TRUE
    CameraMovePreset 1, 11, 16, 0, 12
    EmitterAdd 705, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    EmitterAdd 705, 1, EFFVM_EMIT_DEFENDER, 8, 0x333, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 705, 2, EFFVM_EMIT_DEFENDER, 8, 0x333, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    EmitterAdd 705, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x800, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W027_01, 2, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    MoveSE 2, 0, 0, 0, -1000, 0, 80, 0, 0
    MoveSE 2, 0, 1, 127, 0, 50, 30, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 80, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_023, 1, EFFVM_DEFENDER, 40, 0, 127, 0, 0, 0
    SetYield TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 12
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    McssSetValue EFFVM_ALL, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
