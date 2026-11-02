// MOVE_FRENZY_PLANT's animation: member 338 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant FrenzyPlant

FrenzyPlant:
    CameraMovePreset 1, 9, 16, 0, 12
    LoadBg 138
    SetVanish 2, 0
    PaletteFade 4, 0, 15, 1, 0
    Wait EFFVM_WAIT_ALL
    BgMove 4, 0, 12, 200, 1, 0
    SetVanish 0, 1
    SetVanish 1, 1
    PaletteFade 4, 15, 0, 1, 0
    SetYield FALSE
    PlaySE SEQ_SE_W096_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_ATTACKER, 17, 0, 120, 0, 0, 0
    SetYield TRUE
    PlaySE SEQ_SE_W157_03, 3, EFFVM_ATTACKER, 20, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 3, EFFVM_ATTACKER, 150, 0, 0, 0, 0, 0
    MoveSE 3, 0, 1, 115, 0, 120, 30, 0, 0
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssMoveXY EFFVM_ATTACKER, 2, 0, 0x800, 2, 1, 1
    McssScale EFFVM_ATTACKER, 3, -0x4cd, 0x4cd, 2, 1, 1
    ParticleLoad 514
    EmitterAdd 514, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 514, 8, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 12
    CameraShake 0, 0x333, 4096, 1, 1, 14
    WaitFrames 20
    CameraMovePreset 1, 11, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W157_01, 1, EFFVM_DEFENDER, 0, 500, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 100, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 115, 0, 70, 30, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 85, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 95, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 110, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 123, 0, 115, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    EmitterAdd 514, 4, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 514, 7, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 514, 2, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 514, 5, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 514, 3, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 514, 6, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 80
    EmitterAdd 514, 0, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 6
    WaitFrames 40
    PaletteFade 4, 0, 15, 1, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 15, 0, 1, 0
    SetVanish 1, 0
    SetVanish 0, 0
    CameraMovePreset 1, 8, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
