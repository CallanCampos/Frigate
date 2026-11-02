// MOVE_ENCORE's animation: member 227 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Encore

Encore:
    PaletteFade 2, 0, 10, 2, 0
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W227_03, 1, EFFVM_DEFENDER, 30, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_MSCL_09, 2, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    MoveSE 2, 0, 1, 127, 0, 100, 20, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 120, 0, 0, 0, 0, 0
    SetYield TRUE
    ParticleLoad 397
    EmitterAdd 397, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 397, 1, EFFVM_EMIT_DEFENDER, 8, 0xccd, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 397, 2, EFFVM_EMIT_DEFENDER, 8, 0xccd, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 10, 0, 3
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 10, 0, 2, 0
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
