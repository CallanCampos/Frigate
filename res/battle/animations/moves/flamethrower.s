// MOVE_FLAMETHROWER's animation: member 53 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Flamethrower

Flamethrower:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 0, 10, 0, 30
    ParticleLoad 218
    SetYield FALSE
    PlaySE SEQ_SE_W307_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W027_01, 2, EFFVM_ATTACKER, 0, 2000, 100, 0, 0, 0
    MoveSE 2, 0, 0, -3000, 0, 0, 60, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 40, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 40, 0, 0
    SetYield TRUE
    EmitterMove 218, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    EmitterMove 218, 1, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    WaitFrames 20
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 26
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 0, -500, 100, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_016, 2, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_016, 2, EFFVM_DEFENDER, 30, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W307_02, 0, EFFVM_DEFENDER, 10, 0, 127, 0, 0, 0
    SetYield TRUE
    EmitterAdd 218, 3, EFFVM_EMIT_DEFENDER, 8, 0x1800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 218, 2, EFFVM_EMIT_DEFENDER, 8, 0x1800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_DEFENDER, 0, 10, 1, 0x1f
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 2, 1, 6
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 10, 0, 1, 0x1f
    PaletteFade 2, 10, 0, 0, 30
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
