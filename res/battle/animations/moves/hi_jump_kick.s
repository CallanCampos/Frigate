// MOVE_HI_JUMP_KICK's animation: member 136 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant HiJumpKick

HiJumpKick:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssMoveXY EFFVM_ATTACKER, 2, 0x1000, 0x1000, 2, 2, 1
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 0, 300, 120, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 16, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W011_01, 2, EFFVM_DEFENDER, 10, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 15, 0, 100, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_PARTICLES
    ParticleLoad 305
    EmitterAddOrtho 305, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x333, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 305, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
