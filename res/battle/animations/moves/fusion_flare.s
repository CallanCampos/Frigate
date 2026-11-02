// MOVE_FUSION_FLARE's animation: member 558 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant FusionFlare
    ScriptVariant FusionFlare_Variant1

FusionFlare:
    SetYield FALSE
    PlaySE SEQ_SE_SHINKA_W025, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_016, 1, EFFVM_ATTACKER, 60, -100, 100, 0, 0, 0
    PlaySE SEQ_SE_W411_HAAA, 2, EFFVM_ATTACKER, 70, 200, 127, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 70, 80, 0, 0
    SetYield TRUE
    CameraMovePreset 1, 9, 32, 0, 26
    ParticleLoad 735
    EmitterAdd 735, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_ATTACKER, 0, 8, 1, 0x1f
    WaitFrames 20
    PaletteFade 4, 0, 12, 1, 0
    CameraMove 1, 0xca6, 0x5f33, 0x1dcc3, -0xe8d, 0xad9a, 0x27f6, 32, 0, 26
    EmitterAddAt 735, 2, 0, 0xa000, 0, 0, 0, 0, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddAt 735, 5, 0, 0xa000, 0, 0, 0, 0, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 60
    EmitterAddAt 735, 3, 0, 0xa000, 0, 0, 0, 0, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    EmitterMoveFrom 735, 4, 1, 0, 0x7000, 0, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0, 0x1000, 0x1000, 0
    CameraMovePreset 1, 11, 12, 0, 10
    McssPalFade EFFVM_ATTACKER, 8, 0, 1, 0x1f
    WaitFrames 15
    SetYield FALSE
    PlaySE SEQ_SE_W307_EXP, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W307_01, 2, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    MoveSE 2, 0, 1, 127, 0, 55, 25, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 80, 0, 0, 0, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_DEFENDER, 0, 8, 1, 0x1f
    CameraShake 0, 0x800, 4096, 1, 1, 6
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 1, 1, 12
    EmitterAdd 735, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 735, 3, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 735, 6, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x800, -0x800, 4, 1, 1
    CameraMovePreset 1, 8, 40, 0, 32
    WaitFrames 30
    PaletteFade 4, 12, 0, 1, 0
    McssPalFade EFFVM_DEFENDER, 8, 0, 1, 0x1f
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End

FusionFlare_Variant1:
    SetYield FALSE
    PlaySE SEQ_SE_SHINKA_W025, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_SDEMO_03, 1, EFFVM_ATTACKER, 50, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_162, 2, EFFVM_ATTACKER, 80, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W411_HAAA, 1, EFFVM_ATTACKER, 130, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W307_EXP, 1, EFFVM_PAN_CENTER, 170, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W307_EXP, 2, EFFVM_DEFENDER, 180, -400, 127, 0, 0, 0
    SetYield TRUE
    CameraMovePreset 1, 9, 32, 0, 26
    ParticleLoad 736
    EmitterAdd 736, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_ATTACKER, 0, 8, 1, 0x1f
    WaitFrames 20
    PaletteFade 4, 0, 12, 1, 0
    CameraMove 1, 0xca6, 0x5f33, 0x1dcc3, -0xe8d, 0xcd9a, 0x27f6, 32, 0, 26
    EmitterAddAt 736, 2, 0, 0xd000, 0, 0, 0, 0, 0, 0, 0, 0x1000, 0x1b33, 0x1800, 0x1000
    EmitterAddAt 736, 5, 0, 0xd000, 0, 0, 0, 0, 0, 0, 0, 0x1000, 0x2000, 0x2000, 0x1000
    EmitterAddAt 736, 6, 0, 0xd000, 0, 0, 0, 0, 0, 0, 0, 0x1000, 0x3000, 0x2000, 0x1000
    WaitFrames 60
    EmitterAddAt 736, 1, 0, 0xd000, 0, 0, 0, 0, 0, 0, 0, 0x1000, 0x2000, 0x1000, 0x1000
    WaitFrames 40
    EmitterAddAt 736, 3, 0, 0xd000, 0, 0, 0, 0, 0, 0, 0, 0x1000, 0x1000, 0x2000, 0x1000
    WaitFrames 10
    EmitterMoveFrom 736, 4, 1, 0, 0xd000, 0, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0, 0x2000, 0x2000, 0
    CameraMovePreset 1, 11, 12, 0, 10
    McssPalFade EFFVM_ATTACKER, 8, 0, 1, 0x1f
    WaitFrames 15
    CameraMove 2, 0x14000, 0, 0x1e000, 0, 0x6000, 0, 30, 1, 26
    McssPalFade EFFVM_DEFENDER, 0, 8, 1, 0x1f
    CameraShake 0, 0x800, 4096, 1, 1, 6
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 1, 1, 12
    EmitterAdd 736, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x2000, 0x2000, 0x1000
    EmitterAdd 736, 3, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x2000, 0x2000, 0x1000
    EmitterAdd 736, 6, EFFVM_EMIT_DEFENDER, 8, 0xa000, 0, 0, 0x1000, 0x1000, 0x2000, 0x1000
    EmitterAdd 736, 6, EFFVM_EMIT_DEFENDER, 8, 0x6000, 0, 0, 0x1000, 0x1000, 0x3000, 0x1000
    EmitterAdd 736, 6, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x4000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x800, -0x800, 4, 1, 1
    WaitFrames 40
    CameraMovePreset 1, 8, 50, 0, 46
    WaitFrames 20
    PaletteFade 4, 12, 0, 1, 0
    McssPalFade EFFVM_DEFENDER, 8, 0, 1, 0x1f
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
