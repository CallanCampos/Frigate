// MOVE_FOLLOW_ME's animation: member 266 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant FollowMe

FollowMe:
    CameraMovePreset 1, 9, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W124_01, 1, EFFVM_ATTACKER, 0, 1000, 90, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 80, 1000, 0, 0, 0, 0
    MoveSE 1, 0, 1, 120, 0, 40, 40, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_07, 2, EFFVM_ATTACKER, 75, 300, 120, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_07, 2, EFFVM_ATTACKER, 95, 300, 120, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 436
    EmitterAdd 436, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 436, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
