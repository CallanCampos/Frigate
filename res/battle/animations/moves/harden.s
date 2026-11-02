// MOVE_HARDEN's animation: member 106 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Harden

Harden:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W027_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 40, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 3000, 0, 40, 0, 0
    PlaySE SEQ_SE_W043_01, 2, EFFVM_ATTACKER, 30, -500, 80, 0, 0, 0
    SetYield TRUE
    ParticleLoad 276
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssScale EFFVM_ATTACKER, 2, 0x19a, 0x19a, 2, 0, 4
    McssPalFade EFFVM_ATTACKER, 0, 10, 0, 0x7fff
    Wait EFFVM_WAIT_ALL
    EmitterAdd 276, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 40
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssPalFade EFFVM_ATTACKER, 10, 0, 0, 0x7fff
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
