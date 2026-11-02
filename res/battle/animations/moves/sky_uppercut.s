// MOVE_SKY_UPPERCUT's animation: member 327 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant SkyUppercut

SkyUppercut:
    CameraMovePreset 1, 11, 16, 0, 8
    LoadBg 132
    SetVanish 2, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 0, 15, 0, 0
    Wait EFFVM_WAIT_ALL
    BgMove 4, 0, 10, 100, 0, 0
    SetVanish 1, 1
    SetVanish 0, 1
    PaletteFade 4, 15, 0, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W157_03, 1, EFFVM_DEFENDER, 0, 1000, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 80, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 1000, 4500, 0, 70, 0, 0
    MoveSE 1, 0, 1, 100, 0, 50, 30, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 20, 500, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 40, 500, 50, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 60, 500, 30, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 80, 500, 10, 0, 0, 0
    SetYield TRUE
    ParticleLoad 501
    EmitterAdd 501, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 501, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 501, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 501, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x666, 2, 1, 4
    Wait EFFVM_WAIT_MCSS
    PaletteFade 4, 0, 15, 1, 0
    Wait EFFVM_WAIT_BG
    SetVanish 0, 0
    SetVanish 1, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    PaletteFade 4, 15, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
