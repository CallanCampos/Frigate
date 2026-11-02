// MOVE_POWER_GEM's animation: member 408 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant PowerGem

PowerGem:
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    PaletteFade 4, 0, 12, 1, 0
    McssPalFade EFFVM_ATTACKER, 0, 8, 6, 0x7fff
    SetYield FALSE
    PlaySE SEQ_SE_W278_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 80, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 0, 3000, 0, 75, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W319_02, 2, EFFVM_ATTACKER, 80, 0, 120, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 75, 40, 0, 0
    SetYield TRUE
    ParticleLoad 587
    EmitterAdd 587, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 587, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 587, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 587, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 80
    McssPalFade EFFVM_ATTACKER, 8, 0, 1, 0x7fff
    CameraMovePreset 1, 11, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_DEFENDER, 30, 600, 115, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 1, EFFVM_DEFENDER, 35, 600, 115, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 1, EFFVM_DEFENDER, 40, 600, 115, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 1, EFFVM_DEFENDER, 45, 600, 115, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 1, EFFVM_DEFENDER, 50, 600, 115, 0, 0, 0
    SetYield TRUE
    WaitFrames 20
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 3
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 12, 0, 1, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
