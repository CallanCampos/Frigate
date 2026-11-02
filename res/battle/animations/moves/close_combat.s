// MOVE_CLOSE_COMBAT's animation: member 370 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant CloseCombat

CloseCombat:
    SetYield FALSE
    PlaySE SEQ_SE_W010_01, 1, EFFVM_DEFENDER, 10, -1000, 100, 0, 0, 0
    SetYield TRUE
    CameraMovePreset 1, 11, 16, 0, 8
    LoadBg 153
    SetVanish 2, 0
    PaletteFade 4, 0, 15, 0, 0
    Wait EFFVM_WAIT_ALL
    BgMove 4, 24, 0, 90, 0, 0
    SetVanish 0, 1
    SetVanish 1, 1
    PaletteFade 3, 13, 0, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 0, 300, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 20, 300, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 30, 300, 60, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 40, 300, 40, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 50, 300, 10, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 10, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 15, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 20, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 25, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 30, 0, 110, 0, 0, 0
    SetYield TRUE
    ParticleLoad 549
    EmitterAdd 549, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 549, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 549, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 1, 1, 16
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 2, 1, 4
    Wait EFFVM_WAIT_ALL
    PaletteFade 3, 0, 15, 0, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 0
    SetVanish 1, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    PaletteFade 2, 15, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
