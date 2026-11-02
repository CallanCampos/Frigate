// MOVE_RECYCLE's animation: member 278 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Recycle

Recycle:
    CameraMovePreset 1, 9, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W278_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 100, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 120, 0, 70, 30, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 448
    EmitterAdd 448, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
