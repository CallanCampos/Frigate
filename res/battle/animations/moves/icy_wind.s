// MOVE_ICY_WIND's animation: member 196 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant IcyWind

IcyWind:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 0, 16, 2, 0
    SetYield FALSE
    PlaySE SEQ_SE_W018_01, 1, EFFVM_DEFENDER, 10, 0, 80, 0, 0, 0
    PlaySE SEQ_SE_W059_01, 2, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 80, 0, 0, 0, 0, 0
    MoveSE 2, 0, 0, 0, 1000, 0, 20, 0, 0
    MoveSE 2, 0, 0, 1000, 0, 40, 30, 0, 0
    MoveSE 2, 0, 1, 0, 120, 0, 30, 0, 0
    MoveSE 2, 0, 1, 120, 0, 40, 40, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 60, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 60, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W181_01, 1, EFFVM_DEFENDER, 80, -70, 90, 0, 0, 0
    PlaySE SEQ_SE_W008_01, 2, EFFVM_DEFENDER, 80, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W181_01, 1, EFFVM_DEFENDER, 100, -70, 90, 0, 0, 0
    PlaySE SEQ_SE_W008_01, 2, EFFVM_DEFENDER, 100, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 366
    EmitterMove 366, 1, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    EmitterMove 366, 2, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    WaitFrames 30
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 30
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 2, 2, 4
    EmitterAdd 366, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssPalFade EFFVM_DEFENDER, 0, 12, 3, 0x7fff
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 12, 0, 3, 0x7fff
    CameraMovePreset 1, 8, 16, 0, 8
    PaletteFade 4, 16, 0, 2, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
