// MOVE_ASSIST's animation: member 274 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Assist

Assist:
    CameraMovePreset 1, 9, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W274_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W274_01, 1, EFFVM_ATTACKER, 5, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W274_01, 1, EFFVM_ATTACKER, 10, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W274_01, 1, EFFVM_ATTACKER, 15, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W274_01, 1, EFFVM_ATTACKER, 20, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W274_01, 1, EFFVM_ATTACKER, 25, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W274_01, 1, EFFVM_ATTACKER, 30, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W274_01, 1, EFFVM_ATTACKER, 35, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W274_01, 1, EFFVM_ATTACKER, 40, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W274_01, 1, EFFVM_ATTACKER, 45, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W027_01, 2, EFFVM_ATTACKER, 65, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 120, 0, 0, 0, 0, 0
    MoveSE 2, 0, 0, -1000, 2000, 65, 60, 0, 0
    MoveSE 2, 0, 1, 110, 0, 100, 20, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 443
    EmitterAdd 443, 0, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 443, 1, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
