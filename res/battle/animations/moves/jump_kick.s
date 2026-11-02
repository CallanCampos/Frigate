// MOVE_JUMP_KICK's animation: member 26 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant JumpKick

JumpKick:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    ParticleLoad 192
    EmitterMove 192, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x99a, 0x1000, 0
    SetYield FALSE
    PlaySE SEQ_SE_W026_01, 1, EFFVM_DEFENDER, 0, 300, 120, 0, 0, 0
    PlaySE SEQ_SE_W015_01, 2, EFFVM_DEFENDER, 0, 300, 110, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 20, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 20, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 30, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 35, 0, 110, 0, 0, 0
    SetYield TRUE
    WaitFrames 20
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 6
    EmitterAdd 192, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x19a, 2, 1, 3
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
