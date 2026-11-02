// MOVE_GRASS_PLEDGE's animation: member 520 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant GrassPledge

GrassPledge:
    ParticleLoad 690
    CameraMovePreset 1, 11, 10, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W075_03, 1, EFFVM_DEFENDER, 10, 500, 127, 0, 0, 0
    PlaySE SEQ_SE_W075_03, 1, EFFVM_DEFENDER, 30, 500, 127, 0, 0, 0
    PlaySE SEQ_SE_W075_03, 1, EFFVM_DEFENDER, 50, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W027_01, 2, EFFVM_DEFENDER, 10, 200, 127, 0, 0, 0
    PlaySE SEQ_SE_W027_01, 2, EFFVM_DEFENDER, 30, 200, 127, 0, 0, 0
    PlaySE SEQ_SE_W027_01, 2, EFFVM_DEFENDER, 50, 200, 127, 0, 0, 0
    MoveSE 1, 0, 1, 127, 0, 120, 10, 0, 0
    MoveSE 2, 0, 1, 127, 0, 120, 1, 0, 0
    MoveSE 2, 0, 0, -500, 200, 0, 200, 0, 0
    SetYield TRUE
    EmitterAdd 690, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 690, 4, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 690, 7, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x19a, 2, 1, 10
    EmitterAdd 690, 2, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 690, 5, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 690, 8, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    EmitterAdd 690, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 690, 3, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 690, 6, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
