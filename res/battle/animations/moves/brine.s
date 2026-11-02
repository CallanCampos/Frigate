// MOVE_BRINE's animation: member 362 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Brine

Brine:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W323_01, 1, EFFVM_DEFENDER, 10, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W055_01, 2, EFFVM_DEFENDER, 10, 100, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 65, 100, 0, 0, 0, 0
    SetYield TRUE
    ParticleLoad 540
    EmitterAdd 540, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x99a, 0x1000
    EmitterAdd 540, 1, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    EmitterAdd 540, 2, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0xe66, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 10
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 1, 4
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
