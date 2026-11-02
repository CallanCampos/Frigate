// MOVE_ACID's animation: member 51 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Acid

Acid:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    ParticleLoad 216
    SetYield FALSE
    PlaySE SEQ_SE_W051_02, 1, EFFVM_DEFENDER, 0, 0, 115, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 20, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W051_03, 2, EFFVM_DEFENDER, 45, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W051_01, 1, EFFVM_DEFENDER, 55, 0, 120, 0, 0, 0
    SetYield TRUE
    EmitterMove 216, 1, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x1e000, 0x399a, 0x1000, 0x1000, 0
    WaitFrames 3
    EmitterMove 216, 1, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x1e000, 0x399a, 0x1000, 0x1000, 0
    WaitFrames 3
    EmitterMove 216, 1, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x1e000, 0x399a, 0x1000, 0x1000, 0
    WaitFrames 3
    EmitterMove 216, 1, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x1e000, 0x399a, 0x1000, 0x1000, 0
    WaitFrames 8
    CameraMovePreset 1, 11, 16, 0, 8
    EmitterAdd 216, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 216, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xe66, 0x1000
    EmitterAdd 216, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 6, 1, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
