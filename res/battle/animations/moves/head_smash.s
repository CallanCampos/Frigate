// MOVE_HEAD_SMASH's animation: member 457 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant HeadSmash

HeadSmash:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssMoveXY EFFVM_ATTACKER, 2, 0x1000, 0, 2, 2, 1
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    LoadBg 180
    SetVanish 2, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 0, 13, 1, 0
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 1
    SetVanish 1, 1
    PaletteFade 3, 13, 0, 1, 0
    BgMove 2, 8, 0, 2, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W029_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 636
    EmitterAdd 636, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 636, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 636, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 636, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 636, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssPalFade EFFVM_DEFENDER, 0, 12, 2, 0x1e
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 1, 4
    Wait EFFVM_WAIT_PARTICLES
    PaletteFade 3, 0, 13, 1, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 13, 0, 1, 0
    SetVanish 1, 0
    SetVanish 0, 0
    McssPalFade EFFVM_DEFENDER, 12, 0, 2, 0x1e
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
