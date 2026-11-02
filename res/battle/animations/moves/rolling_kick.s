// MOVE_ROLLING_KICK's animation: member 27 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant RollingKick

RollingKick:
    McssCircle EFFVM_ATTACKER, 3, 0, 0x2000, 0x2000, 0xa000, 0x1000, 0x1000, 0
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 2, EFFVM_ATTACKER, 0, 300, 0, 0, 0, 0
    SetYield TRUE
    WaitFrames 2
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 2
    ParticleLoad 193
    EmitterAdd 193, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_DEFENDER, 5, 0, 115, 0, 0, 0
    SetYield TRUE
    WaitFrames 2
    EmitterAdd 193, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_PARTICLES
    SetYield FALSE
    PlaySE SEQ_SE_W023_01, 2, EFFVM_DEFENDER, 0, -300, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 0, 0, 90, 0, 0, 0
    SetYield TRUE
    EmitterAdd 193, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
