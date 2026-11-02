// MOVE_ZEN_HEADBUTT's animation: member 428 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant ZenHeadbutt

ZenHeadbutt:
    CameraMovePreset 1, 9, 10, 0, 6
    PaletteFade 4, 0, 13, 2, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W129_04, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    MoveSE 1, 0, 0, -4000, 4000, 0, 64, 0, 0
    PlaySE SEQ_SE_W015_01, 2, EFFVM_DEFENDER, 60, -300, 120, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 55, 20, 0, 0
    SetYield TRUE
    ParticleLoad 607
    EmitterAdd 607, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 607, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 60
    McssMoveXY EFFVM_ATTACKER, 2, 0x2000, 0, 2, 1, 1
    WaitFrames 1
    CameraMovePreset 1, 11, 8, 0, 4
    WaitFrames 10
    SetYield FALSE
    PlaySE SEQ_SE_W029_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    EmitterAdd 607, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 607, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 607, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 2, 1, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 10, 0, 6
    McssSetVanish EFFVM_ATTACKER, FALSE
    PaletteFade 4, 13, 0, 2, 0
    Wait EFFVM_WAIT_ALL
    End
