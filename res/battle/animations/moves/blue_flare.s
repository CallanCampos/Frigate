// MOVE_BLUE_FLARE's animation: member 551 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant BlueFlare

BlueFlare:
    SetYield FALSE
    PlaySE SEQ_SE_SDEMO_03, 1, EFFVM_DEFENDER, 30, 0, 127, 0, 0, 0
    SetYield TRUE
    LoadBg 216
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 0, 16, 2, 0
    McssSetValue EFFVM_ALL, 1
    Wait EFFVM_WAIT_ALL
    SetVanish 2, 0
    SetVanish 0, 1
    SetVanish 1, 1
    ParticleLoad 724
    EmitterAdd 724, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    SetYield FALSE
    PlaySE SEQ_SE_W307_02, 1, EFFVM_DEFENDER, 0, 500, 100, 0, 0, 0
    PlaySE SEQ_SE_W307_02, 2, EFFVM_DEFENDER, 8, 500, 105, 0, 0, 0
    PlaySE SEQ_SE_W307_02, 1, EFFVM_DEFENDER, 16, 450, 120, 0, 0, 0
    PlaySE SEQ_SE_W307_02, 2, EFFVM_DEFENDER, 24, 550, 100, 0, 0, 0
    PlaySE SEQ_SE_W307_02, 1, EFFVM_DEFENDER, 35, 600, 95, 0, 0, 0
    PlaySE SEQ_SE_W307_02, 2, EFFVM_DEFENDER, 40, 400, 110, 0, 0, 0
    PlaySE SEQ_SE_W307_02, 1, EFFVM_DEFENDER, 48, 500, 100, 0, 0, 0
    PlaySE SEQ_SE_W307_02, 2, EFFVM_DEFENDER, 56, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W307_02, 1, EFFVM_DEFENDER, 70, -600, 127, 0, 0, 0
    PlaySE SEQ_SE_W018_01, 0, EFFVM_DEFENDER, 40, -800, 127, 0, 0, 0
    PlaySE SEQ_SE_W018_01, 1, EFFVM_DEFENDER, 80, -800, 127, 0, 0, 0
    MoveSE 0, 0, 0, -800, 0, 41, 100, 0, 0
    MoveSE 1, 0, 0, -800, 0, 81, 140, 0, 0
    PlaySE SEQ_SE_W075_04, 1, EFFVM_DEFENDER, 200, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W310_01, 2, EFFVM_DEFENDER, 190, 0, 127, 0, 0, 0
    SetYield TRUE
    EmitterAdd 724, 5, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    EmitterAdd 724, 4, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    WaitFrames 30
    McssPalFade EFFVM_DEFENDER, 0, 8, 4, 0x1f
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 140
    BgWave 0, 0x2000, 16, 90, 0, 0
    BgMove 4, 2, 0, 60, 0, 0
    PaletteFade 3, 16, 0, 1, 0
    McssScale EFFVM_DEFENDER, 2, 0x19a, -0x19a, 4, 0, 6
    EmitterAdd 724, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 724, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 40
    EmitterAdd 724, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    PaletteFade 3, 0, 16, 2, 0
    WaitFrames 6
    McssPalFade EFFVM_DEFENDER, 8, 0, 2, 0x1f
    WaitFrames 30
    SetVanish 1, 0
    SetVanish 0, 0
    WaitFrames 2
    PaletteFade 4, 16, 0, 2, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetValue EFFVM_ALL, 0
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
