// MOVE_TAIL_WHIP's animation: member 39 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant TailWhip

TailWhip:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W039_01, 1, EFFVM_ATTACKER, 0, 0, 90, 0, 0, 0
    PlaySE SEQ_SE_W039_01, 1, EFFVM_ATTACKER, 35, 0, 90, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssCircle EFFVM_ATTACKER, 2, 2, 0x1000, 0x1000, 0x10000, 0x1000, 0x2000, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
