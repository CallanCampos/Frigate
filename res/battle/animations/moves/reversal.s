// MOVE_REVERSAL's animation: member 179 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Reversal

Reversal:
    PaletteFade 4, 0, 12, 1, 31710
    PlaySE SEQ_SE_W179_01, 1, EFFVM_PAN_CENTER, 0, 200, 100, 0, 0, 0
    Wait EFFVM_WAIT_STAGE_FIELD_FADE
    PaletteFade 4, 12, 0, 1, 31710
    Wait EFFVM_WAIT_STAGE_FIELD_FADE
    PaletteFade 4, 0, 12, 1, 31710
    PlaySE SEQ_SE_W179_01, 1, EFFVM_PAN_CENTER, 0, 200, 100, 0, 0, 0
    Wait EFFVM_WAIT_STAGE_FIELD_FADE
    PaletteFade 4, 12, 0, 1, 31710
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 9, 16, 0, 8
    ParticleLoad 350
    EmitterAdd 350, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W179_03, 1, EFFVM_ATTACKER, 0, 200, 110, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 80, 200, 0, 0, 0, 0
    MoveSE 1, 0, 1, 110, 0, 60, 20, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    PlaySE SEQ_SE_W179_02, 1, EFFVM_PAN_CENTER, 15, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W006_03, 2, EFFVM_PAN_CENTER, 15, 0, 115, 0, 0, 0
    McssMoveXY EFFVM_ATTACKER, 2, 0x1000, 0, 6, 1, 1
    PaletteFade 4, 0, 12, 1, 31710
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 12, 0, 1, 31710
    CameraMovePreset 1, 11, 14, 0, 8
    Wait EFFVM_WAIT_ALL
    EmitterAdd 350, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 350, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 2, 0, 1
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 0, 60, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 0, 0, 110, 0, 0, 0
    SetYield TRUE
    WaitFrames 20
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
