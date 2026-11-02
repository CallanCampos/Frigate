// MOVE_FUSION_BOLT's animation: member 559 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 3
    ScriptVariant FusionBolt
    ScriptVariant FusionBolt_Variant1
    ScriptVariant FusionBolt_Variant2

FusionBolt:
    SetYield FALSE
    PlaySE SEQ_SE_FLD_166, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    MoveSE 1, 0, 1, 127, 0, 40, 20, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 60, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_SDEMO_03, 2, EFFVM_ATTACKER, 50, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_SHINKA_W060C, 1, EFFVM_ATTACKER, 80, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_162, 2, EFFVM_DEFENDER, 130, 0, 127, 0, 0, 0
    MoveSE 2, 0, 1, 0, 127, 130, 10, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 80, 40, 0, 0
    SetYield TRUE
    CameraMovePreset 1, 9, 32, 0, 26
    PaletteFade 4, 0, 12, 1, 0
    McssPalFade EFFVM_ATTACKER, 0, 12, 1, 0x5980
    ParticleLoad 737
    EmitterAdd 737, 4, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    EmitterAdd 737, 4, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 737, 0, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetValue EFFVM_ATTACKER, 1
    McssAlpha EFFVM_ATTACKER, 1, 1, 32, 1, 0
    WaitFrames 40
    McssMoveXY EFFVM_ATTACKER, 1, 0, 0xc000, 24, 1, 0
    EmitterAdd 737, 3, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 737, 1, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 737, 6, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    McssSetMode EFFVM_ATTACKER, 3
    CameraMove 1, 0xca6, 0x5f33, 0x1dcc3, -0xe8d, 0xad9a, 0x27f6, 32, 0, 26
    EmitterAddAt 737, 4, 0, 0xa000, 0, 0, 0, 0, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddAt 737, 7, 0, 0xa000, 0, 0, 0, 0, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 80
    EmitterMoveFrom 737, 2, 2, 0, 0xa000, 0, EFFVM_EMIT_DEFENDER, 0x3000, 0x14000, -0x5000, 0x1000, 0x1000, 0
    CameraMovePreset 1, 11, 12, 0, 10
    SetYield FALSE
    PlaySE SEQ_SE_W229_KIIN, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_LBDEMO_05, 2, EFFVM_DEFENDER, 20, 0, 127, 0, 0, 0
    SetYield TRUE
    WaitFrames 15
    CameraShake 0, 0x800, 4096, 1, 1, 6
    McssPalFade EFFVM_DEFENDER, 0, 12, 1, 0x5980
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 4, 1, 1
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 1, 1, 12
    EmitterAdd 737, 3, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 737, 5, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 737, 1, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssSetMode EFFVM_ATTACKER, 4
    McssMoveXY EFFVM_ATTACKER, 5, 0, 0, 8, 1, 0
    McssAlpha EFFVM_ATTACKER, 1, 31, 32, 1, 0
    WaitFrames 30
    PaletteFade 4, 12, 0, 1, 0
    McssPalFade EFFVM_ATTACKER, 12, 0, 1, 0x5980
    McssPalFade EFFVM_DEFENDER, 12, 0, 1, 0x5980
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    WaitFrames 26
    McssSetValue EFFVM_ATTACKER, 0
    Wait EFFVM_WAIT_ALL
    End

FusionBolt_Variant1:
    SetYield FALSE
    PlaySE SEQ_SE_FLD_166, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_SDEMO_03, 2, EFFVM_ATTACKER, 50, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_SHINKA_W060C, 1, EFFVM_ATTACKER, 80, 0, 127, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 80, 90, 0, 0
    PlaySE SEQ_SE_FLD_162, 2, EFFVM_ATTACKER, 120, 0, 127, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 120, 50, 0, 0
    PlaySE SEQ_SE_LBDEMO_05, 2, EFFVM_DEFENDER, 200, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W229_KIIN, 1, EFFVM_DEFENDER, 270, -500, 100, 0, 0, 0
    PlaySE SEQ_SE_W009_01, 2, EFFVM_DEFENDER, 300, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W307_EXP, 1, EFFVM_PAN_CENTER, 300, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W307_EXP, 0, EFFVM_DEFENDER, 320, -700, 127, 0, 0, 0
    SetYield TRUE
    CameraMovePreset 1, 9, 32, 0, 26
    PaletteFade 4, 0, 12, 1, 0
    McssPalFade EFFVM_ATTACKER, 0, 12, 1, 0x5980
    ParticleLoad 738
    EmitterAdd 738, 4, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    McssSetValue EFFVM_ATTACKER, 1
    EmitterAdd 738, 4, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 738, 0, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssAlpha EFFVM_ATTACKER, 1, 1, 32, 1, 0
    WaitFrames 40
    CameraShake 0, 0x4cd, 0, 1, 1, 3
    McssMoveXY EFFVM_ATTACKER, 1, 0, 0xc000, 24, 1, 0
    EmitterAdd 738, 3, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 738, 3, EFFVM_EMIT_ATTACKER, 8, 0x4000, 0, 0, 0x1000, 0x1000, 0x99a, 0x1000
    EmitterAdd 738, 3, EFFVM_EMIT_ATTACKER, 8, 0x6000, 0, 0, 0x1000, 0x1000, 0x4cd, 0x1000
    EmitterAdd 738, 1, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 738, 6, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    McssSetMode EFFVM_ATTACKER, 3
    CameraMove 1, 0xca6, 0x5f33, 0x1dcc3, -0xe8d, 0xcd9a, 0x27f6, 32, 0, 26
    EmitterAddAt 738, 4, 0, 0xd000, 0, 0, 0, 0, 0, 0, 0, 0x800, 0x2000, 0x800, 0x1000
    EmitterAddAt 738, 7, 0, 0xd000, 0, 0, 0, 0, 0, 0, 0, 0x1000, 0x2000, 0x800, 0x1000
    WaitFrames 75
    EmitterAddAt 738, 5, 0, 0xd000, 0, 0, 0, 0, 0, 0, 0, 0x1000, 0x3000, 0x1000, 0x1000
    EmitterAddAt 738, 3, 0, 0xd000, 0, 0, 0, 0, 0, 0, 0, 0x1000, 0x3000, 0x3000, 0x1000
    EmitterAddAt 738, 4, 0, 0xd000, 0, 0, 0, 0, 0, 0, 0, 0x800, 0x2000, 0x800, 0x1000
    WaitFrames 80
    EmitterMoveFrom 738, 2, 2, 0, 0xd000, 0, EFFVM_EMIT_DEFENDER, 0x3000, 0x14000, -0x7000, 0x2000, 0x2000, 0
    CameraMovePreset 1, 11, 12, 0, 10
    WaitFrames 15
    CameraMove 2, 0x14000, 0, 0x1e000, 0, 0x6000, 0, 30, 1, 26
    CameraShake 0, 0x800, 4096, 1, 1, 6
    McssPalFade EFFVM_DEFENDER, 0, 12, 1, 0x5980
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 4, 1, 1
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 1, 1, 12
    EmitterAdd 738, 0, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1800, 0x1000, 0x1000
    EmitterAdd 738, 4, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x2000, 0x3000, 0x3000, 0x1000
    EmitterAdd 738, 5, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x2000, 0x3000, 0x1000
    EmitterAdd 738, 1, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x2000, 0x2000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssSetMode EFFVM_ATTACKER, 4
    McssMoveXY EFFVM_ATTACKER, 5, 0, 0, 8, 1, 0
    McssAlpha EFFVM_ATTACKER, 1, 31, 32, 1, 0
    WaitFrames 30
    CameraMovePreset 1, 8, 50, 0, 46
    PaletteFade 4, 12, 0, 3, 0
    McssPalFade EFFVM_ATTACKER, 12, 0, 1, 0x5980
    McssPalFade EFFVM_DEFENDER, 12, 0, 1, 0x5980
    McssSetVanish EFFVM_DEFENDER, FALSE
    WaitFrames 26
    McssSetValue EFFVM_ATTACKER, 0
    Wait EFFVM_WAIT_ALL
    End

FusionBolt_Variant2:
    CameraMovePreset 1, 9, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    CameraMove 2, 0xca6, 0x5f33, 0x1dcc3, -0xe8d, 0xcd9a, 0x27f6, 32, 0, 26
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
