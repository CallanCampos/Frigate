// MOVE_POISON_GAS's animation: member 139 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant PoisonGas

PoisonGas:
    CameraMovePreset 1, 9, 12, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W055_01, 2, EFFVM_ATTACKER, 0, 800, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 80, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 100, 0, 50, 30, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 40, 0, 0
    SetYield TRUE
    ParticleLoad 308
    EmitterMove 308, 0, 6, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x64000, 0x666, 0x1000, 0x1000, 0x4000
    WaitFrames 2
    EmitterMove 308, 0, 6, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x64000, 0x666, 0x1000, 0x1000, 0x4000
    WaitFrames 2
    EmitterMove 308, 0, 6, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x64000, 0x666, 0x1000, 0x1000, 0x4000
    WaitFrames 2
    EmitterMove 308, 0, 6, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x64000, 0x666, 0x1000, 0x1000, 0x4000
    WaitFrames 2
    EmitterMove 308, 0, 6, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x64000, 0x666, 0x1000, 0x1000, 0x4000
    WaitFrames 2
    EmitterMove 308, 0, 6, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x64000, 0x666, 0x1000, 0x1000, 0x4000
    WaitFrames 2
    EmitterMove 308, 0, 6, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x64000, 0x666, 0x1000, 0x1000, 0x4000
    WaitFrames 2
    EmitterMove 308, 0, 6, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x64000, 0x666, 0x1000, 0x1000, 0x4000
    WaitFrames 2
    EmitterMove 308, 0, 6, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x64000, 0x666, 0x1000, 0x1000, 0x4000
    WaitFrames 2
    CameraMovePreset 1, 11, 24, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W054_01, 1, EFFVM_DEFENDER, 50, 0, 127, 0, 0, 0
    MoveSE 1, 0, 1, 127, 0, 90, 30, 0, 0
    SetYield TRUE
    EmitterMove 308, 0, 6, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x64000, 0x666, 0x1000, 0x1000, 0x4000
    WaitFrames 2
    EmitterMove 308, 0, 6, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x64000, 0x666, 0x1000, 0x1000, 0x4000
    WaitFrames 2
    WaitFrames 10
    EmitterAdd 308, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
