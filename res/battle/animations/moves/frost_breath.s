// MOVE_FROST_BREATH's animation: member 524 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant FrostBreath

FrostBreath:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 0, 16, 1, 0
    SetYield FALSE
    PlaySE SEQ_SE_W018_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W363_01, 2, EFFVM_DEFENDER, 30, 1700, 100, 0, 0, 0
    MoveSE 1, 0, 1, 120, 0, 80, 10, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 60, 0, 0
    SetYield TRUE
    ParticleLoad 694
    EmitterMove 694, 2, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    EmitterMove 694, 4, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    WaitFrames 30
    CameraMovePreset 1, 11, 14, 0, 8
    McssPalFade EFFVM_DEFENDER, 0, 6, 3, 0x7fff
    SetYield FALSE
    PlaySE SEQ_SE_W181_01, 2, EFFVM_DEFENDER, 60, -100, 127, 0, 0, 0
    PlaySE SEQ_SE_W181_01, 2, EFFVM_DEFENDER, 100, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W043_01, 2, EFFVM_DEFENDER, 180, 0, 127, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 2, 0, 32
    EmitterAdd 694, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 694, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 694, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 195
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 2, 0, 2
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    PaletteFade 4, 16, 0, 1, 0
    McssPalFade EFFVM_DEFENDER, 6, 0, 3, 0x7fff
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
