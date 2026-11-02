// MOVE_SHADOW_SNEAK's animation: member 425 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant ShadowSneak

ShadowSneak:
    McssSetValue EFFVM_ALL, 1
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 0, 5, 2, 32767
    SetYield FALSE
    PlaySE SEQ_SE_W028_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W157_02, 2, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 110, 0, 0, 0, 0, 0
    MoveSE 2, 0, 0, -2000, 1000, 0, 50, 0, 0
    MoveSE 2, 0, 1, 115, 0, 80, 20, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 40, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 60, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 70, -200, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 75, -200, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 80, -200, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 604
    EmitterMove 604, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x1000, 0, 0, 0x1000, 0x1000, 0
    WaitFrames 30
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 30
    EmitterAdd 604, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x800, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssPalFade EFFVM_DEFENDER, 0, 12, 0, 0
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 2, 1, 4
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 12, 0, 0, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetValue EFFVM_ALL, 0
    PaletteFade 4, 5, 0, 2, 32767
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
