// MOVE_PSYSTRIKE's animation: member 540 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Psystrike

Psystrike:
    LoadBg 204
    SetVanish 2, 0
    SetYield FALSE
    PlaySE SEQ_SE_SHINKA_W060C, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    CameraMovePreset 1, 9, 16, 0, 8
    ParticleLoad 709
    EmitterAdd 709, 5, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssMoveXY EFFVM_ATTACKER, 2, 0x4cd, 0, 2, 0, 10
    WaitFrames 30
    CameraMovePreset 1, 11, 16, 0, 8
    PaletteFade 4, 0, 16, 2, 0
    WaitFrames 20
    SetYield FALSE
    PlaySE SEQ_SE_SYS_24, 2, EFFVM_DEFENDER, 0, 100, 127, 0, 0, 0
    PlaySE SEQ_SE_W540_KYUN, 2, EFFVM_DEFENDER, 40, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 70, -100, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 77, 50, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 84, -200, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 91, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 100, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 105, 0, 127, 0, 0, 0
    SetYield TRUE
    EmitterAdd 709, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 709, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetValue EFFVM_ALL, 1
    WaitFrames 10
    SetVanish 1, 1
    SetVanish 0, 1
    EmitterAdd 709, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    EmitterAdd 709, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    EmitterAdd 709, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 10
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 1, 5
    PaletteFade 4, 16, 0, 2, 0
    BgPalAnim 34, 20
    WaitFrames 50
    PaletteFade 4, 0, 16, 2, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 1, 0
    SetVanish 0, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 16, 0, 2, 0
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetValue EFFVM_ALL, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
