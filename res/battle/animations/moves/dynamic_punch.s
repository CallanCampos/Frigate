// MOVE_DYNAMIC_PUNCH's animation: member 223 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant DynamicPunch

DynamicPunch:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    LoadBg 102
    PaletteFade 4, 0, 13, 0, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 3, 13, 0, 0, 0
    ParticleLoad 393
    EmitterAdd 393, 6, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 393, 5, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 393, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 393, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 393, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 393, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 20, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W307_01, 2, EFFVM_DEFENDER, 20, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 100, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 120, 0, 60, 40, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 30, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 40, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 50, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 60, 0, 100, 0, 0, 0
    SetYield TRUE
    WaitFrames 10
    EmitterAdd 393, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 4, 0, 2
    BgMove 2, 0, 4, 1, 1, 20
    SetVanish 2, 0
    SetVanish 0, 1
    SetVanish 1, 1
    McssPalFade EFFVM_DEFENDER, 0, 12, 2, 0
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 1, 16
    Wait EFFVM_WAIT_ALL
    PaletteFade 3, 0, 16, 1, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 16, 0, 1, 0
    SetVanish 0, 0
    SetVanish 1, 0
    SetVanish 2, 1
    CameraMovePreset 1, 8, 16, 0, 8
    McssPalFade EFFVM_DEFENDER, 12, 0, 0, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
