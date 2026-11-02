// MOVE_HYPER_BEAM's animation: member 63 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant HyperBeam

HyperBeam:
    LoadBg 33
    SetYield FALSE
    PlaySE SEQ_SE_SDEMO_03, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    PaletteFade 2, 0, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 3, 16, 0, 0, 0
    CameraMovePreset 1, 9, 16, 0, 12
    SetVanish 0, 1
    SetVanish 1, 1
    SetVanish 2, 0
    BgMove 4, -30, 0, 140, 1, 0
    WaitFrames 6
    McssSetVanish EFFVM_ATTACKER, TRUE
    WaitFrames 20
    CameraMovePreset 1, 11, 58, 0, 50
    SetYield FALSE
    PlaySE SEQ_SE_W011_01, 1, EFFVM_ATTACKER, 0, -300, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 100, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W063_BEEMB, 2, EFFVM_ATTACKER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 50, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 55, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 60, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 65, 0, 127, 0, 0, 0
    MoveSE 1, 0, 0, 0, 2000, 0, 50, 0, 0
    MoveSE 1, 0, 1, 120, 0, 80, 20, 0, 0
    MoveSE 2, 0, 1, 125, 0, 60, 40, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 50, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 50, 0, 0
    SetYield TRUE
    ParticleLoad 228
    EmitterAdd 228, 0, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 228, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 228, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 228, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 14
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 8
    WaitFrames 58
    PaletteFade 3, 0, 16, 1, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 16, 0, 0, 0
    SetVanish 0, 0
    SetVanish 1, 0
    SetVanish 2, 1
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
