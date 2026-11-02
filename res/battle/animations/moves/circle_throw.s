// MOVE_CIRCLE_THROW's animation: member 509 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant CircleThrow

CircleThrow:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W026_01, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    McssCircle EFFVM_ATTACKER, 3, 0, 0x2000, 0x2000, 0xa000, 0x1000, 0x1000, 0
    WaitFrames 2
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 2
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W014_01, 2, EFFVM_DEFENDER, 20, 0, 100, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    ParticleLoad 679
    EmitterAdd 679, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 679, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 8
    McssCircle EFFVM_DEFENDER, 3, 1, 0x1000, 0x1000, 0x6000, 0x1000, 0x2000, 0
    McssScale EFFVM_DEFENDER, 2, 0x666, -0x666, 3, 0, 4
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
