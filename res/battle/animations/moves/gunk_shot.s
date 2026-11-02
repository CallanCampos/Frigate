// MOVE_GUNK_SHOT's animation: member 441 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant GunkShot

GunkShot:
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    Wait EFFVM_WAIT_ALL
    LoadBg 21
    SetVanish 2, 0
    PaletteFade 4, 0, 14, 0, 0
    WaitFrames 6
    CameraMovePreset 1, 9, 16, 0, 12
    SetVanish 0, 1
    SetVanish 1, 1
    BgMove 4, -30, 0, 200, 1, 0
    WaitFrames 6
    PaletteFade 4, 14, 0, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W157_03, 3, EFFVM_ATTACKER, 0, -700, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 3, EFFVM_ATTACKER, 80, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 120, 0, 60, 20, 0, 0
    PlaySE SEQ_SE_W027_01, 1, EFFVM_ATTACKER, 0, -700, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 80, 0, 0, 0, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W124_01, 2, EFFVM_ATTACKER, 80, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 110, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 115, 0, 90, 20, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 111, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 116, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 121, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W055_01, 1, EFFVM_ATTACKER, 80, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 200, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 115, 0, 160, 40, 0, 0
    MoveSE 1, 0, 0, -1000, 2000, 80, 80, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 75, 50, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 75, 50, 0, 0
    SetYield TRUE
    ParticleLoad 620
    EmitterAdd 620, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 620, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 620, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0xe66, 0x1000, 0x1000
    WaitFrames 80
    CameraMovePreset 1, 11, 32, 0, 22
    WaitFrames 20
    McssPalFade EFFVM_DEFENDER, 0, 12, 1, 0x300c
    EmitterAdd 620, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 3
    WaitFrames 30
    McssPalFade EFFVM_DEFENDER, 12, 0, 1, 0x300c
    WaitFrames 30
    PaletteFade 4, 0, 14, 0, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 0
    SetVanish 1, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 14, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
