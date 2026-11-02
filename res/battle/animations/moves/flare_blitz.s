// MOVE_FLARE_BLITZ's animation: member 394 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant FlareBlitz

FlareBlitz:
    CameraMovePreset 1, 9, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W307_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 60, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 0, 3000, 0, 60, 0, 0
    MoveSE 1, 0, 1, 120, 0, 40, 20, 0, 0
    PlaySE SEQ_SE_W002_01, 2, EFFVM_ATTACKER, 40, -100, 120, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 2, EFFVM_ATTACKER, 50, -100, 40, 0, 0, 0
    SetYield TRUE
    ParticleLoad 573
    EmitterAdd 573, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    McssMoveXY EFFVM_ATTACKER, 2, 0x1000, 0, 3, 2, 1
    LoadBg 156
    PaletteFade 3, 0, 13, 0, 0
    SetVanish 2, 0
    PaletteFade 2, 0, 13, 1, 0
    Wait EFFVM_WAIT_ALL
    PlaySE SEQ_SE_W307_01, 0, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    MoveSE 0, 0, 0, 0, 500, 0, 60, 0, 0
    PlaySE SEQ_SE_DUMMY5, 0, EFFVM_DEFENDER, 60, 0, 0, 0, 0, 0
    PanSE 0, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 60, 0, 0
    CameraMovePreset 1, 11, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W295_02, 2, EFFVM_DEFENDER, 50, -200, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 50, -200, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 65, 0, 60, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 80, 0, 20, 0, 0, 0
    SetYield TRUE
    EmitterAdd 573, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 573, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    SetVanish 0, 1
    SetVanish 1, 1
    Wait EFFVM_WAIT_BG
    PaletteFade 3, 13, 0, 2, 0
    WaitFrames 2
    BgPalAnim 26, 2
    BgWave 0, 0x3000, 16, 60, 0, 0
    McssPalFade EFFVM_DEFENDER, 0, 13, 0, 0
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 2, 1, 2
    WaitFrames 45
    PaletteFade 3, 0, 13, 2, 0
    WaitFrames 10
    SetVanish 0, 0
    SetVanish 1, 0
    McssPalFade EFFVM_DEFENDER, 13, 0, 0, 0
    PaletteFade 4, 13, 0, 0, 0
    CameraMovePreset 1, 8, 12, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
