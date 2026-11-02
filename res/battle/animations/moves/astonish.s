// MOVE_ASTONISH's animation: member 310 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Astonish

Astonish:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W310_01, 1, EFFVM_ATTACKER, 0, 1000, 115, 0, 0, 0
    SetYield TRUE
    McssScale EFFVM_ATTACKER, 2, 0x333, -0x333, 3, 2, 1
    McssCircle EFFVM_ATTACKER, 2, 0, 0, 0x3000, 0x14000, 0, 0x1000, 0
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 12, 0, 8
    Wait EFFVM_WAIT_CAMERA
    SetYield FALSE
    PlaySE SEQ_SE_W310_02, 2, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 481
    McssSetVanish EFFVM_DEFENDER, TRUE
    EmitterAdd 481, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 481, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 3, 2, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
