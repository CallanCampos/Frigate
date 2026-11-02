// MOVE_UPROAR's animation: member 253 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Uproar

Uproar:
    CameraMovePreset 1, 9, 16, 0, 8
    McssSetValue EFFVM_ALL, 1
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W253_02, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W253_01, 2, EFFVM_ATTACKER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W253_02, 1, EFFVM_ATTACKER, 15, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W253_02, 1, EFFVM_ATTACKER, 30, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W253_02, 1, EFFVM_ATTACKER, 45, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 80, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 115, 0, 70, 10, 0, 0
    SetYield TRUE
    ParticleLoad 423
    EmitterAdd 423, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    EmitterAdd 423, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x800, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_ATTACKER, 2, 0x333, -0x333, 6, 1, 3
    Wait EFFVM_WAIT_ALL
    McssSetValue EFFVM_ALL, 0
    CameraMovePreset 1, 8, 12, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
