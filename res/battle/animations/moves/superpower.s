// MOVE_SUPERPOWER's animation: member 276 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Superpower

Superpower:
    McssSetValue EFFVM_ALL, 1
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W157_03, 3, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 3, EFFVM_ATTACKER, 100, 0, 0, 0, 0, 0
    MoveSE 3, 0, 0, 0, 2200, 0, 100, 0, 0
    MoveSE 3, 0, 1, 120, 0, 70, 30, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssMoveXY EFFVM_ATTACKER, 2, 0x333, 0, 2, 1, 10
    ParticleLoad 446
    EmitterAdd 446, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    EmitterAdd 446, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 2, EFFVM_ATTACKER, 0, 700, 120, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssCircle EFFVM_ATTACKER, 0, 1, 0x1000, 0, 0xa000, 0x1000, 0x1000, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 0, 16, 1, 0
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W011_01, 2, EFFVM_DEFENDER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 22, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 45, 0, 60, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 65, 0, 30, 0, 0, 0
    SetYield TRUE
    EmitterAdd 446, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    LoadBg 120
    PaletteFade 3, 16, 0, 0, 0
    EmitterAdd 446, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    BgPalAnim 16, 3
    SetVanish 2, 0
    SetVanish 1, 1
    SetVanish 0, 1
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 6
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 2, 1, 2
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 16, 0, 1, 0
    SetVanish 1, 0
    SetVanish 0, 0
    SetVanish 2, 1
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetValue EFFVM_ALL, 0
    Wait EFFVM_WAIT_ALL
    End
