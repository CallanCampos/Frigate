// MOVE_LEAF_STORM's animation: member 437 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant LeafStorm

LeafStorm:
    CameraMovePreset 1, 9, 10, 0, 6
    LoadBg 33
    SetVanish 2, 0
    PaletteFade 4, 0, 15, 2, 0
    Wait EFFVM_WAIT_ALL
    BgMove 4, -36, 0, 100, 0, 0
    SetVanish 0, 1
    SetVanish 1, 1
    PaletteFade 4, 15, 0, 2, 0
    SetYield FALSE
    PlaySE SEQ_SE_W239_01, 1, EFFVM_ATTACKER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 50, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 0, 1000, 0, 40, 0, 0
    MoveSE 1, 0, 1, 100, 0, 30, 10, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_FLD_80, 2, EFFVM_ATTACKER, 30, 1000, 120, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 60, 0, 0
    PlaySE SEQ_SE_W010_01, 2, EFFVM_DEFENDER, 60, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W010_01, 2, EFFVM_DEFENDER, 75, -500, 50, 0, 0, 0
    PlaySE SEQ_SE_W010_01, 2, EFFVM_DEFENDER, 90, -500, 10, 0, 0, 0
    SetYield TRUE
    ParticleLoad 616
    EmitterMove 616, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    EmitterAdd 616, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 616, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 40
    CameraMovePreset 1, 11, 10, 0, 6
    McssSetVanish EFFVM_DEFENDER, TRUE
    EmitterAdd 616, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 1, 4
    Wait EFFVM_WAIT_PARTICLES
    PaletteFade 4, 0, 15, 0, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 0
    SetVanish 1, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 12, 0, 8
    PaletteFade 4, 15, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    End
