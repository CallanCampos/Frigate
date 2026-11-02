// MOVE_REVENGE's animation: member 279 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Revenge

Revenge:
    PaletteFade 4, 0, 12, 3, 0
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    ParticleLoad 449
    EmitterAdd 449, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W018_02, 2, EFFVM_ATTACKER, 0, -300, 80, 0, 0, 0
    PlaySE SEQ_SE_W379_POWER, 1, EFFVM_ATTACKER, 0, 300, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 100, 100, 0, 0, 0, 0
    MoveSE 1, 0, 1, 120, 0, 40, 50, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 10, 0, 8
    McssSetVanish EFFVM_DEFENDER, TRUE
    EmitterAdd 449, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 449, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 449, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 3, 1, 1
    SetYield FALSE
    PlaySE SEQ_SE_W010_01, 2, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 0, 0, 100, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 12, 0, 3, 0
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
