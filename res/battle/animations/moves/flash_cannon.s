// MOVE_FLASH_CANNON's animation: member 430 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant FlashCannon

FlashCannon:
    PaletteFade 4, 0, 13, 2, 0
    McssSetValue EFFVM_ALL, 1
    CameraMovePreset 1, 9, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W179_03, 1, EFFVM_ATTACKER, 30, 0, 110, 0, 0, 0
    MoveSE 1, 0, 1, 110, 0, 100, 30, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W015_01, 2, EFFVM_ATTACKER, 130, -1000, 127, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 70, 20, 0, 0
    SetYield TRUE
    ParticleLoad 609
    ParticleLoad 610
    EmitterAdd 609, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 609, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 609, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 609, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 609, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 610, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 610, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 5
    WaitFrames 120
    EmitterMove 610, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x10000, 0, 0x99a, 0x1000, 0
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 16
    SetYield FALSE
    PlaySE SEQ_SE_W129_04, 2, EFFVM_DEFENDER, 0, 100, 90, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    EmitterAdd 610, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x99a, 0x1000
    WaitFrames 5
    EmitterAdd 610, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x99a, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 30
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 2, 0, 8
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 2, 0, 10
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 13, 0, 2, 0
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetValue EFFVM_ATTACKER, 0
    McssSetVanish EFFVM_ALL, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
