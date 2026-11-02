// MOVE_FLING's animation: member 374 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Fling

Fling:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W088_01, 1, EFFVM_DEFENDER, 0, 0, 70, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 40, 0, 0
    SetYield TRUE
    ParticleLoad 553
    EmitterMove 553, 0, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x1e000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 1
    EmitterMove 553, 0, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x1e000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 1
    EmitterMove 553, 0, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x1e000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 2
    CameraMovePreset 1, 11, 16, 0, 8
    EmitterMove 553, 0, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x1e000, 0x4000, 0x1000, 0x1000, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W111_01, 1, EFFVM_DEFENDER, 0, -500, 120, 0, 0, 0
    PlaySE SEQ_SE_W111_02, 2, EFFVM_DEFENDER, 3, -500, 120, 0, 0, 0
    MoveSE 1, 0, 1, 120, 0, 0, 30, 0, 0
    SetYield TRUE
    EmitterAdd 553, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 553, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 4, 1, 1
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
