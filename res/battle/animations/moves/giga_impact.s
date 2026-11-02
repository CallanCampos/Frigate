// MOVE_GIGA_IMPACT's animation: member 416 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant GigaImpact

GigaImpact:
    CameraMovePreset 1, 11, 12, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_FLD_39, 1, EFFVM_ATTACKER, 0, 200, 120, 0, 0, 0
    SetYield TRUE
    McssCircle EFFVM_ATTACKER, 0, 1, 0x1000, 0, 0xa000, 0x1000, 0x1000, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W157_03, 3, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 3, EFFVM_DEFENDER, 60, 0, 0, 0, 0, 0
    MoveSE 3, 0, 0, 0, 2000, 0, 40, 0, 0
    MoveSE 3, 0, 1, 120, 0, 40, 20, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 60, 300, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 70, 300, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 90, 0, 50, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 110, 0, 10, 0, 0, 0
    PlaySE SEQ_SE_W295_02, 2, EFFVM_DEFENDER, 60, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 595
    EmitterAdd 595, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 595, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 595, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 595, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 595, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 40
    LoadBg 162
    PaletteFade 4, 0, 13, 1, 0
    BgMove 2, 0, 4, 1, 1, 20
    WaitFrames 10
    SetVanish 1, 1
    SetVanish 0, 1
    SetVanish 2, 0
    WaitFrames 10
    PaletteFade 3, 13, 0, 0, 0
    BgPalAnim 28, 1
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 2, 1, 4
    Wait EFFVM_WAIT_ALL
    SetVanish 1, 0
    SetVanish 0, 0
    SetVanish 2, 1
    PaletteFade 4, 13, 0, 1, 0
    CameraMovePreset 1, 8, 12, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
