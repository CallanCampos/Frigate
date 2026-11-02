// MOVE_GRAVITY's animation: member 356 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Gravity

Gravity:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetValue EFFVM_ALL, 1
    McssSetVanish EFFVM_DEFENDER, TRUE
    ParticleLoad 533
    ParticleLoad 534
    PlaySE SEQ_SE_W239_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 110, 0, 115, 0, 0, 0
    MoveSE 1, 0, 0, 0, -3000, 0, 110, 0, 0
    MoveSE 1, 0, 1, 115, 0, 90, 20, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W464_01, 2, EFFVM_DEFENDER, 45, 0, 115, 0, 0, 0
    SetYield FALSE
    EmitterAdd 533, 0, EFFVM_EMIT_ATTACKER, 8, 0x19a, 0, 0, 0x1000, 0x800, 0x1000, 0x1000
    EmitterAdd 534, 0, EFFVM_EMIT_ATTACKER, 8, 0x19a, 0, 0, 0x1000, 0xccd, 0x1000, 0x1000
    EmitterAdd 533, 1, 0, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    PaletteFade 4, 0, 6, 1, 0
    WaitFrames 20
    CameraMovePreset 1, 8, 42, 0, 38
    McssMoveXY EFFVM_DEFENDER, 2, 0, 0x333, 1, 2, 15
    McssScale EFFVM_DEFENDER, 2, 0, -0xb33, 22, 1, 1
    Wait EFFVM_WAIT_ALL
    McssSetValue EFFVM_ALL, 0
    PaletteFade 4, 6, 0, 1, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
