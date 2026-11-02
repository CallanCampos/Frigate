// MOVE_WHIRLWIND's animation: member 18 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Whirlwind

Whirlwind:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W018_01, 1, EFFVM_DEFENDER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W059_01, 2, EFFVM_DEFENDER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 100, 0, 0, 0, 0, 0
    MoveSE 2, 0, 0, 0, 1300, 0, 20, 0, 0
    MoveSE 2, 0, 0, 1300, 0, 40, 20, 0, 0
    MoveSE 2, 0, 1, 0, 100, 0, 30, 0, 0
    MoveSE 2, 0, 1, 100, 0, 50, 40, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 40, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 40, 0, 0
    SetYield TRUE
    ParticleLoad 183
    EmitterAdd 183, 0, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 183, 0, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x4cd, 0x1000, 0x1000
    WaitFrames 20
    EmitterAdd 183, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x4cd, 0x1000, 0x1000
    EmitterAdd 183, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x4cd, 0x1000, 0x1000
    WaitFrames 10
    EmitterAdd 183, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x4cd, 0x1000, 0x1000
    EmitterAdd 183, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x4cd, 0x1000, 0x1000
    EmitterAdd 183, 3, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x4cd, 0x1000, 0x1000
    EmitterAdd 183, 3, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    CameraMovePreset 1, 11, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W018_02, 1, EFFVM_DEFENDER, 50, 0, 125, 0, 0, 0
    SetYield TRUE
    WaitFrames 25
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 3, 1, 1
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 2, 0, 6
    WaitFrames 20
    McssRotate EFFVM_DEFENDER, 1, 0xffff000, 80, 0, 0
    McssSetValue EFFVM_DEFENDER, 1
    McssMoveXY EFFVM_DEFENDER, 1, -0x14000, 0xa000, 30, 0, 1
    WaitFrames 20
    McssSetMode EFFVM_DEFENDER, 3
    Wait EFFVM_WAIT_ALL
    Gauge 2, 4
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
