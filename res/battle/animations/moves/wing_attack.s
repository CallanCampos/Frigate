// MOVE_WING_ATTACK's animation: member 17 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant WingAttack

WingAttack:
    CameraMovePreset 1, 11, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W017_01, 2, EFFVM_DEFENDER, 12, 0, 125, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    ParticleLoad 182
    EmitterAdd 182, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 182, 1, EFFVM_EMIT_DEFENDER, 8, 0x2800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 4
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 0, 0, 110, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
