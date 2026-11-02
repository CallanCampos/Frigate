// MOVE_FURY_CUTTER's animation: member 210 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant FuryCutter

FuryCutter:
    PaletteFade 4, 0, 16, 0, 0
    PlaySE SEQ_SE_W179_01, 1, EFFVM_DEFENDER, 0, 200, 100, 0, 0, 0
    Wait EFFVM_WAIT_STAGE_FIELD_FADE
    PaletteFade 4, 16, 0, 0, 0
    Wait EFFVM_WAIT_STAGE_FIELD_FADE
    PaletteFade 4, 0, 16, 0, 0
    PlaySE SEQ_SE_W179_01, 2, EFFVM_DEFENDER, 0, 200, 100, 0, 0, 0
    Wait EFFVM_WAIT_STAGE_FIELD_FADE
    PaletteFade 4, 16, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    ParticleLoad 380
    EmitterAdd 380, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 380, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 380, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x19a, -0x19a, 1, 1, 4
    SetYield FALSE
    PlaySE SEQ_SE_W010_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 0, 0, 110, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
