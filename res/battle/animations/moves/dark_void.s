// MOVE_DARK_VOID's animation: member 464 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant DarkVoid

DarkVoid:
    McssSetValue EFFVM_ALL, 1
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    LoadBg 192
    SetVanish 2, 0
    PaletteFade 4, 0, 13, 1, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 1
    SetVanish 1, 1
    PaletteFade 3, 13, 0, 1, 0
    BgMove 4, 0, -1, 240, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W464_02, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W464_01, 2, EFFVM_DEFENDER, 50, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_SYS_31, 1, EFFVM_DEFENDER, 100, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 644
    EmitterAdd 644, 1, EFFVM_EMIT_DEFENDER, 8, 0x4cd, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 644, 2, EFFVM_EMIT_DEFENDER, 8, 0x333, 0, 0, 0x1000, 0x1000, 0x5000, 0x1000
    EmitterAdd 644, 0, EFFVM_EMIT_DEFENDER, 8, 0x333, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 644, 4, EFFVM_EMIT_DEFENDER, 8, 0x333, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    McssMoveXY EFFVM_DEFENDER, 2, 0, -0x4cd, 2, 0, 1
    Wait EFFVM_WAIT_MCSS
    McssAlpha EFFVM_DEFENDER, 2, 1, 4, 0, 0
    McssMoveXY EFFVM_DEFENDER, 2, 0, -0x1000, 10, 0, 1
    EmitterAdd 644, 3, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_MCSS
    McssMoveXY EFFVM_DEFENDER, 2, 0, -0xc000, 60, 0, 1
    WaitFrames 1
    McssSetMode EFFVM_DEFENDER, 3
    Wait EFFVM_WAIT_PARTICLES
    PaletteFade 4, 0, 15, 1, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 0
    SetVanish 1, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 15, 0, 1, 0
    McssSetMode EFFVM_DEFENDER, 4
    McssSetValue EFFVM_ALL, 0
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
