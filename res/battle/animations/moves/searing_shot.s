// MOVE_SEARING_SHOT's animation: member 545 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant SearingShot

SearingShot:
    CameraMovePreset 1, 9, 16, 0, 8
    LoadBg 210
    SetVanish 2, 0
    PaletteFade 4, 0, 16, 1, 0
    WaitFrames 10
    SetVanish 0, 1
    SetVanish 1, 1
    SetYield FALSE
    PlaySE SEQ_SE_W179_03, 1, EFFVM_ATTACKER, 0, 500, 60, 0, 0, 0
    MoveSE 1, 0, 1, 60, 0, 45, 9, 0, 0
    PlaySE SEQ_SE_W464_02, 1, EFFVM_ATTACKER, 55, 0, 127, 0, 0, 0
    SetYield TRUE
    ParticleLoad 714
    EmitterAdd 714, 5, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 714, 6, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 714, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 714, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 714, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_ATTACKER, 0, 10, 3, 0x1f
    WaitFrames 40
    WaitFrames 40
    McssPalFade EFFVM_ATTACKER, 10, 0, 3, 0x1f
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 20
    SetYield FALSE
    PlaySE SEQ_SE_W088_02, 2, EFFVM_DEFENDER, 0, 300, 100, 0, 0, 0
    PlaySE SEQ_SE_W464_02, 1, EFFVM_DEFENDER, 10, 300, 127, 0, 0, 0
    PlaySE SEQ_SE_W088_02, 2, EFFVM_DEFENDER, 20, 200, 127, 0, 0, 0
    PlaySE SEQ_SE_W464_02, 1, EFFVM_DEFENDER, 30, 200, 127, 0, 0, 0
    PlaySE SEQ_SE_W088_02, 2, EFFVM_DEFENDER, 40, 500, 100, 0, 0, 0
    PlaySE SEQ_SE_W464_02, 2, EFFVM_DEFENDER, 50, 300, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_127, 1, EFFVM_DEFENDER, 50, 0, 127, 0, 0, 0
    SetYield TRUE
    McssScale EFFVM_ATTACKER, 2, 0x666, -0x666, 8, 0, 1
    McssSetVanish EFFVM_DEFENDER, TRUE
    EmitterAdd 714, 0, EFFVM_EMIT_DEFENDER, 8, 0x1800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    McssPalFade EFFVM_DEFENDER, 0, 10, 3, 0x1f
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 4, 0, 9
    EmitterAdd 714, 4, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    PaletteFade 4, 16, 0, 2, 0
    BgMove 2, 6, 0, 2, 0, 20
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 10, 0, 3, 0x1f
    PaletteFade 4, 0, 16, 2, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 16, 0, 2, 0
    CameraMovePreset 1, 8, 16, 0, 8
    SetVanish 0, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    SetVanish 1, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
