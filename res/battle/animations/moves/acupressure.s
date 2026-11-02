// MOVE_ACUPRESSURE's animation: member 367 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Acupressure

Acupressure:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W295_02, 1, EFFVM_DEFENDER, 0, 0, 90, 0, 0, 0
    PlaySE SEQ_SE_W100_02, 2, EFFVM_DEFENDER, 0, 0, 70, 0, 0, 0
    PlaySE SEQ_SE_W255_MOMOMO, 1, EFFVM_DEFENDER, 30, 100, 120, 0, 0, 0
    MoveSE 1, 0, 1, 120, 0, 45, 27, 0, 0
    MoveSE 1, 0, 0, 0, 500, 20, 20, 0, 0
    SetYield TRUE
    ParticleLoad 545
    EmitterAdd 545, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 545, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 545, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 545, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_DEFENDER, 0, 10, 0, 0x3de
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 10, 0, 0, 0x3de
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
