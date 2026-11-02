// MOVE_CHATTER's animation: member 448 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Chatter

Chatter:
    PlayCry EFFVM_ATTACKER, 0, 0, 0, 0, 0, 0
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_FLD_103, 1, EFFVM_ATTACKER, 0, 0, 70, 0, 0, 0
    PlaySE SEQ_SE_SYS_43, 2, EFFVM_ATTACKER, 40, 150, 127, 0, 0, 0
    PlaySE SEQ_SE_SYS_43, 2, EFFVM_ATTACKER, 70, 150, 127, 0, 0, 0
    SetYield TRUE
    ParticleLoad 627
    EmitterAdd 627, 0, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 627, 1, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_ATTACKER, 2, 0x1000, 0, 6, 1, 3
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    Wait EFFVM_WAIT_CRY
    End
    .balign 4
