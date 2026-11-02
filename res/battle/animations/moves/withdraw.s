// MOVE_WITHDRAW's animation: member 110 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Withdraw

Withdraw:
    CameraMovePreset 1, 9, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 10, -800, 120, 0, 0, 0
    PlaySE SEQ_SE_W011_01, 2, EFFVM_ATTACKER, 30, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_SYS_39, 2, EFFVM_ATTACKER, 40, 0, 90, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 100, 0, 0, 0, 0, 0
    MoveSE 2, 0, 0, 0, 2900, 40, 50, 0, 0
    MoveSE 2, 0, 1, 90, 0, 80, 20, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssScale EFFVM_ATTACKER, 2, -0x333, -0x333, 14, 1, 1
    ParticleLoad 279
    EmitterAdd 279, 0, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 279, 1, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 279, 2, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 279, 3, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
