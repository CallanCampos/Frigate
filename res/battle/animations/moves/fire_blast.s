// MOVE_FIRE_BLAST's animation: member 126 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant FireBlast

FireBlast:
    ParticleLoad 295
    CameraMovePreset 1, 9, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W307_02, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W239_01, 0, EFFVM_ATTACKER, 0, 0, 70, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 0, EFFVM_ATTACKER, 50, 0, 0, 0, 0, 0
    MoveSE 0, 0, 0, -1000, 2000, 0, 45, 0, 0
    SetYield TRUE
    EmitterAdd 295, 6, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    CameraMovePreset 1, 11, 32, 0, 28
    EmitterAdd 295, 7, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    PaletteFade 2, 0, 13, 1, 31
    SetYield FALSE
    PlaySE SEQ_SE_W307_01, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 60, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 0, EFFVM_DEFENDER, 10, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 20, -500, 120, 0, 0, 0
    SetYield TRUE
    EmitterAdd 295, 0, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 295, 1, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 295, 2, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 295, 3, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 295, 4, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 295, 5, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_DEFENDER, 0, 10, 2, 0x1f
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x4cd, 2, 1, 6
    LoadBg 60
    SetVanish 1, 1
    SetVanish 0, 1
    SetVanish 2, 0
    BgPalAnim 6, 3
    BgMove 2, 0, 8, 2, 0, 16
    WaitFrames 60
    McssPalFade EFFVM_DEFENDER, 10, 0, 2, 0x1f
    PaletteFade 3, 0, 16, 0, 0
    WaitFrames 10
    PaletteFade 2, 16, 0, 1, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    SetVanish 1, 0
    SetVanish 0, 0
    SetVanish 2, 1
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    PaletteFade 3, 16, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    End
