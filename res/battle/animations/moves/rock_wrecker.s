// MOVE_ROCK_WRECKER's animation: member 439 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant RockWrecker

RockWrecker:
    LoadBg 177
    SetVanish 2, 0
    PaletteFade 4, 0, 13, 2, 0
    SetVanish 0, 1
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    BgMove 4, -36, 0, 160, 0, 0
    SetVanish 1, 1
    McssSetValue EFFVM_ATTACKER, 1
    McssSetValue EFFVM_DEFENDER, 1
    PaletteFade 3, 13, 0, 2, 0
    SetYield FALSE
    PlaySE SEQ_SE_W157_01, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 70, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 2000, 0, 70, 0, 0
    MoveSE 1, 0, 1, 120, 0, 60, 10, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_ATTACKER, 0, 70, 0, 0
    SetYield TRUE
    ParticleLoad 618
    EmitterAdd 618, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 618, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_PARTICLES
    SetYield FALSE
    PlaySE SEQ_SE_W319_02, 2, EFFVM_DEFENDER, 0, -400, 100, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 30, 0, 0
    SetYield TRUE
    EmitterMove 618, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x1e000, 0, 0x1000, 0x1000, 0
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 20
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W011_01, 2, EFFVM_DEFENDER, 0, -500, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 15, 0, 60, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 30, 0, 20, 0, 0, 0
    SetYield TRUE
    EmitterAdd 618, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 618, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 1, 4
    Wait EFFVM_WAIT_PARTICLES
    PaletteFade 3, 0, 13, 2, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 1, 0
    McssSetValue EFFVM_ATTACKER, 0
    McssSetValue EFFVM_DEFENDER, 0
    CameraMovePreset 1, 8, 16, 0, 8
    PaletteFade 4, 13, 2, 2, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    SetVanish 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
