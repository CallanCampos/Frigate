// MOVE_SLACK_OFF's animation: member 303 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant SlackOff

SlackOff:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W281_AKUBI, 1, EFFVM_ATTACKER, 0, -500, 120, 0, 0, 0
    PlaySE SEQ_SE_W234_HIKARI, 2, EFFVM_ATTACKER, 0, 0, 70, 0, 0, 0
    SetYield TRUE
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 6, 1, 1
    ParticleLoad 473
    McssPalFade EFFVM_ATTACKER, 0, 12, 3, 0x7fff
    EmitterAdd 473, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 12, 0, 3, 0x7fff
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
