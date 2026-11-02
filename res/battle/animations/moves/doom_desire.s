// MOVE_DOOM_DESIRE's animation: member 353 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant DoomDesire
    ScriptVariant DoomDesire_Variant1

DoomDesire:
    CameraMovePreset 1, 14, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 0, 13, 0, 0
    McssScale EFFVM_ATTACKER, 2, 0x4cd, 0x4cd, 6, 2, 2
    SetYield FALSE
    PlaySE SEQ_SE_W464_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W464_01, 2, EFFVM_ATTACKER, 40, 0, 120, 0, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_ATTACKER, 0, 10, 2, 0x7fff
    McssPalFade EFFVM_DEFENDER, 0, 10, 2, 0
    WaitFrames 10
    McssPalFade EFFVM_ATTACKER, 10, 0, 2, 0x7fff
    WaitFrames 10
    McssPalFade EFFVM_ATTACKER, 0, 10, 2, 0x7fff
    WaitFrames 10
    McssPalFade EFFVM_ATTACKER, 10, 0, 2, 0x7fff
    McssPalFade EFFVM_DEFENDER, 10, 0, 2, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 13, 0, 0, 0
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End

DoomDesire_Variant1:
    PaletteFade 4, 0, 13, 2, 32767
    PlaySE SEQ_SE_W239_01, 1, EFFVM_DEFENDER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 100, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 0, 1000, 0, 100, 0, 0
    MoveSE 1, 0, 1, 115, 0, 80, 20, 0, 0
    PlaySE SEQ_SE_W319_01, 2, EFFVM_DEFENDER, 60, 0, 110, 0, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 100, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 110, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 120, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 130, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 140, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 150, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 160, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 170, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 180, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 190, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 200, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 210, 0, 115, 0, 0, 0
    SetYield TRUE
    SetVanish 0, 1
    Wait EFFVM_WAIT_ALL
    ParticleLoad 530
    EmitterAddAt 530, 2, 0, 0, 0, 0, 0, 0, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddAt 530, 0, 0, 0, 0, 0, 0, 0, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 50
    CameraMovePreset 1, 11, 12, 0, 8
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 2, 2, 6
    EmitterAdd 530, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    EmitterAdd 530, 1, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    EmitterAdd 530, 1, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    PaletteFade 4, 13, 0, 2, 32767
    CameraMovePreset 1, 8, 12, 0, 8
    Wait EFFVM_WAIT_ALL
    End
